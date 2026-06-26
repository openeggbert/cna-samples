#pragma once
#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadCapabilities.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadDeadZone.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadType.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "ChargeSwitch.hpp"
#include "ChargeSwitchDeadZone.hpp"
#include "ChargeSwitchExit.hpp"
#include "InputReporterResources.hpp"

namespace InputReporter {

using V2 = Microsoft::Xna::Framework::Vector2;

class InputReporterGame : public Microsoft::Xna::Framework::Game {
    // --- Colors ---
    static const Microsoft::Xna::Framework::Color titleColor;
    static const Microsoft::Xna::Framework::Color typeColor;
    static const Microsoft::Xna::Framework::Color descriptionColor;
    static const Microsoft::Xna::Framework::Color valueColor;
    static const Microsoft::Xna::Framework::Color disabledColor;
    static const Microsoft::Xna::Framework::Color instructionsColor;

    // --- Graphics ---
    Microsoft::Xna::Framework::GraphicsDeviceManager graphics_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::SpriteBatch> spriteBatch_;
    std::optional<Microsoft::Xna::Framework::Graphics::SpriteFont> titleFont_;
    std::optional<Microsoft::Xna::Framework::Graphics::SpriteFont> dataFont_;
    std::optional<Microsoft::Xna::Framework::Graphics::SpriteFont> dataActiveFont_;
    std::optional<Microsoft::Xna::Framework::Graphics::SpriteFont> typeFont_;
    std::optional<Microsoft::Xna::Framework::Graphics::SpriteFont> instructionsFont_;
    std::optional<Microsoft::Xna::Framework::Graphics::SpriteFont> instructionsActiveFont_;
    Microsoft::Xna::Framework::Graphics::Texture2D backgroundTexture_;
    std::array<Microsoft::Xna::Framework::Graphics::Texture2D, 4> connectedTextures_;
    std::array<Microsoft::Xna::Framework::Graphics::Texture2D, 4> selectedTextures_;
    float dataSpacing_ = 0.0f;

    // --- Dead zone ---
    Microsoft::Xna::Framework::Input::GamePadDeadZone deadZone_
        = Microsoft::Xna::Framework::Input::GamePadDeadZone::IndependentAxes;
    std::string deadZoneString_;
    V2 deadZoneStringPos_;
    V2 deadZoneStringCenterPos_;

    // --- Input data ---
    int selectedPlayer_ = 0;
    std::array<Microsoft::Xna::Framework::Input::GamePadState, 4>        gamePadStates_;
    std::array<Microsoft::Xna::Framework::Input::GamePadCapabilities, 4> gamePadCaps_;
    Microsoft::Xna::Framework::Input::KeyboardState lastKeyboardState_;

    // --- Charge switches ---
    ChargeSwitchExit      exitSwitch_;
    ChargeSwitchDeadZone  deadZoneSwitch_;

public:
    InputReporterGame()
        : graphics_(this)
        , exitSwitch_(2.0f)
        , deadZoneSwitch_(2.0f)
    {
        graphics_.setPreferredBackBufferWidthProperty(853);
        graphics_.setPreferredBackBufferHeightProperty(480);
        getContentProperty().setRootDirectoryProperty("Content");

        exitSwitch_.Fire     = [this]() { Exit(); };
        deadZoneSwitch_.Fire = [this]() { ToggleDeadZone(); };
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "InputReporterGame";
        return name;
    }

protected:
    void Initialize() override {
        selectedPlayer_ = 0;
        exitSwitch_.Reset(2.0f);
        deadZoneSwitch_.Reset(2.0f);
        Game::Initialize();
        SetDeadZone(Microsoft::Xna::Framework::Input::GamePadDeadZone::IndependentAxes);
    }

    void LoadContent() override {
        using namespace Microsoft::Xna::Framework::Graphics;
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

        auto& content = getContentProperty();
        titleFont_.emplace(content.Load<SpriteFont>("Fonts/TitleFont"));
        dataFont_.emplace(content.Load<SpriteFont>("Fonts/DataFont"));
        dataActiveFont_.emplace(content.Load<SpriteFont>("Fonts/DataActiveFont"));
        typeFont_.emplace(content.Load<SpriteFont>("Fonts/TypeFont"));
        instructionsFont_.emplace(content.Load<SpriteFont>("Fonts/InstructionsFont"));
        instructionsActiveFont_.emplace(content.Load<SpriteFont>("Fonts/InstructionsActiveFont"));

        dataSpacing_ = std::floor((float)dataFont_->getLineSpacingProperty() * 1.3f);
        deadZoneStringCenterPos_ = V2(
            687.f,
            std::floor(380.f + (float)dataFont_->getLineSpacingProperty() * 1.7f));

        backgroundTexture_ = content.Load<Texture2D>("Textures/Background");
        for (int i = 0; i < 4; ++i) {
            connectedTextures_[i] = content.Load<Texture2D>(
                "Textures/connected_controller" + std::to_string(i + 1));
            selectedTextures_[i] = content.Load<Texture2D>(
                "Textures/select_controller" + std::to_string(i + 1));
        }
        SetDeadZone(deadZone_);
    }

    void Update(Microsoft::Xna::Framework::GameTime& gameTime) override {
        using namespace Microsoft::Xna::Framework::Input;

        KeyboardState kb = Keyboard::GetState();
        if (kb.IsKeyDown(Keys::Escape))
            Exit();
        if (kb.IsKeyDown(Keys::Space) && !lastKeyboardState_.IsKeyDown(Keys::Space))
            ToggleDeadZone();

        bool setSelected = false;
        for (int i = 0; i < 4; ++i) {
            gamePadStates_[i] = GamePad::GetState(
                static_cast<Microsoft::Xna::Framework::PlayerIndex>(i), deadZone_);
            gamePadCaps_[i] = GamePad::GetCapabilities(
                static_cast<Microsoft::Xna::Framework::PlayerIndex>(i));
            if (!setSelected && IsActiveGamePad(gamePadStates_[i])) {
                selectedPlayer_ = i;
                setSelected     = true;
            }
        }

        deadZoneSwitch_.Update(gameTime, gamePadStates_[selectedPlayer_]);
        exitSwitch_.Update(gameTime, gamePadStates_[selectedPlayer_]);

        Game::Update(gameTime);
        lastKeyboardState_ = kb;
    }

    void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) override {
        using namespace Microsoft::Xna::Framework;
        using namespace Microsoft::Xna::Framework::Graphics;
        using namespace Microsoft::Xna::Framework::Input;

        // Layout constants (match original pixel positions)
        static const float cx[] = { 606.f, 656.f, 606.f, 656.f };
        static const float cy[] = {  60.f,  60.f, 110.f, 110.f };
        static const float sx[] = { 594.f, 686.f, 594.f, 686.f };
        static const float sy[] = {  36.f,  36.f, 137.f, 137.f };
        static const V2 titlePos      { 180.f,  73.f };
        static const V2 typeCenterPos { 660.f, 270.f };
        static const V2 dzInstrPos    { 570.f, 380.f };
        static const V2 exitInstrPos  { 618.f, 425.f };

        getGraphicsDeviceProperty().Clear(Color::Black);
        Game::Draw(gameTime);

        spriteBatch_->Begin();

        spriteBatch_->Draw(backgroundTexture_, V2::Zero, Color::White);

        for (int i = 0; i < 4; ++i) {
            if (gamePadStates_[i].getIsConnectedProperty())
                spriteBatch_->Draw(connectedTextures_[i], V2(cx[i], cy[i]), Color::White);
        }
        spriteBatch_->Draw(selectedTextures_[selectedPlayer_],
            V2(sx[selectedPlayer_], sy[selectedPlayer_]), Color::White);

        std::string titleText = std::string(InputReporterResources::Title)
                              + PlayerIndexName(static_cast<PlayerIndex>(selectedPlayer_));
        spriteBatch_->DrawString(*titleFont_, titleText, titlePos, titleColor);

        std::string typeText = GamePadTypeName(gamePadCaps_[selectedPlayer_].getGamePadTypeProperty());
        V2 typeSize = typeFont_->MeasureString(typeText);
        spriteBatch_->DrawString(*typeFont_, typeText,
            V2(std::floor(typeCenterPos.X - typeSize.X / 2.f),
               std::floor(typeCenterPos.Y - typeSize.Y / 2.f)),
            typeColor);

        DrawData(gamePadStates_[selectedPlayer_], gamePadCaps_[selectedPlayer_]);

        auto& dzFont = deadZoneSwitch_.Active() ? instructionsActiveFont_ : instructionsFont_;
        spriteBatch_->DrawString(*dzFont,
            InputReporterResources::DeadZoneInstructions, dzInstrPos, instructionsColor);
        spriteBatch_->DrawString(*instructionsFont_,
            deadZoneString_, deadZoneStringPos_, instructionsColor);

        auto& exFont = exitSwitch_.Active() ? instructionsActiveFont_ : instructionsFont_;
        spriteBatch_->DrawString(*exFont,
            InputReporterResources::ExitInstructions, exitInstrPos, instructionsColor);

        spriteBatch_->End();
    }

private:
    void SetDeadZone(Microsoft::Xna::Framework::Input::GamePadDeadZone dz) {
        deadZone_       = dz;
        deadZoneString_ = std::string("(") + DeadZoneName(dz) + ")";
        if (dataFont_) {
            V2 sz = dataFont_->MeasureString(deadZoneString_);
            deadZoneStringPos_ = V2(
                std::floor(deadZoneStringCenterPos_.X - sz.X / 2.f),
                std::floor(deadZoneStringCenterPos_.Y - sz.Y / 2.f));
        }
    }

    void ToggleDeadZone() {
        using namespace Microsoft::Xna::Framework::Input;
        switch (deadZone_) {
            case GamePadDeadZone::IndependentAxes: SetDeadZone(GamePadDeadZone::Circular);        break;
            case GamePadDeadZone::Circular:        SetDeadZone(GamePadDeadZone::None);            break;
            case GamePadDeadZone::None:            SetDeadZone(GamePadDeadZone::IndependentAxes); break;
        }
    }

    static bool IsActiveGamePad(const Microsoft::Xna::Framework::Input::GamePadState& s) {
        using namespace Microsoft::Xna::Framework::Input;
        if (!s.getIsConnectedProperty()) return false;
        const auto& btn = s.getButtonsProperty();
        const auto& dp  = s.getDPadProperty();
        return btn.getAProperty()             == ButtonState::Pressed ||
               btn.getBProperty()             == ButtonState::Pressed ||
               btn.getXProperty()             == ButtonState::Pressed ||
               btn.getYProperty()             == ButtonState::Pressed ||
               btn.getStartProperty()         == ButtonState::Pressed ||
               btn.getBackProperty()          == ButtonState::Pressed ||
               btn.getLeftShoulderProperty()  == ButtonState::Pressed ||
               btn.getRightShoulderProperty() == ButtonState::Pressed ||
               btn.getLeftStickProperty()     == ButtonState::Pressed ||
               btn.getRightStickProperty()    == ButtonState::Pressed ||
               dp.getUpProperty()             == ButtonState::Pressed ||
               dp.getLeftProperty()           == ButtonState::Pressed ||
               dp.getRightProperty()          == ButtonState::Pressed ||
               dp.getDownProperty()           == ButtonState::Pressed;
    }

    void DrawData(const Microsoft::Xna::Framework::Input::GamePadState& s,
                  const Microsoft::Xna::Framework::Input::GamePadCapabilities& cap) {
        using namespace Microsoft::Xna::Framework;
        using namespace Microsoft::Xna::Framework::Input;

        static const V2 dc1 {  65.f, 135.f };
        static const V2 vc1 { 220.f, 135.f };
        static const V2 dc2 { 310.f, 135.f };
        static const V2 vc2 { 472.f, 135.f };

        V2 descPos = dc1;
        V2 valPos  = vc1;

        const auto& sticks   = s.getThumbSticksProperty();
        const auto& triggers = s.getTriggersProperty();
        const auto& dpad     = s.getDPadProperty();
        const auto& buttons  = s.getButtonsProperty();

        DrawValue(InputReporterResources::LeftThumbstickX, descPos,
                  Fmt3(sticks.getLeftProperty().X), valPos,
                  cap.HasLeftXThumbStick, sticks.getLeftProperty().X != 0.f);
        DrawValue(InputReporterResources::LeftThumbstickY, descPos,
                  Fmt3(sticks.getLeftProperty().Y), valPos,
                  cap.HasLeftYThumbStick, sticks.getLeftProperty().Y != 0.f);
        DrawValue(InputReporterResources::RightThumbstickX, descPos,
                  Fmt3(sticks.getRightProperty().X), valPos,
                  cap.HasRightXThumbStick, sticks.getRightProperty().X != 0.f);
        DrawValue(InputReporterResources::RightThumbstickY, descPos,
                  Fmt3(sticks.getRightProperty().Y), valPos,
                  cap.HasRightYThumbStick, sticks.getRightProperty().Y != 0.f);

        descPos.Y = descPos.Y + dataSpacing_;
        valPos.Y  = valPos.Y  + dataSpacing_;

        DrawValue(InputReporterResources::LeftTrigger, descPos,
                  Fmt3(triggers.getLeftProperty()), valPos,
                  cap.HasLeftTrigger, triggers.getLeftProperty() != 0.f);
        DrawValue(InputReporterResources::RightTrigger, descPos,
                  Fmt3(triggers.getRightProperty()), valPos,
                  cap.HasRightTrigger, triggers.getRightProperty() != 0.f);

        descPos.Y = descPos.Y + dataSpacing_;
        valPos.Y  = valPos.Y  + dataSpacing_;

        DrawValue(InputReporterResources::DPadUp, descPos,
                  BtnStr(dpad.getUpProperty()), valPos,
                  cap.HasDPadUpButton, dpad.getUpProperty() == ButtonState::Pressed);
        DrawValue(InputReporterResources::DPadDown, descPos,
                  BtnStr(dpad.getDownProperty()), valPos,
                  cap.HasDPadDownButton, dpad.getDownProperty() == ButtonState::Pressed);
        DrawValue(InputReporterResources::DPadLeft, descPos,
                  BtnStr(dpad.getLeftProperty()), valPos,
                  cap.HasDPadLeftButton, dpad.getLeftProperty() == ButtonState::Pressed);
        DrawValue(InputReporterResources::DPadRight, descPos,
                  BtnStr(dpad.getRightProperty()), valPos,
                  cap.HasDPadRightButton, dpad.getRightProperty() == ButtonState::Pressed);

        descPos.Y = descPos.Y + dataSpacing_;

        if (cap.HasLeftVibrationMotor) {
            const char* msg = cap.HasRightVibrationMotor
                ? InputReporterResources::BothVibrationMotors
                : InputReporterResources::LeftVibrationMotor;
            spriteBatch_->DrawString(*dataFont_, msg, descPos, descriptionColor);
        } else if (cap.HasRightVibrationMotor) {
            spriteBatch_->DrawString(*dataFont_,
                InputReporterResources::RightVibrationMotor, descPos, descriptionColor);
        } else {
            spriteBatch_->DrawString(*dataFont_,
                InputReporterResources::NoVibration, descPos, descriptionColor);
        }

        // Column 2
        descPos = dc2;
        valPos  = vc2;

        DrawValue(InputReporterResources::A, descPos,
                  BtnStr(buttons.getAProperty()), valPos,
                  cap.HasAButton, buttons.getAProperty() == ButtonState::Pressed);
        DrawValue(InputReporterResources::B, descPos,
                  BtnStr(buttons.getBProperty()), valPos,
                  cap.HasBButton, buttons.getBProperty() == ButtonState::Pressed);
        DrawValue(InputReporterResources::X, descPos,
                  BtnStr(buttons.getXProperty()), valPos,
                  cap.HasXButton, buttons.getXProperty() == ButtonState::Pressed);
        DrawValue(InputReporterResources::Y, descPos,
                  BtnStr(buttons.getYProperty()), valPos,
                  cap.HasYButton, buttons.getYProperty() == ButtonState::Pressed);
        DrawValue(InputReporterResources::LeftShoulder, descPos,
                  BtnStr(buttons.getLeftShoulderProperty()), valPos,
                  cap.HasLeftShoulderButton,
                  buttons.getLeftShoulderProperty() == ButtonState::Pressed);
        DrawValue(InputReporterResources::RightShoulder, descPos,
                  BtnStr(buttons.getRightShoulderProperty()), valPos,
                  cap.HasRightShoulderButton,
                  buttons.getRightShoulderProperty() == ButtonState::Pressed);
        DrawValue(InputReporterResources::LeftStick, descPos,
                  BtnStr(buttons.getLeftStickProperty()), valPos,
                  cap.HasLeftStickButton,
                  buttons.getLeftStickProperty() == ButtonState::Pressed);
        DrawValue(InputReporterResources::RightStick, descPos,
                  BtnStr(buttons.getRightStickProperty()), valPos,
                  cap.HasRightStickButton,
                  buttons.getRightStickProperty() == ButtonState::Pressed);
        DrawValue(InputReporterResources::Start, descPos,
                  BtnStr(buttons.getStartProperty()), valPos,
                  cap.HasStartButton, buttons.getStartProperty() == ButtonState::Pressed);
        DrawValue(InputReporterResources::Back, descPos,
                  BtnStr(buttons.getBackProperty()), valPos,
                  cap.HasBackButton, buttons.getBackProperty() == ButtonState::Pressed);

        descPos.Y = descPos.Y + dataSpacing_;
        valPos.Y  = valPos.Y  + dataSpacing_;

        DrawValue(InputReporterResources::PacketNumber, descPos,
                  std::to_string(s.getPacketNumberProperty()), valPos,
                  cap.IsConnected, false);
    }

    void DrawValue(const std::string& description, V2& descPos,
                   const std::string& value,       V2& valPos,
                   bool enabled, bool active) {
        using namespace Microsoft::Xna::Framework;
        spriteBatch_->DrawString(*dataFont_, description, descPos,
                                 enabled ? descriptionColor : disabledColor);
        descPos.Y = descPos.Y + dataSpacing_;
        auto& vFont = active ? dataActiveFont_ : dataFont_;
        spriteBatch_->DrawString(*vFont, value, valPos,
                                 enabled ? valueColor : disabledColor);
        valPos.Y = valPos.Y + dataSpacing_;
    }

    // --- String helpers ---
    static const char* BtnStr(Microsoft::Xna::Framework::Input::ButtonState bs) {
        using Microsoft::Xna::Framework::Input::ButtonState;
        return bs == ButtonState::Pressed ? InputReporterResources::ButtonPressed
                                          : InputReporterResources::ButtonReleased;
    }

    static std::string Fmt3(float v) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.3f", v);
        return buf;
    }

    static const char* PlayerIndexName(Microsoft::Xna::Framework::PlayerIndex pi) {
        using Microsoft::Xna::Framework::PlayerIndex;
        switch (pi) {
            case PlayerIndex::One:   return "One";
            case PlayerIndex::Two:   return "Two";
            case PlayerIndex::Three: return "Three";
            case PlayerIndex::Four:  return "Four";
            default:                 return "Unknown";
        }
    }

    static const char* DeadZoneName(Microsoft::Xna::Framework::Input::GamePadDeadZone dz) {
        using Microsoft::Xna::Framework::Input::GamePadDeadZone;
        switch (dz) {
            case GamePadDeadZone::None:            return "None";
            case GamePadDeadZone::IndependentAxes: return "IndependentAxes";
            case GamePadDeadZone::Circular:        return "Circular";
            default:                               return "Unknown";
        }
    }

    static const char* GamePadTypeName(Microsoft::Xna::Framework::Input::GamePadType t) {
        using Microsoft::Xna::Framework::Input::GamePadType;
        switch (t) {
            case GamePadType::Unknown:         return "Unknown";
            case GamePadType::GamePad:         return "GamePad";
            case GamePadType::Wheel:           return "Wheel";
            case GamePadType::ArcadeStick:     return "ArcadeStick";
            case GamePadType::FlightStick:     return "FlightStick";
            case GamePadType::DancePad:        return "DancePad";
            case GamePadType::Guitar:          return "Guitar";
            case GamePadType::AlternateGuitar: return "AlternateGuitar";
            case GamePadType::DrumKit:         return "DrumKit";
            case GamePadType::BigButtonPad:    return "BigButtonPad";
            default:                           return "Unknown";
        }
    }
};

inline const Microsoft::Xna::Framework::Color InputReporterGame::titleColor       { 60, 134,  11, 255};
inline const Microsoft::Xna::Framework::Color InputReporterGame::typeColor        { 38, 108,  87, 255};
inline const Microsoft::Xna::Framework::Color InputReporterGame::descriptionColor { 33,  89,  15, 255};
inline const Microsoft::Xna::Framework::Color InputReporterGame::valueColor       { 38, 108,  87, 255};
inline const Microsoft::Xna::Framework::Color InputReporterGame::disabledColor    {171, 171, 171, 255};
inline const Microsoft::Xna::Framework::Color InputReporterGame::instructionsColor{127, 130, 127, 255};

} // namespace InputReporter
