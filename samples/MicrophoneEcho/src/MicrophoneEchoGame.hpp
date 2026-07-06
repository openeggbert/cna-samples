#pragma once

#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteFont.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp>
#include <Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/GamePad.hpp>
#include <Microsoft/Xna/Framework/Input/GamePadState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <Microsoft/Xna/Framework/Input/Buttons.hpp>
#include <Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp>
#include <Microsoft/Xna/Framework/Input/Touch/GestureType.hpp>
#include <Microsoft/Xna/Framework/Input/Touch/GestureSample.hpp>
#include <Microsoft/Xna/Framework/PlayerIndex.hpp>
#include <Microsoft/Xna/Framework/Audio/Microphone.hpp>
#include <Microsoft/Xna/Framework/Audio/MicrophoneState.hpp>
#include <Microsoft/Xna/Framework/Audio/NoMicrophoneConnectedException.hpp>
#include <Microsoft/Xna/Framework/Audio/DynamicSoundEffectInstance.hpp>
#include <Microsoft/Xna/Framework/Audio/AudioChannels.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <System/TimeSpan.hpp>
#include <System/BitConverter.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;
using namespace Microsoft::Xna::Framework::Input::Touch;
using namespace Microsoft::Xna::Framework::Audio;

namespace MicrophoneEcho {

// Follow these instructions (desktop branch of the original's #if WINDOWS_PHONE / #else split).
static const char* kInstructions = "Press 'A' to start and 'B' to stop recording";

// Echo processing constants.
static constexpr float kEchoDelay  = 0.15f; // Delay applied in seconds.
static constexpr float kEchoAmount = 0.5f;  // Rate of echo decay.

class MicrophoneEchoGame : public Game {
public:
    MicrophoneEchoGame() {
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        graphics_->setPreferredBackBufferWidthProperty(800);
        graphics_->setPreferredBackBufferHeightProperty(480);
        graphics_->setSupportedOrientationsProperty(DisplayOrientation::LandscapeLeft);

        TouchPanel::setEnabledGesturesProperty(GestureType::Tap | GestureType::DoubleTap);
        setIsMouseVisibleProperty(true);
        getContentProperty().setRootDirectoryProperty("Content");
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "MicrophoneEchoGame";
        return name;
    }

protected:
    void LoadContent() override {
        spriteBatch_.emplace(getGraphicsDeviceProperty());
        font_.emplace(getContentProperty().Load<SpriteFont>("font"));
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
        InitializeMicrophone();
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        // F1 help overlay
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        // Picks a microphone to start recording - if one isn't picked already.
        InitializeMicrophone();
        // Handle input to start/stop recording.
        HandleInput();
        // Check and update microphone status.
        UpdateMicrophoneStatus();

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255)); // CornflowerBlue
        DrawWaveform();
        Game::Draw(gameTime);
    }

private:
    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::optional<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont>  font_;

    // The most recent microphone samples.
    std::vector<SharpRuntime::bytecs> micSamples_;
    // A circular buffer that we feedback into from micSamples_.
    std::vector<SharpRuntime::bytecs> echoBuffer_;
    // Tracks the position into the echo buffer.
    int echoBufferPosition_ = 0;
    // Used to playback the captured audio after processing it for echo.
    std::unique_ptr<DynamicSoundEffectInstance> dynamicSound_;
    // Microphone used for recording (owned by CNA's Microphone::getAllProperty() storage).
    Microphone* activeMicrophone_ = nullptr;

    KeyboardState currentKeyboardState_;
    KeyboardState previousKeyboardState_;
    GamePadState  currentGamePadState_;
    GamePadState  previousGamePadState_;

    // Used to communicate the microphone status to the user.
    std::string microphoneStatus_;

    // On big endian systems audio samples need to be swapped because
    // the byte buffer is always written/read little-endian here.
    bool bigEndian_ = !System::BitConverter::IsLittleEndian;

    // Used for drawing the audio waveform.
    std::optional<BasicEffect> effect_;
    std::vector<VertexPositionColor> vertexPosColor_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

    // Handles input for starting and stopping the recording.
    void HandleInput() {
        // Allows the game to exit.
        if (GamePad::GetState(PlayerIndex::One).getButtonsProperty().getBackProperty() == ButtonState::Pressed ||
            Keyboard::GetState().IsKeyDown(Keys::Escape)) {
            Exit();
            return;
        }

        if (TouchPanel::getIsGestureAvailableProperty()) {
            GestureSample gesture = TouchPanel::ReadGesture();
            if (gesture.getGestureTypeProperty() == GestureType::Tap) {
                StartMicrophone();
            } else if (gesture.getGestureTypeProperty() == GestureType::DoubleTap) {
                StopMicrophone();
            }
        } else {
            previousGamePadState_ = currentGamePadState_;
            previousKeyboardState_ = currentKeyboardState_;

            currentGamePadState_ = GamePad::GetState(PlayerIndex::One);
            currentKeyboardState_ = Keyboard::GetState();

            if ((currentGamePadState_.IsButtonDown(Buttons::A) && previousGamePadState_.IsButtonUp(Buttons::A)) ||
                (currentKeyboardState_.IsKeyDown(Keys::A) && previousKeyboardState_.IsKeyUp(Keys::A))) {
                StartMicrophone();
            }

            if ((currentGamePadState_.IsButtonDown(Buttons::B) && previousGamePadState_.IsButtonUp(Buttons::B)) ||
                (currentKeyboardState_.IsKeyDown(Keys::B) && previousKeyboardState_.IsKeyUp(Keys::B))) {
                StopMicrophone();
            }
        }
    }

    // Draws the audio waveform being played back.
    void DrawWaveform() {
        spriteBatch_->Begin(SpriteSortMode::Deferred, BlendState::AlphaBlend);
        spriteBatch_->DrawString(*font_, kInstructions, Vector2(10.0f, 20.0f), Color(255, 255, 255, 255));
        spriteBatch_->DrawString(*font_, microphoneStatus_, Vector2(10.0f, 50.0f), Color(255, 255, 255, 255));
        if (!echoBuffer_.empty()) {
            int sampleCount = (int)echoBuffer_.size() / (int)sizeof(int16_t);
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            for (int index = 0; index < (int)echoBuffer_.size(); index += (int)sizeof(int16_t)) {
                int sampleIndex = index / (int)sizeof(int16_t);
                vertexPosColor_[sampleIndex].Position.X =
                    sampleIndex * ((float)vp.getWidthProperty() / (float)sampleCount);
                vertexPosColor_[sampleIndex].Position.Y =
                    (vp.getHeightProperty() / 2) -
                    ((float)ReadSample(echoBuffer_, index) / 32767.0f * (vp.getHeightProperty() / 2));
            }
            effect_->getCurrentTechniqueProperty()->getPassesProperty()[0].Apply();
            getGraphicsDeviceProperty().DrawUserPrimitives(PrimitiveType::LineStrip,
                vertexPosColor_.data(), 0, (int)vertexPosColor_.size() - 1);
        }
        DrawHelpOverlay();
        spriteBatch_->End();
    }

    // Finds a good microphone to use and sets up everything to start recording and playback.
    // Once a microphone is selected the game uses it throughout its lifetime. If it gets
    // disconnected it will tell the user to reconnect it.
    void InitializeMicrophone() {
        // We already have a microphone, skip out early.
        if (activeMicrophone_ != nullptr) return;

        try {
            // Find the first microphone that's ready to rock.
            activeMicrophone_ = PickFirstConnectedMicrophone();
            if (activeMicrophone_ != nullptr) {
                // Set the capture buffer size for low latency. Microphone will call the game
                // back when it has captured at least that much audio data.
                activeMicrophone_->setBufferDurationProperty(System::TimeSpan::FromMilliseconds(100));
                // Subscribe to the event that's raised when the capture buffer is filled.
                activeMicrophone_->BufferReady += [this](System::Object* sender, const System::EventArgs& e) {
                    OnBufferReady(sender, e);
                };

                // We will put the mic samples in this buffer. We only want to allocate it once.
                micSamples_.assign(
                    activeMicrophone_->GetSampleSizeInBytes(activeMicrophone_->getBufferDurationProperty()), 0);

                // This is a circular buffer. Samples from the mic will be mixed with the oldest
                // sample in this buffer and written back out to this buffer. This feedback
                // creates an echo effect.
                echoBuffer_.assign(
                    activeMicrophone_->GetSampleSizeInBytes(System::TimeSpan::FromSeconds(kEchoDelay)), 0);

                // Create a DynamicSoundEffectInstance in the right format to playback the
                // captured audio.
                dynamicSound_ = std::make_unique<DynamicSoundEffectInstance>(
                    activeMicrophone_->getSampleRateProperty(), AudioChannels::Mono);
                dynamicSound_->Play();

                // Success - now allocate everything we need to draw the audio waveform.
                effect_.emplace(getGraphicsDeviceProperty());
                auto& vp = getGraphicsDeviceProperty().getViewportProperty();
                auto bounds = vp.getBoundsProperty();
                effect_->setProjectionProperty(
                    Matrix::CreateTranslation(-0.5f, -0.5f, 0.0f) *
                    Matrix::CreateOrthographicOffCenter((float)bounds.getLeftProperty(), (float)bounds.getRightProperty(),
                                                         (float)bounds.getBottomProperty(), (float)bounds.getTopProperty(),
                                                         -1.0f, 1.0f));
                int sampleCount = (int)echoBuffer_.size() / (int)sizeof(int16_t);
                vertexPosColor_.assign(sampleCount, VertexPositionColor(Vector3(), Color(255, 255, 255, 255)));
            }
        } catch (const NoMicrophoneConnectedException&) {
            // Uh oh, the microphone was disconnected in the middle of initialization. Let's
            // clean up everything so we can look for another microphone again on the next update.
            activeMicrophone_ = nullptr;
        }
    }

    // Start the microphone.
    void StartMicrophone() {
        if (activeMicrophone_ == nullptr) return;
        try {
            activeMicrophone_->Start();
        } catch (const NoMicrophoneConnectedException&) {
            UpdateMicrophoneStatus();
        }
    }

    // Stop the microphone.
    void StopMicrophone() {
        if (activeMicrophone_ == nullptr) return;
        try {
            activeMicrophone_->Stop();
            // And clear the echo buffer.
            std::fill(echoBuffer_.begin(), echoBuffer_.end(), (SharpRuntime::bytecs)0);
        } catch (const NoMicrophoneConnectedException&) {
            UpdateMicrophoneStatus();
        }
    }

    // Look for a good microphone to start recording.
    Microphone* PickFirstConnectedMicrophone() {
        // Let's pick the default microphone if it's ready.
        Microphone* def = Microphone::getDefaultProperty();
        if (def != nullptr && IsConnected(*def)) {
            return def;
        }

        // Default microphone seems to be disconnected so look for another microphone that we
        // can use. And if the default was null then the list will be empty and we'll skip the
        // search.
        for (Microphone* microphone : Microphone::getAllProperty()) {
            if (IsConnected(*microphone)) {
                return microphone;
            }
        }

        // There are no microphones hooked up to the system!
        return nullptr;
    }

    // Provides a simple way to check if a microphone is connected. There is no guarantee that
    // the microphone will not get disconnected at any time.
    static bool IsConnected(Microphone& microphone) {
        try {
            [[maybe_unused]] MicrophoneState state = microphone.getStateProperty();
            return true;
        } catch (const NoMicrophoneConnectedException&) {
            return false;
        }
    }

    // Keep track of the microphone status to communicate to the user.
    void UpdateMicrophoneStatus() {
        if (activeMicrophone_ == nullptr) {
            microphoneStatus_ = "Waiting for microphone connection...";
        } else {
            try {
                // Update the status - if the microphone gets disconnected this will throw.
                MicrophoneState state = activeMicrophone_->getStateProperty();
                microphoneStatus_ = activeMicrophone_->Name + " is " +
                    (state == MicrophoneState::Started ? "Started" : "Stopped");
            } catch (const NoMicrophoneConnectedException&) {
                // Microphone got disconnected - Let's ask the user to reconnect it.
                microphoneStatus_ = "Please reconnect " + activeMicrophone_->Name;
                // Clear the echo buffer.
                std::fill(echoBuffer_.begin(), echoBuffer_.end(), (SharpRuntime::bytecs)0);
            }
        }
    }

    // This is called each time a microphone buffer has been filled.
    void OnBufferReady(System::Object* /*sender*/, const System::EventArgs& /*e*/) {
        try {
            // Copy the captured audio data into the pre-allocated array.
            activeMicrophone_->GetData(micSamples_, 0, (SharpRuntime::intcs)micSamples_.size());
            ProcessEcho();
        } catch (const NoMicrophoneConnectedException&) {
            // Microphone was disconnected - let the user know.
            UpdateMicrophoneStatus();
        }
    }

    // Captured audio is processed for echo in following steps:
    //   1) Mix each sample with a delayed sample from the echo buffer.
    //   2) Write mixed sample back into echoBuffer_ so it can echo back later.
    //   3) Submit echo buffer to dynamicSound_.
    void ProcessEcho() {
        for (int index = 0; index < (int)micSamples_.size(); index += (int)sizeof(int16_t)) {
            int16_t micSample  = ReadSample(micSamples_, index);
            int16_t echoSample = ReadSample(echoBuffer_, echoBufferPosition_);

            // Mix the echo back into the buffer.
            int16_t outputSample = (int16_t)((float)micSample * (1.0f - kEchoAmount) +
                                              (float)echoSample * kEchoAmount);
            WriteSample(echoBuffer_, echoBufferPosition_, outputSample);
            echoBufferPosition_ += (int)sizeof(int16_t);

            // Play back the echo buffer if it's filled.
            if (echoBufferPosition_ == (int)echoBuffer_.size()) {
                dynamicSound_->SubmitBuffer(echoBuffer_, 0, (SharpRuntime::intcs)echoBuffer_.size());
                // Reset the position to the beginning of the buffer.
                echoBufferPosition_ = 0;
            }
        }
    }

    // Returns a sample value from the passed buffer, taking into account the endian-ness of
    // the system.
    int16_t ReadSample(const std::vector<SharpRuntime::bytecs>& buffer, int index) const {
        if (bigEndian_) {
            return (int16_t)(buffer[index] << 8 | (buffer[index + 1] & 0xff));
        }
        return (int16_t)((buffer[index] & 0xff) | (buffer[index + 1] << 8));
    }

    // Writes the passed sample value to the buffer, taking into account the endian-ness of
    // the system.
    void WriteSample(std::vector<SharpRuntime::bytecs>& buffer, int index, int16_t sample) const {
        if (bigEndian_) {
            buffer[index]     = (SharpRuntime::bytecs)(sample >> 8);
            buffer[index + 1] = (SharpRuntime::bytecs)sample;
        } else {
            buffer[index]     = (SharpRuntime::bytecs)sample;
            buffer[index + 1] = (SharpRuntime::bytecs)(sample >> 8);
        }
    }

    void DrawHelpOverlay() {
        if (helpTimer_ <= 0.0f || !helpTexture_.has_value()) return;
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        int hw = helpTexture_->getWidthProperty();
        int hh = helpTexture_->getHeightProperty();
        float sx = (float)((vp.getWidthProperty()  - hw) / 2);
        float sy = (float)((vp.getHeightProperty() - hh) / 2);
        spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
    }
};

} // namespace MicrophoneEcho
