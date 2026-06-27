#pragma once
#include <memory>
#include <optional>
#include <string>
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "AlignedSpriteBatch.hpp"
#include "SafeAreaOverlay.hpp"

namespace SafeArea {

class SafeAreaGame : public Microsoft::Xna::Framework::Game {
    static constexpr int ScreenWidth  = 1280;
    static constexpr int ScreenHeight = 720;

    Microsoft::Xna::Framework::GraphicsDeviceManager graphics_;
    std::unique_ptr<AlignedSpriteBatch> spriteBatch_;
    Microsoft::Xna::Framework::Graphics::Texture2D catTexture_;
    Microsoft::Xna::Framework::Graphics::Texture2D backgroundTexture_;
    std::optional<Microsoft::Xna::Framework::Graphics::SpriteFont> font_;

    Microsoft::Xna::Framework::Vector2 catPosition_;
    Microsoft::Xna::Framework::Vector2 catVelocity_;
    Microsoft::Xna::Framework::Vector2 cameraPosition_;

    Microsoft::Xna::Framework::Input::KeyboardState currentKeyboardState_;
    Microsoft::Xna::Framework::Input::GamePadState  currentGamePadState_;
    Microsoft::Xna::Framework::Input::KeyboardState previousKeyboardState_;
    Microsoft::Xna::Framework::Input::GamePadState  previousGamePadState_;

    SafeAreaOverlay* safeAreaOverlay_ = nullptr;

    // --- F1 help overlay ---
    std::optional<Microsoft::Xna::Framework::Graphics::Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

public:
    SafeAreaGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");
        graphics_.setPreferredBackBufferWidthProperty(ScreenWidth);
        graphics_.setPreferredBackBufferHeightProperty(ScreenHeight);

        auto* overlay = new SafeAreaOverlay(*this);
        getComponentsProperty().Add(overlay);
        safeAreaOverlay_ = overlay;
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "SafeAreaGame";
        return name;
    }

protected:
    void LoadContent() override {
        using namespace Microsoft::Xna::Framework::Graphics;
        spriteBatch_      = std::make_unique<AlignedSpriteBatch>(getGraphicsDeviceProperty());
        catTexture_        = getContentProperty().Load<Texture2D>("Cat");
        backgroundTexture_ = getContentProperty().Load<Texture2D>("Background");
        font_.emplace(getContentProperty().Load<SpriteFont>("Font"));
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(Microsoft::Xna::Framework::GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Microsoft::Xna::Framework::Input::Keyboard::GetState().IsKeyDown(Microsoft::Xna::Framework::Input::Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput();
        UpdateCat();
        UpdateCamera();
        Game::Update(gameTime);
    }

    void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) override {
        using namespace Microsoft::Xna::Framework;
        getGraphicsDeviceProperty().Clear(Color::Black);

        Vector2 screenCenter((float)ScreenWidth / 2.0f, (float)ScreenHeight / 2.0f);
        Vector2 scrollOffset = screenCenter - cameraPosition_;

        spriteBatch_->Begin();
        DrawBackground(scrollOffset);
        DrawCat(scrollOffset);
        DrawOverlays();
        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = (float)((vp.getWidthProperty()  - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }
        spriteBatch_->End();

        Game::Draw(gameTime);
    }

private:
    void HandleInput() {
        using namespace Microsoft::Xna::Framework::Input;
        previousKeyboardState_ = currentKeyboardState_;
        previousGamePadState_  = currentGamePadState_;
        currentKeyboardState_  = Keyboard::GetState();
        currentGamePadState_   = GamePad::GetState(Microsoft::Xna::Framework::PlayerIndex::One);

        if (currentKeyboardState_.IsKeyDown(Keys::Escape) ||
            currentGamePadState_.IsButtonDown(Buttons::Back))
        {
            Exit();
        }

        if (safeAreaOverlay_ != nullptr) {
            bool aPressed = (currentKeyboardState_.IsKeyDown(Keys::A) &&
                             previousKeyboardState_.IsKeyUp(Keys::A)) ||
                            (currentGamePadState_.IsButtonDown(Buttons::A) &&
                             previousGamePadState_.IsButtonUp(Buttons::A));
            if (aPressed)
                safeAreaOverlay_->setVisibleProperty(!safeAreaOverlay_->getVisibleProperty());
        }
    }

    void UpdateCat() {
        using namespace Microsoft::Xna::Framework;
        const float speedOfCat = 0.75f;
        const float catFriction = 0.9f;

        Vector2 flipY(1.0f, -1.0f);
        catVelocity_ = catVelocity_ +
            currentGamePadState_.getThumbSticksProperty().getLeftProperty() * flipY * speedOfCat;

        if (currentKeyboardState_.IsKeyDown(Microsoft::Xna::Framework::Input::Keys::Left))
            catVelocity_ = catVelocity_ + Vector2(-speedOfCat, 0.0f);
        if (currentKeyboardState_.IsKeyDown(Microsoft::Xna::Framework::Input::Keys::Right))
            catVelocity_ = catVelocity_ + Vector2(speedOfCat, 0.0f);
        if (currentKeyboardState_.IsKeyDown(Microsoft::Xna::Framework::Input::Keys::Up))
            catVelocity_ = catVelocity_ + Vector2(0.0f, -speedOfCat);
        if (currentKeyboardState_.IsKeyDown(Microsoft::Xna::Framework::Input::Keys::Down))
            catVelocity_ = catVelocity_ + Vector2(0.0f, speedOfCat);

        catPosition_ = catPosition_ + catVelocity_;
        catVelocity_ = catVelocity_ * catFriction;
    }

    void UpdateCamera() {
        using namespace Microsoft::Xna::Framework;
        Vector2 maxScroll((float)ScreenWidth / 2.0f, (float)ScreenHeight / 2.0f);
        const float catSafeArea = 0.7f;
        maxScroll = maxScroll * catSafeArea;
        maxScroll = maxScroll - Vector2((float)catTexture_.getWidthProperty() / 2.0f,
                                        (float)catTexture_.getHeightProperty() / 2.0f);

        Vector2 min = catPosition_ - maxScroll;
        Vector2 max = catPosition_ + maxScroll;

        cameraPosition_.X = MathHelper::Clamp(cameraPosition_.X, min.X, max.X);
        cameraPosition_.Y = MathHelper::Clamp(cameraPosition_.Y, min.Y, max.Y);
    }

    void DrawBackground(Microsoft::Xna::Framework::Vector2 scrollOffset) {
        using namespace Microsoft::Xna::Framework;
        int bw = backgroundTexture_.getWidthProperty();
        int bh = backgroundTexture_.getHeightProperty();

        int tileX = (int)scrollOffset.X % bw;
        int tileY = (int)scrollOffset.Y % bh;
        if (tileX > 0) tileX -= bw;
        if (tileY > 0) tileY -= bh;

        for (int x = tileX; x < ScreenWidth; x += bw)
            for (int y = tileY; y < ScreenHeight; y += bh)
                spriteBatch_->Draw(backgroundTexture_, Vector2((float)x, (float)y), Color::White);
    }

    void DrawCat(Microsoft::Xna::Framework::Vector2 scrollOffset) {
        using namespace Microsoft::Xna::Framework;
        Vector2 catCenter((float)catTexture_.getWidthProperty() / 2.0f,
                           (float)catTexture_.getHeightProperty() / 2.0f);
        Vector2 position = catPosition_ - catCenter + scrollOffset;
        spriteBatch_->Draw(catTexture_, position, Color::White);
    }

    void DrawOverlays() {
        using namespace Microsoft::Xna::Framework;
        using namespace Microsoft::Xna::Framework::Graphics;

        Viewport viewport = getGraphicsDeviceProperty().getViewportProperty();
        Rectangle safeArea = viewport.getTitleSafeAreaProperty();

        spriteBatch_->DrawString(*font_, "Top Left",
                                 Vector2((float)safeArea.getLeftProperty(),
                                         (float)safeArea.getTopProperty()),
                                 Color::White, Alignment::TopLeft);

        spriteBatch_->DrawString(*font_, "Top Right",
                                 Vector2((float)safeArea.getRightProperty(),
                                         (float)safeArea.getTopProperty()),
                                 Color::White, Alignment::TopRight);

        spriteBatch_->DrawString(*font_, "Bottom Left",
                                 Vector2((float)safeArea.getLeftProperty(),
                                         (float)safeArea.getBottomProperty()),
                                 Color::White, Alignment::BottomLeft);

        spriteBatch_->DrawString(*font_, "Bottom Right",
                                 Vector2((float)safeArea.getRightProperty(),
                                         (float)safeArea.getBottomProperty()),
                                 Color::White, Alignment::BottomRight);

        if (safeAreaOverlay_ != nullptr) {
            spriteBatch_->DrawString(*font_, "Press A to toggle the safe area overlay",
                                     Vector2((float)safeArea.getCenterProperty().X,
                                             (float)safeArea.getTopProperty()),
                                     Color::White, Alignment::TopCenter);
        }
    }
};

} // namespace SafeArea
