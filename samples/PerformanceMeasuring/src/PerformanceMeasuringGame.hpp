#pragma once

// PerformanceMeasuringGame.hpp — C++ port of PerformanceMeasuringGame.cs (XNA
// 4.0 PerformanceMeasuring sample). A 3D scene of bouncing, optionally
// colliding spheres, instrumented with the GameDebugTools TimeRuler and
// FpsCounter to demonstrate profiling a game in real time.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "System/Random.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/Model.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureSample.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"

#include "GameDebugTools/DebugSystem.hpp"
#include "Sphere.hpp"

namespace PerformanceMeasuring {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::GraphicsDeviceManager;
using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::Matrix;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Graphics::GraphicsDevice;
using Microsoft::Xna::Framework::Graphics::Model;
using Microsoft::Xna::Framework::Graphics::SamplerState;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::GamePadState;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Input::Touch::GestureSample;
using Microsoft::Xna::Framework::Input::Touch::GestureType;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

using GameDebugTools::DebugSystem;

// This sample game shows how to use the GameDebugTools to measure the
// performance of a game, as well as how the number of objects and
// interactions between them can affect performance. Port of
// PerformanceMeasuringGame.cs.
class PerformanceMeasuringGame : public Game {
public:
    PerformanceMeasuringGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "PerformanceMeasuringGame";
        return name;
    }

protected:
    void Initialize() override {
        // The FpsCounter shows the current frames-per-second. The TimeRuler
        // shows where the per-frame CPU time is going.
        DebugSystem::Initialize(*this, "Font");
        DebugSystem::Instance().getFpsCounter().setVisibleProperty(true);
        DebugSystem::Instance().getTimeRuler().setVisibleProperty(true);
        DebugSystem::Instance().getTimeRuler().ShowLog = true;

        // Enable Tap and FreeDrag gestures (a touch fallback isn't needed on
        // this desktop, but TouchPanel::ReadGesture() is still polled below to
        // stay faithful to the original -- it simply never produces gestures
        // without a touchscreen).
        TouchPanel::setEnabledGesturesProperty(GestureType::Tap | GestureType::FreeDrag);

        Game::Initialize();
    }

    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

        font_.emplace(getContentProperty().Load<SpriteFont>("Font"));

        blank_.emplace(getGraphicsDeviceProperty(), 1, 1);
        Color white = Color::White;
        blank_->SetData(&white, 1);

        // F1 help overlay (CNA addition beyond the XNA original).
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        ground_.emplace(getContentProperty().Load<Model>("Ground"));

        CreateSpheres();
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        // We must call StartFrame at the top of Update to indicate to the
        // TimeRuler that a new frame has started.
        DebugSystem::Instance().getTimeRuler().StartFrame();

        DebugSystem::Instance().getTimeRuler().BeginMark("Update", Color::Blue);

        UpdateInput(gameTime);
        UpdateSpheres(gameTime);

        Game::Update(gameTime);

        DebugSystem::Instance().getTimeRuler().EndMark("Update");
    }

    void Draw(const GameTime& gameTime) override {
        DebugSystem::Instance().getTimeRuler().BeginMark("Draw", Color::Red);

        getGraphicsDeviceProperty().Clear(Color::CornflowerBlue);

        Matrix view = Matrix::CreateLookAt(Vector3(WorldSize, WorldSize, WorldSize) * 1.5f, Vector3::Zero, Vector3::Up);
        Matrix projection = Matrix::CreatePerspectiveFieldOfView(
            MathHelper::PiOver4, getGraphicsDeviceProperty().getViewportProperty().getAspectRatioProperty(), 0.01f,
            100.0f);

        getGraphicsDeviceProperty().getSamplerStatesProperty()[0] = SamplerState::LinearWrap;

        ground_->Draw(Matrix::CreateScale(WorldSize, 1.0f, WorldSize), view, projection);

        for (int i = 0; i < activeSphereCount_; i++) {
            spheres_[(size_t)i].Draw(view, projection);
        }

        DrawDemoText();

        // F1 help overlay (CNA addition beyond the XNA original), drawn in its
        // own screen-space batch -- see NEXT.md section 5 on why a second
        // Begin/End per frame is safe on the default EasyGL backend.
        if (helpTimer_ > 0.0f) {
            spriteBatch_->Begin();
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = (float)((vp.getWidthProperty() - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
            spriteBatch_->End();
        }

        Game::Draw(gameTime);

        DebugSystem::Instance().getTimeRuler().EndMark("Draw");
    }

private:
    static constexpr int MaximumNumberOfSpheres = 200;
    static constexpr float WorldSize = 20.0f;

    void CreateSpheres() {
        System::Random random;

        Color sphereColors[] = {Color::Red,  Color::Blue,   Color::Green,  Color::Orange,
                                 Color::Pink, Color::Purple, Color::Yellow};

        const float radius = 1.0f;

        spheres_.reserve(MaximumNumberOfSpheres);
        for (int i = 0; i < MaximumNumberOfSpheres; i++) {
            spheres_.emplace_back(getGraphicsDeviceProperty(), radius);
            Sphere& sphere = spheres_.back();

            sphere.Position = Vector3(RandomFloat(random, -WorldSize + radius, WorldSize - radius),
                                       RandomFloat(random, radius, WorldSize - radius),
                                       RandomFloat(random, -WorldSize + radius, WorldSize - radius));

            sphere.SphereColor = sphereColors[random.Next((int)(sizeof(sphereColors) / sizeof(sphereColors[0])))];

            sphere.Velocity = Vector3(RandomFloat(random, -10.0f, 10.0f), RandomFloat(random, -10.0f, 10.0f),
                                       RandomFloat(random, -10.0f, 10.0f));
        }
    }

    static float RandomFloat(System::Random& random, float min, float max) {
        return (float)random.NextDouble() * (max - min) + min;
    }

    void UpdateInput(const GameTime&) {
        gamePadPrev_ = gamePad_;
        gamePad_ = GamePad::GetState(PlayerIndex::One);
        keyboardPrev_ = keyboard_;
        keyboard_ = Keyboard::GetState();

        if (gamePad_.IsButtonDown(Buttons::Back) || keyboard_.IsKeyDown(Keys::Escape))
            Exit();

        if ((gamePad_.IsButtonDown(Buttons::X) && gamePadPrev_.IsButtonUp(Buttons::X)) ||
            (keyboard_.IsKeyDown(Keys::X) && keyboardPrev_.IsKeyUp(Keys::X))) {
            collideSpheres_ = !collideSpheres_;
        }

        if (gamePad_.IsButtonDown(Buttons::DPadUp) || gamePad_.IsButtonDown(Buttons::LeftThumbstickUp) ||
            keyboard_.IsKeyDown(Keys::Up)) {
            activeSphereCount_++;
        } else if (gamePad_.IsButtonDown(Buttons::DPadDown) || gamePad_.IsButtonDown(Buttons::LeftThumbstickDown) ||
                   keyboard_.IsKeyDown(Keys::Down)) {
            activeSphereCount_--;
        }

        while (TouchPanel::getIsGestureAvailableProperty()) {
            GestureSample gesture = TouchPanel::ReadGesture();

            if (gesture.getGestureTypeProperty() == GestureType::Tap) {
                collideSpheres_ = !collideSpheres_;
            } else if (gesture.getGestureTypeProperty() == GestureType::FreeDrag) {
                float dy = gesture.getDeltaProperty().Y;
                activeSphereCount_ -= (dy > 0.0f) - (dy < 0.0f);
            }
        }

        activeSphereCount_ = std::max(std::min(activeSphereCount_, MaximumNumberOfSpheres), 1);
    }

    void UpdateSpheres(const GameTime& gameTime) {
        for (int i = 0; i < activeSphereCount_; i++) {
            Sphere& s = spheres_[(size_t)i];
            s.Update(gameTime);
            BounceSphereInWorld(s);
        }

        if (collideSpheres_) {
            for (int i = 0; i < activeSphereCount_; i++) {
                for (int j = 0; j < activeSphereCount_; j++) {
                    if (i == j) continue;

                    Sphere& a = spheres_[(size_t)i];
                    Sphere& b = spheres_[(size_t)j];

                    if (a.getBounds().Intersects(b.getBounds())) {
                        Vector3 delta = b.Position - a.Position;
                        Vector3 center = a.Position + delta / 2.0f;
                        delta.Normalize();

                        a.Position = center - delta * a.getRadius();
                        b.Position = center + delta * b.getRadius();

                        a.Velocity = Vector3::Normalize(Vector3::Reflect(a.Velocity, delta)) * b.Velocity.Length();
                        b.Velocity = Vector3::Normalize(Vector3::Reflect(b.Velocity, delta)) * a.Velocity.Length();
                    }
                }
            }
        }
    }

    static void BounceSphereInWorld(Sphere& s) {
        if (s.Position.X < -WorldSize + s.getRadius()) {
            s.Position.X = -WorldSize + s.getRadius();
            if (s.Velocity.X < 0.0f) s.Velocity.X *= -1.0f;
        } else if (s.Position.X > WorldSize - s.getRadius()) {
            s.Position.X = WorldSize - s.getRadius();
            if (s.Velocity.X > 0.0f) s.Velocity.X *= -1.0f;
        }

        if (s.Position.Y < s.getRadius()) {
            s.Position.Y = s.getRadius();
            if (s.Velocity.Y < 0.0f) s.Velocity.Y *= -1.0f;
        } else if (s.Position.Y > WorldSize - s.getRadius()) {
            s.Position.Y = WorldSize - s.getRadius();
            if (s.Velocity.Y > 0.0f) s.Velocity.Y *= -1.0f;
        }

        if (s.Position.Z < -WorldSize + s.getRadius()) {
            s.Position.Z = -WorldSize + s.getRadius();
            if (s.Velocity.Z < 0.0f) s.Velocity.Z *= -1.0f;
        } else if (s.Position.Z > WorldSize - s.getRadius()) {
            s.Position.Z = WorldSize - s.getRadius();
            if (s.Velocity.Z > 0.0f) s.Velocity.Z *= -1.0f;
        }
    }

    void DrawDemoText() {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "Sphere count: %d\nCollisions Enabled: %s\n\n%s", activeSphereCount_,
                      collideSpheres_ ? "True" : "False", Instructions);
        std::string demoText = buf;

        Vector2 size = font_->MeasureString(demoText);
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        Rectangle safeArea = vp.getTitleSafeAreaProperty();
        Vector2 pos((float)safeArea.getRightProperty() - size.X, (float)safeArea.getTopProperty());

        spriteBatch_->Begin();

        spriteBatch_->Draw(*blank_, Rectangle((int)pos.X - 5, (int)pos.Y, (int)size.X + 10, (int)size.Y + 5),
                            Color(0, 0, 0, 128));

        spriteBatch_->DrawString(*font_, demoText, pos, Color::White);

        spriteBatch_->End();
    }

    static constexpr const char* Instructions =
        "X - Toggle collisions\nUp - Increase number of spheres\nDown - Decrease number of spheres";

    GraphicsDeviceManager graphics_;

    std::unique_ptr<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont> font_;
    std::optional<Texture2D> blank_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;

    std::optional<Model> ground_;

    std::vector<Sphere> spheres_;
    int activeSphereCount_ = 50;

    bool collideSpheres_ = true;

    GamePadState gamePad_, gamePadPrev_;
    KeyboardState keyboard_, keyboardPrev_;
};

} // namespace PerformanceMeasuring
