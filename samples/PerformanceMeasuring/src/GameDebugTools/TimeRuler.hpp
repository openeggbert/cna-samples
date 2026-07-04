#pragma once

// TimeRuler.hpp — C++ port of GameDebugTools/TimeRuler.cs (XNA 4.0
// PerformanceMeasuring sample). Realtime CPU measuring tool: visualizes
// BeginMark/EndMark-instrumented sections of code as colored bars, plus an
// optional text log of min/max/avg times per marker.
//
// Adaptation note: the original wraps all profiling calls in
// [Conditional("TRACE")] so a release build can compile them away, and guards
// shared state with `lock(this)`/Interlocked since XNA's fixed-timestep catch-up
// can call Update from... in practice, the same thread as StartFrame/BeginMark.
// This is a single-threaded desktop sample with no release/debug split, so the
// profiling code is always active and the locking is dropped (a plain int
// counter replaces Interlocked) — see missing.md.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "System/Diagnostics/Stopwatch.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"

#include "DebugManager.hpp"
#include "IDebugCommandHost.hpp"
#include "Layout.hpp"

namespace PerformanceMeasuring::GameDebugTools {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// Realtime CPU measuring tool. Port of GameDebugTools/TimeRuler.cs.
class TimeRuler : public DrawableGameComponent {
public:
    bool ShowLog = false;
    int TargetSampleFrames = 1;

    Vector2 Position;
    int Width = 0;

    explicit TimeRuler(Game& game) : DrawableGameComponent(game) {
        game.getServicesProperty().AddService<TimeRuler>(this);
    }

    void Initialize() override {
        debugManager_ = getGameProperty().getServicesProperty().GetService<DebugManager>();
        if (debugManager_ == nullptr)
            throw std::runtime_error("DebugManager is not registered.");

        auto* host = getGameProperty().getServicesProperty().GetService<IDebugCommandHost>();
        if (host != nullptr) {
            host->RegisterCommand("tr", "TimeRuler",
                [this](IDebugCommandHost& h, const std::string&, const std::vector<std::string>& args) {
                    CommandExecute(h, args);
                });
            setVisibleProperty(true);
        }

        logs_[0] = FrameLog{};
        logs_[1] = FrameLog{};
        sampleFrames_ = TargetSampleFrames = 1;

        // The TimeRuler's Update method doesn't need to get called.
        setEnabledProperty(false);

        DrawableGameComponent::Initialize();
    }

    void LoadContent() override {
        // Not using getViewportProperty() here: this runs during Game::Initialize()
        // (DrawableGameComponent::Initialize() calls LoadContent() once), where CNA's
        // viewport is known to read stale/wrong values (see NEXT.md section 5's
        // Initialize()-time viewport gotcha) — use the known back-buffer default instead,
        // since this sample never overrides PreferredBackBufferWidth/Height.
        int backBufferWidth = Microsoft::Xna::Framework::GraphicsDeviceManager::DefaultBackBufferWidth;
        int backBufferHeight = Microsoft::Xna::Framework::GraphicsDeviceManager::DefaultBackBufferHeight;

        Width = (int)((float)backBufferWidth * 0.8f);

        Rectangle clientArea(0, 0, backBufferWidth, backBufferHeight);
        Layout layout(clientArea, clientArea);
        Position = layout.Place(Vector2((float)Width, (float)BarHeight), 0.0f, 0.01f, Alignment::BottomCenter);

        DrawableGameComponent::LoadContent();
    }

    // Starts a new frame. Call at the top of Game::Update.
    void StartFrame() {
        // We skip resetting the frame when this method gets called multiple times
        // (XNA's fixed-timestep catch-up calls Update more than once per Draw).
        int count = ++updateCount_;
        if (getVisibleProperty() && (1 < count && count < MaxSampleFrames))
            return;

        prevLog_ = &logs_[frameCount_++ & 0x1];
        curLog_ = &logs_[frameCount_ & 0x1];

        float endFrameTime = (float)stopwatch_.getElapsedProperty().getTotalMillisecondsProperty();

        for (int barIdx = 0; barIdx < MaxBars; ++barIdx) {
            MarkerCollection& prevBar = prevLog_->Bars[barIdx];
            MarkerCollection& nextBar = curLog_->Bars[barIdx];

            // Re-open markers that didn't get EndMark called in the previous frame.
            for (int nest = 0; nest < prevBar.NestCount; ++nest) {
                int markerIdx = prevBar.MarkerNests[nest];

                prevBar.Markers[markerIdx].EndTime = endFrameTime;

                nextBar.MarkerNests[nest] = nest;
                nextBar.Markers[nest].MarkerId = prevBar.Markers[markerIdx].MarkerId;
                nextBar.Markers[nest].BeginTime = 0.0f;
                nextBar.Markers[nest].EndTime = -1.0f;
                nextBar.Markers[nest].BarColor = prevBar.Markers[markerIdx].BarColor;
            }

            // Update the marker log.
            for (int markerIdx = 0; markerIdx < prevBar.MarkCount; ++markerIdx) {
                float duration = prevBar.Markers[markerIdx].EndTime - prevBar.Markers[markerIdx].BeginTime;

                int markerId = prevBar.Markers[markerIdx].MarkerId;
                MarkerInfo& m = markers_[markerId];

                m.Logs[barIdx].BarColor = prevBar.Markers[markerIdx].BarColor;

                if (!m.Logs[barIdx].Initialized) {
                    m.Logs[barIdx].Min = duration;
                    m.Logs[barIdx].Max = duration;
                    m.Logs[barIdx].Avg = duration;
                    m.Logs[barIdx].Initialized = true;
                } else {
                    m.Logs[barIdx].Min = std::min(m.Logs[barIdx].Min, duration);
                    m.Logs[barIdx].Max = std::min(m.Logs[barIdx].Max, duration);
                    m.Logs[barIdx].Avg += duration;
                    m.Logs[barIdx].Avg *= 0.5f;

                    if (m.Logs[barIdx].Samples++ >= LogSnapDuration) {
                        m.Logs[barIdx].SnapMin = m.Logs[barIdx].Min;
                        m.Logs[barIdx].SnapMax = m.Logs[barIdx].Max;
                        m.Logs[barIdx].SnapAvg = m.Logs[barIdx].Avg;
                        m.Logs[barIdx].Samples = 0;
                    }
                }
            }

            nextBar.MarkCount = prevBar.NestCount;
            nextBar.NestCount = prevBar.NestCount;
        }

        stopwatch_.Reset();
        stopwatch_.Start();
    }

    void BeginMark(const std::string& markerName, Color color) { BeginMark(0, markerName, color); }

    void BeginMark(int barIndex, const std::string& markerName, Color color) {
        if (barIndex < 0 || barIndex >= MaxBars)
            throw std::out_of_range("barIndex");

        MarkerCollection& bar = curLog_->Bars[barIndex];

        if (bar.MarkCount >= MaxSamples)
            throw std::overflow_error("Exceeded TimeRuler sample count.");

        if (bar.NestCount >= MaxNestCall)
            throw std::overflow_error("Exceeded TimeRuler nest count.");

        int markerId = GetOrRegisterMarker(markerName);

        bar.MarkerNests[bar.NestCount++] = bar.MarkCount;

        bar.Markers[bar.MarkCount].MarkerId = markerId;
        bar.Markers[bar.MarkCount].BarColor = color;
        bar.Markers[bar.MarkCount].BeginTime = (float)stopwatch_.getElapsedProperty().getTotalMillisecondsProperty();
        bar.Markers[bar.MarkCount].EndTime = -1.0f;

        bar.MarkCount++;
    }

    void EndMark(const std::string& markerName) { EndMark(0, markerName); }

    void EndMark(int barIndex, const std::string& markerName) {
        if (barIndex < 0 || barIndex >= MaxBars)
            throw std::out_of_range("barIndex");

        MarkerCollection& bar = curLog_->Bars[barIndex];

        if (bar.NestCount <= 0)
            throw std::runtime_error("Call BeginMark before calling EndMark.");

        auto it = markerNameToIdMap_.find(markerName);
        if (it == markerNameToIdMap_.end())
            throw std::runtime_error("Marker '" + markerName + "' is not registered.");

        int markerId = it->second;
        int markerIdx = bar.MarkerNests[--bar.NestCount];
        if (bar.Markers[markerIdx].MarkerId != markerId)
            throw std::runtime_error("Incorrect call order of BeginMark/EndMark.");

        bar.Markers[markerIdx].EndTime = (float)stopwatch_.getElapsedProperty().getTotalMillisecondsProperty();
    }

    float GetAverageTime(int barIndex, const std::string& markerName) const {
        if (barIndex < 0 || barIndex >= MaxBars)
            throw std::out_of_range("barIndex");

        auto it = markerNameToIdMap_.find(markerName);
        if (it == markerNameToIdMap_.end())
            return 0.0f;

        return markers_[it->second].Logs[barIndex].Avg;
    }

    void ResetLog() {
        for (MarkerInfo& markerInfo : markers_) {
            for (int i = 0; i < MaxBars; ++i) {
                markerInfo.Logs[i] = MarkerLog{};
            }
        }
    }

    void Draw(const GameTime&) override { Draw(Position, Width); }

    void Draw(Vector2 position, int width) {
        updateCount_ = 0;

        SpriteBatch& spriteBatch = debugManager_->getSpriteBatch();
        SpriteFont& font = debugManager_->getDebugFont();
        Texture2D& texture = debugManager_->getWhiteTexture();

        int height = 0;
        float maxTime = 0.0f;
        for (const MarkerCollection& bar : prevLog_->Bars) {
            if (bar.MarkCount > 0) {
                height += BarHeight + BarPadding * 2;
                maxTime = std::max(maxTime, bar.Markers[bar.MarkCount - 1].EndTime);
            }
        }

        const float frameSpan = 1.0f / 60.0f * 1000.0f;
        float sampleSpan = (float)sampleFrames_ * frameSpan;

        if (maxTime > sampleSpan)
            frameAdjust_ = std::max(0, frameAdjust_) + 1;
        else
            frameAdjust_ = std::min(0, frameAdjust_) - 1;

        if (std::abs(frameAdjust_) > AutoAdjustDelay) {
            sampleFrames_ = std::min(MaxSampleFrames, sampleFrames_);
            sampleFrames_ = std::max(TargetSampleFrames, (int)(maxTime / frameSpan) + 1);
            frameAdjust_ = 0;
        }

        float msToPs = (float)width / sampleSpan;

        int startY = (int)position.Y - (height - BarHeight);
        int y = startY;

        spriteBatch.Begin();

        Rectangle rc((int)position.X, y, width, height);
        spriteBatch.Draw(texture, rc, Color(0, 0, 0, 128));

        rc.Height = BarHeight;
        for (const MarkerCollection& bar : prevLog_->Bars) {
            rc.Y = y + BarPadding;
            for (int j = 0; j < bar.MarkCount; ++j) {
                float bt = bar.Markers[j].BeginTime;
                float et = bar.Markers[j].EndTime;
                int sx = (int)(position.X + bt * msToPs);
                int ex = (int)(position.X + et * msToPs);
                rc.X = sx;
                rc.Width = std::max(ex - sx, 1);

                spriteBatch.Draw(texture, rc, bar.Markers[j].BarColor);
            }

            y += BarHeight + BarPadding;
        }

        // Grid lines: one per millisecond.
        rc = Rectangle((int)position.X, startY, 1, height);
        for (float t = 1.0f; t < sampleSpan; t += 1.0f) {
            rc.X = (int)(position.X + t * msToPs);
            spriteBatch.Draw(texture, rc, Color::Gray);
        }

        // Frame grid.
        for (int i = 0; i <= sampleFrames_; ++i) {
            rc.X = (int)(position.X + frameSpan * (float)i * msToPs);
            spriteBatch.Draw(texture, rc, Color::White);
        }

        if (ShowLog) {
            y = startY - font.getLineSpacingProperty();
            std::string logString;
            for (const MarkerInfo& markerInfo : markers_) {
                for (int i = 0; i < MaxBars; ++i) {
                    if (markerInfo.Logs[i].Initialized) {
                        if (!logString.empty())
                            logString += "\n";

                        char buf[64];
                        std::snprintf(buf, sizeof(buf), " Bar %d %s Avg.:%.2fms ", i, markerInfo.Name.c_str(),
                                      markerInfo.Logs[i].SnapAvg);
                        logString += buf;

                        y -= font.getLineSpacingProperty();
                    }
                }
            }

            Vector2 size = font.MeasureString(logString);
            rc = Rectangle((int)position.X, y, (int)size.X + 12, (int)size.Y);
            spriteBatch.Draw(texture, rc, Color(0, 0, 0, 128));

            spriteBatch.DrawString(font, logString, Vector2(position.X + 12.0f, (float)y), Color::White);

            y += (int)((float)font.getLineSpacingProperty() * 0.3f);
            rc = Rectangle((int)position.X + 4, y, 10, 10);
            Rectangle rc2((int)position.X + 5, y + 1, 8, 8);
            for (const MarkerInfo& markerInfo : markers_) {
                for (int i = 0; i < MaxBars; ++i) {
                    if (markerInfo.Logs[i].Initialized) {
                        rc.Y = y;
                        rc2.Y = y + 1;
                        spriteBatch.Draw(texture, rc, Color::White);
                        spriteBatch.Draw(texture, rc2, markerInfo.Logs[i].BarColor);

                        y += font.getLineSpacingProperty();
                    }
                }
            }
        }

        spriteBatch.End();
    }

private:
    static constexpr int MaxBars = 8;
    static constexpr int MaxSamples = 256;
    static constexpr int MaxNestCall = 32;
    static constexpr int MaxSampleFrames = 4;
    static constexpr int LogSnapDuration = 120;
    static constexpr int BarHeight = 8;
    static constexpr int BarPadding = 2;
    static constexpr int AutoAdjustDelay = 30;

    struct Marker {
        int MarkerId = 0;
        float BeginTime = 0.0f;
        float EndTime = 0.0f;
        Color BarColor = Color::White;
    };

    struct MarkerCollection {
        std::array<Marker, MaxSamples> Markers;
        int MarkCount = 0;

        std::array<int, MaxNestCall> MarkerNests{};
        int NestCount = 0;
    };

    struct FrameLog {
        std::array<MarkerCollection, MaxBars> Bars;
    };

    struct MarkerLog {
        float SnapMin = 0.0f, SnapMax = 0.0f, SnapAvg = 0.0f;
        float Min = 0.0f, Max = 0.0f, Avg = 0.0f;
        int Samples = 0;
        Color BarColor = Color::White;
        bool Initialized = false;
    };

    struct MarkerInfo {
        std::string Name;
        std::array<MarkerLog, MaxBars> Logs;
        explicit MarkerInfo(std::string name) : Name(std::move(name)) {}
    };

    void CommandExecute(IDebugCommandHost& host, const std::vector<std::string>& arguments) {
        bool previousVisible = getVisibleProperty();

        if (arguments.empty())
            setVisibleProperty(!getVisibleProperty());

        for (const std::string& orgArg : arguments) {
            std::string arg = orgArg;
            std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);

            std::string sub0 = arg;
            std::string sub1;
            auto colon = arg.find(':');
            if (colon != std::string::npos) {
                sub0 = arg.substr(0, colon);
                sub1 = arg.substr(colon + 1);
            }

            if (sub0 == "on") {
                setVisibleProperty(true);
            } else if (sub0 == "off") {
                setVisibleProperty(false);
            } else if (sub0 == "reset") {
                ResetLog();
            } else if (sub0 == "log") {
                if (!sub1.empty()) {
                    if (sub1 == "on") ShowLog = true;
                    if (sub1 == "off") ShowLog = false;
                } else {
                    ShowLog = !ShowLog;
                }
            } else if (sub0 == "frame") {
                int a = std::max(1, std::min(MaxSampleFrames, std::stoi(sub1)));
                TargetSampleFrames = a;
            } else if (sub0 == "/?" || sub0 == "--help") {
                host.Echo("tr [log|on|off|reset|frame]");
                host.Echo("Options:");
                host.Echo("       on     Display TimeRuler.");
                host.Echo("       off    Hide TimeRuler.");
                host.Echo("       log    Show/Hide marker log.");
                host.Echo("       reset  Reset marker log.");
                host.Echo("       frame:sampleFrames");
                host.Echo("              Change target sample frame count");
            }
        }

        if (getVisibleProperty() != previousVisible)
            updateCount_ = 0;
    }

    int GetOrRegisterMarker(const std::string& markerName) {
        auto it = markerNameToIdMap_.find(markerName);
        if (it != markerNameToIdMap_.end())
            return it->second;

        int markerId = (int)markers_.size();
        markerNameToIdMap_.emplace(markerName, markerId);
        markers_.emplace_back(markerName);
        return markerId;
    }

    DebugManager* debugManager_ = nullptr;

    std::array<FrameLog, 2> logs_;
    FrameLog* prevLog_ = &logs_[0];
    FrameLog* curLog_ = &logs_[0];

    int frameCount_ = 0;
    System::Diagnostics::Stopwatch stopwatch_;

    std::vector<MarkerInfo> markers_;
    std::unordered_map<std::string, int> markerNameToIdMap_;

    int frameAdjust_ = 0;
    int sampleFrames_ = 1;

    int updateCount_ = 0;
};

} // namespace PerformanceMeasuring::GameDebugTools
