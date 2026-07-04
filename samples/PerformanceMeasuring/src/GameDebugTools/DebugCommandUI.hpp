#pragma once

// DebugCommandUI.hpp — C++ port of GameDebugTools/DebugCommandUI.cs (XNA 4.0
// PerformanceMeasuring sample). An in-game command console: type commands with
// the keyboard, toggle open/closed with Tab.
//
// Adaptation note: RemoteDebugCommand.cs (the sibling file that lets a Windows
// PC drive this console over an Xbox 360 SystemLink connection) is not ported
// — it depends on Microsoft.Xna.Framework.Net/GamerServices, which have no CNA
// equivalent and no meaning on desktop. See missing.md.

#include <algorithm>
#include <cctype>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteSortMode.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"

#include "DebugManager.hpp"
#include "IDebugCommandHost.hpp"
#include "KeyboardUtils.hpp"

namespace PerformanceMeasuring::GameDebugTools {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Matrix;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::SpriteSortMode;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;

// Command window for debug purposes. Type commands with the keyboard; press
// Tab to open/close. Port of GameDebugTools/DebugCommandUI.cs.
class DebugCommandUI : public DrawableGameComponent, public IDebugCommandHost {
public:
    static constexpr const char* DefaultPrompt = "CMD>";

    std::string Prompt = DefaultPrompt;

    bool Focused() const { return state_ != State::Closed; }

    explicit DebugCommandUI(Game& game) : DrawableGameComponent(game) {
        game.getServicesProperty().AddService<IDebugCommandHost>(this);

        // Draw the command UI on top of everything.
        setDrawOrderProperty(0x7fffffff);

        RegisterCommand("help", "Show Command helps",
            [this](IDebugCommandHost&, const std::string&, const std::vector<std::string>&) {
                size_t maxLen = 0;
                for (auto& [name, info] : commandTable_)
                    maxLen = std::max(maxLen, info.command.size());

                for (auto& [name, info] : commandTable_) {
                    std::string line = info.command;
                    line.resize(maxLen, ' ');
                    Echo(line + "    " + info.description);
                }
            });

        RegisterCommand("cls", "Clear Screen",
            [this](IDebugCommandHost&, const std::string&, const std::vector<std::string>&) {
                lines_.clear();
            });

        RegisterCommand("echo", "Display Messages",
            [this](IDebugCommandHost&, const std::string& command, const std::vector<std::string>&) {
                Echo(command.size() > 5 ? command.substr(5) : std::string());
            });
    }

    void Initialize() override {
        debugManager_ = getGameProperty().getServicesProperty().GetService<DebugManager>();
        if (debugManager_ == nullptr)
            throw std::runtime_error("Couldn't find DebugManager.");

        DrawableGameComponent::Initialize();
    }

    // ---- IDebugCommandHost ----

    void RegisterCommand(const std::string& command, const std::string& description,
                          DebugCommandExecute callback) override {
        std::string lower = ToLower(command);
        if (commandTable_.find(lower) != commandTable_.end())
            throw std::runtime_error("Command \"" + command + "\" is already registered.");

        commandTable_.emplace(lower, CommandInfo{command, description, std::move(callback)});
    }

    void UnregisterCommand(const std::string& command) override {
        std::string lower = ToLower(command);
        auto it = commandTable_.find(lower);
        if (it == commandTable_.end())
            throw std::runtime_error("Command \"" + command + "\" is not registered.");
        commandTable_.erase(it);
    }

    void ExecuteCommand(const std::string& commandIn) override {
        if (!executioners_.empty()) {
            executioners_.back()->ExecuteCommand(commandIn);
            return;
        }

        Echo(Prompt + commandIn);

        std::string command = commandIn;
        size_t start = command.find_first_not_of(' ');
        command = (start == std::string::npos) ? std::string() : command.substr(start);

        std::vector<std::string> args;
        {
            size_t pos = 0;
            while (pos <= command.size()) {
                size_t next = command.find(' ', pos);
                if (next == std::string::npos) {
                    args.push_back(command.substr(pos));
                    break;
                }
                args.push_back(command.substr(pos, next - pos));
                pos = next + 1;
            }
        }
        std::string cmdText = args.empty() ? std::string() : args.front();
        if (!args.empty())
            args.erase(args.begin());

        auto it = commandTable_.find(ToLower(cmdText));
        if (it != commandTable_.end()) {
            try {
                it->second.callback(*this, command, args);
            } catch (const std::exception& e) {
                EchoError("Unhandled Exception occurred");
                EchoError(e.what());
            }
        } else {
            Echo("Unknown Command");
        }

        commandHistory_.push_back(command);
        while (commandHistory_.size() > MaxCommandHistory)
            commandHistory_.erase(commandHistory_.begin());

        commandHistoryIndex_ = (int)commandHistory_.size();
    }

    void RegisterEchoListener(IDebugEchoListener* listener) override { listeners_.push_back(listener); }

    void UnregisterEchoListener(IDebugEchoListener* listener) override {
        listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end());
    }

    void Echo(DebugCommandMessage messageType, const std::string& text) override {
        lines_.push_back(text);
        while (lines_.size() >= MaxLineCount)
            lines_.pop_front();

        for (IDebugEchoListener* listener : listeners_)
            listener->Echo(messageType, text);
    }

    void Echo(const std::string& text) override { Echo(DebugCommandMessage::Standard, text); }
    void EchoWarning(const std::string& text) override { Echo(DebugCommandMessage::Warning, text); }
    void EchoError(const std::string& text) override { Echo(DebugCommandMessage::Error, text); }

    void PushExecutioner(IDebugCommandExecutioner* executioner) override { executioners_.push_back(executioner); }
    void PopExecutioner() override { executioners_.pop_back(); }

    // ---- Update and Draw ----

    void Show() {
        if (state_ == State::Closed) {
            stateTransition_ = 0.0f;
            state_ = State::Opening;
        }
    }

    void Hide() {
        if (state_ == State::Opened) {
            stateTransition_ = 1.0f;
            state_ = State::Closing;
        }
    }

    void Update(GameTime& gameTime) override {
        KeyboardState keyState = Keyboard::GetState();

        float dt = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        const float OpenSpeed = 8.0f;
        const float CloseSpeed = 8.0f;

        switch (state_) {
            case State::Closed:
                if (keyState.IsKeyDown(Keys::Tab))
                    Show();
                break;
            case State::Opening:
                stateTransition_ += dt * OpenSpeed;
                if (stateTransition_ > 1.0f) {
                    stateTransition_ = 1.0f;
                    state_ = State::Opened;
                }
                break;
            case State::Opened:
                ProcessKeyInputs(dt);
                break;
            case State::Closing:
                stateTransition_ -= dt * CloseSpeed;
                if (stateTransition_ < 0.0f) {
                    stateTransition_ = 0.0f;
                    state_ = State::Closed;
                }
                break;
        }

        prevKeyState_ = keyState;

        DrawableGameComponent::Update(gameTime);
    }

    void ProcessKeyInputs(float dt) {
        KeyboardState keyState = Keyboard::GetState();
        std::vector<Keys> keys = keyState.GetPressedKeys();

        bool shift = keyState.IsKeyDown(Keys::LeftShift) || keyState.IsKeyDown(Keys::RightShift);

        for (Keys key : keys) {
            if (!IsKeyPressed(key, dt)) continue;

            char ch;
            if (KeyboardUtils::KeyToString(key, shift, ch)) {
                commandLine_.insert((size_t)cursorIndex_, 1, ch);
                cursorIndex_++;
            } else {
                switch (key) {
                    case Keys::Back:
                        if (cursorIndex_ > 0)
                            commandLine_.erase((size_t)--cursorIndex_, 1);
                        break;
                    case Keys::Delete:
                        if (cursorIndex_ < (int)commandLine_.size())
                            commandLine_.erase((size_t)cursorIndex_, 1);
                        break;
                    case Keys::Left:
                        if (cursorIndex_ > 0)
                            cursorIndex_--;
                        break;
                    case Keys::Right:
                        if (cursorIndex_ < (int)commandLine_.size())
                            cursorIndex_++;
                        break;
                    case Keys::Enter:
                        ExecuteCommand(commandLine_);
                        commandLine_.clear();
                        cursorIndex_ = 0;
                        break;
                    case Keys::Up:
                        if (!commandHistory_.empty()) {
                            commandHistoryIndex_ = std::max(0, commandHistoryIndex_ - 1);
                            commandLine_ = commandHistory_[(size_t)commandHistoryIndex_];
                            cursorIndex_ = (int)commandLine_.size();
                        }
                        break;
                    case Keys::Down:
                        if (!commandHistory_.empty()) {
                            commandHistoryIndex_ =
                                std::min((int)commandHistory_.size() - 1, commandHistoryIndex_ + 1);
                            commandLine_ = commandHistory_[(size_t)commandHistoryIndex_];
                            cursorIndex_ = (int)commandLine_.size();
                        }
                        break;
                    case Keys::Tab:
                        Hide();
                        break;
                    default:
                        break;
                }
            }
        }
    }

    void Draw(const GameTime&) override {
        if (state_ == State::Closed)
            return;

        SpriteFont& font = debugManager_->getDebugFont();
        SpriteBatch& spriteBatch = debugManager_->getSpriteBatch();

        float w = (float)getGraphicsDeviceProperty().getViewportProperty().getWidthProperty();
        float h = (float)getGraphicsDeviceProperty().getViewportProperty().getHeightProperty();
        float topMargin = h * 0.1f;
        float leftMargin = w * 0.1f;

        Rectangle rect;
        rect.X = (int)leftMargin;
        rect.Y = (int)topMargin;
        rect.Width = (int)(w * 0.8f);
        rect.Height = (int)((float)MaxLineCount * (float)font.getLineSpacingProperty());

        Matrix mtx = Matrix::CreateTranslation(Vector3(0.0f, -(float)rect.Height * (1.0f - stateTransition_), 0.0f));

        spriteBatch.Begin(SpriteSortMode::Deferred, BlendState_(), nullptr, nullptr, nullptr, nullptr, mtx);

        spriteBatch.Draw(debugManager_->getWhiteTexture(), rect, Color(0, 0, 0, 200));

        Vector2 pos(leftMargin, topMargin);
        for (const std::string& line : lines_) {
            spriteBatch.DrawString(font, line, pos, Color::White);
            pos.Y += (float)font.getLineSpacingProperty();
        }

        std::string leftPart = Prompt + commandLine_.substr(0, (size_t)cursorIndex_);
        Vector2 cursorPos = pos + font.MeasureString(leftPart);
        cursorPos.Y = pos.Y;

        spriteBatch.DrawString(font, Prompt + commandLine_, pos, Color::White);
        spriteBatch.DrawString(font, Cursor, cursorPos, Color::White);

        spriteBatch.End();
    }

private:
    static constexpr size_t MaxLineCount = 20;
    static constexpr size_t MaxCommandHistory = 32;
    static constexpr const char* Cursor = "_";

    enum class State { Closed, Opening, Opened, Closing };

    struct CommandInfo {
        std::string command;
        std::string description;
        DebugCommandExecute callback;
    };

    static std::string ToLower(const std::string& s) {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        return r;
    }

    // Default BlendState (AlphaBlend), matching the original passing null for
    // blendState in its Begin(SpriteSortMode, null, null, null, null, null, mtx) call.
    static Microsoft::Xna::Framework::Graphics::BlendState BlendState_() {
        return Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend;
    }

    bool IsKeyPressed(Keys key, float dt) {
        if (prevKeyState_.IsKeyUp(key)) {
            keyRepeatTimer_ = keyRepeatStartDuration_;
            pressedKey_ = key;
            return true;
        }

        if (key == pressedKey_) {
            keyRepeatTimer_ -= dt;
            if (keyRepeatTimer_ <= 0.0f) {
                keyRepeatTimer_ += keyRepeatDuration_;
                return true;
            }
        }

        return false;
    }

    DebugManager* debugManager_ = nullptr;

    State state_ = State::Closed;
    float stateTransition_ = 0.0f;

    std::vector<IDebugEchoListener*> listeners_;
    std::vector<IDebugCommandExecutioner*> executioners_;
    std::unordered_map<std::string, CommandInfo> commandTable_;

    std::string commandLine_;
    int cursorIndex_ = 0;

    std::deque<std::string> lines_;
    std::vector<std::string> commandHistory_;
    int commandHistoryIndex_ = 0;

    KeyboardState prevKeyState_;
    Keys pressedKey_ = Keys::None;
    float keyRepeatTimer_ = 0.0f;
    float keyRepeatStartDuration_ = 0.3f;
    float keyRepeatDuration_ = 0.03f;
};

} // namespace PerformanceMeasuring::GameDebugTools
