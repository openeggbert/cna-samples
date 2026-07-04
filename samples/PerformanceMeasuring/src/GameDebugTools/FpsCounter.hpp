#pragma once

// FpsCounter.hpp — C++ port of GameDebugTools/FpsCounter.cs (XNA 4.0
// PerformanceMeasuring sample). Component for FPS measurement and display.

#include <cstdio>
#include <stdexcept>
#include <string>

#include "System/Diagnostics/Stopwatch.hpp"
#include "System/TimeSpan.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
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

// Component for FPS measurement and drawing. Port of GameDebugTools/FpsCounter.cs.
class FpsCounter : public DrawableGameComponent {
public:
    float Fps = 0.0f;
    System::TimeSpan SampleSpan = System::TimeSpan::FromSeconds(1.0);

    explicit FpsCounter(Game& game) : DrawableGameComponent(game) {}

    void Initialize() override {
        debugManager_ = getGameProperty().getServicesProperty().GetService<DebugManager>();
        if (debugManager_ == nullptr)
            throw std::runtime_error("DebugManager is not registered.");

        // Register the 'fps' command if a debug command host is registered.
        auto* host = getGameProperty().getServicesProperty().GetService<IDebugCommandHost>();
        if (host != nullptr) {
            host->RegisterCommand("fps", "FPS Counter",
                [this](IDebugCommandHost&, const std::string&, const std::vector<std::string>& args) {
                    CommandExecute(args);
                });
            setVisibleProperty(true);
        }

        Fps = 0.0f;
        sampleFrames_ = 0;
        stopwatch_ = System::Diagnostics::Stopwatch::StartNew();

        DrawableGameComponent::Initialize();
    }

    void Update(GameTime&) override {
        if (stopwatch_.getElapsedProperty() > SampleSpan) {
            Fps = (float)sampleFrames_ / (float)stopwatch_.getElapsedProperty().getTotalSecondsProperty();

            stopwatch_.Reset();
            stopwatch_.Start();
            sampleFrames_ = 0;

            char buf[32];
            std::snprintf(buf, sizeof(buf), "FPS: %.2f", Fps);
            text_ = buf;
        }
    }

    void Draw(const GameTime&) override {
        sampleFrames_++;

        SpriteBatch& spriteBatch = debugManager_->getSpriteBatch();
        SpriteFont& font = debugManager_->getDebugFont();

        Vector2 size = font.MeasureString("X");
        Rectangle rc(0, 0, (int)(size.X * 14.0f), (int)(size.Y * 1.3f));

        Layout layout(getGraphicsDeviceProperty().getViewportProperty());
        rc = layout.Place(rc, 0.01f, 0.01f, Alignment::TopLeft);

        size = font.MeasureString(text_);
        layout.ClientArea = rc;
        Vector2 pos = layout.Place(size, 0.0f, 0.1f, Alignment::Center);

        spriteBatch.Begin();
        spriteBatch.Draw(debugManager_->getWhiteTexture(), rc, Color(0, 0, 0, 128));
        spriteBatch.DrawString(font, text_, pos, Color::White);
        spriteBatch.End();
    }

private:
    void CommandExecute(const std::vector<std::string>& args) {
        if (args.empty())
            setVisibleProperty(!getVisibleProperty());

        for (const std::string& arg : args) {
            if (arg == "on" || arg == "On" || arg == "ON")
                setVisibleProperty(true);
            else if (arg == "off" || arg == "Off" || arg == "OFF")
                setVisibleProperty(false);
        }
    }

    DebugManager* debugManager_ = nullptr;
    System::Diagnostics::Stopwatch stopwatch_;
    int sampleFrames_ = 0;
    std::string text_ = "FPS: 0.00";
};

} // namespace PerformanceMeasuring::GameDebugTools
