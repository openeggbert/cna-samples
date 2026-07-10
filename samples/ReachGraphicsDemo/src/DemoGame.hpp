#pragma once

// Ported from XnaGraphicsDemo.DemoGame (DemoGame.cs) -- the main game class. Owns the
// shared SpriteBatch/SpriteFont/BigFont/BlankTexture resources, the 7 MenuComponent
// screens (TitleMenu + 6 demo scenes), the crossfade "transition effect" played whenever
// SetActiveMenu() switches screens, and the "zoomy text" visual feedback effect shown
// when a menu item is clicked.
//
// DemoGame's own constructor is declared here but DEFINED in Program.cpp: it needs to
// construct all 7 concrete MenuComponent subclasses (TitleMenu, BasicDemo, AlphaDemo,
// DualDemo, SkinnedDemo, EnvmapDemo, ParticleDemo), and each of THOSE headers itself
// `#include`s this file (DemoGame.hpp) -- so DemoGame.hpp cannot include them back
// without a cycle. Every other DemoGame method needs only MenuComponent's (already
// complete, see below) interface, so they stay inline here.
//
// This header also provides the out-of-line definitions of MenuComponent's/MenuEntry's
// own methods that were only DECLARED in MenuComponent.hpp/MenuEntry.hpp (because those
// two headers only forward-declare DemoGame) -- mirroring this repo's
// GameStateManagement port, where GameScreen::Update/ExitScreen are defined out-of-line
// at the bottom of ScreenManager.hpp for the identical reason.

#include <cctype>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/RenderTarget2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "System/Random.hpp"
#include "System/TimeSpan.hpp"

#include "MenuComponent.hpp"
#include "MenuEntry.hpp"

namespace ReachGraphicsDemoSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;
using System::TimeSpan;

class DemoGame : public Game {
public:
    // Defined in Program.cpp (constructs the 7 concrete MenuComponent subclasses).
    DemoGame();

    const std::string& GetTypeName() const override {
        static const std::string name = "DemoGame";
        return name;
    }

    // Properties (mirrors the C# original's public get-only properties).
    GraphicsDeviceManager& GetGraphics() { return *graphics_; }
    SpriteBatch& GetSpriteBatch() { return *spriteBatch_; }
    SpriteFont& GetFont() { return *font_; }
    SpriteFont& GetBigFont() { return *bigFont_; }
    Texture2D& GetBlankTexture() { return *blankTexture_; }
    const Matrix& GetScaleMatrix() const { return scaleMatrix_; }

    // Changes which menu screen is currently active.
    void SetActiveMenu(int index) {
        // Trigger the transition effect.
        for (std::size_t i = 0; i < menuComponents_.size(); ++i) {
            if (menuComponents_[i]->getVisibleProperty()) {
                BeginTransition(static_cast<int>(i), index);
                break;
            }
        }

        // Mark the previous menu as inactive, and the new one as active.
        for (std::size_t i = 0; i < menuComponents_.size(); ++i) {
            bool active = (static_cast<int>(i) == index);
            menuComponents_[i]->setEnabledProperty(active);
            menuComponents_[i]->setVisibleProperty(active);

            menuComponents_[i]->Reset();
        }
    }

    // Creates a new zoomy text menu item selection effect.
    static void SpawnZoomyText(const std::string& text, Vector2 position) {
        ZoomyText zt;
        zt.Text = text;
        zt.Position = position;
        zt.Age = 0.0f;
        zoomyTexts_.push_back(zt);
    }

protected:
    // Loads content and creates graphics resources.
    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

        font_.emplace(getContentProperty().Load<SpriteFont>("font"));
        bigFont_.emplace(getContentProperty().Load<SpriteFont>("bigfont"));

        blankTexture_.emplace(getGraphicsDeviceProperty(), 1, 1);
        Color white = Color::White;
        blankTexture_->SetData(&white, 1);

        transitionRenderTarget_ = std::make_unique<RenderTarget2D>(
            getGraphicsDeviceProperty(), 480, 800, false, SurfaceFormat::Color, DepthFormat::Depth24, 0,
            RenderTargetUsage::DiscardContents);

        // F1 help overlay (CNA addition -- see CLAUDE.md).
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    // Updates the transition effect and zoomy text animations.
    void Update(GameTime& gameTime) override {
        // F1 help overlay (CNA addition -- see CLAUDE.md), checked before anything else.
        float elapsed = static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        currentGameTime_ = &gameTime;

        UpdateZoomyText(gameTime);

        if (transitionTimer_ < std::numeric_limits<float>::max()) {
            transitionTimer_ += static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());
        }

        // This updates game components, including the currently active menu screen.
        Game::Update(gameTime);
    }

    // Draws the game.
    void Draw(const GameTime& gameTime) override {
        scaleMatrix_ = Matrix::CreateScale(graphics_->getPreferredBackBufferWidthProperty() / 480.0f,
                                            graphics_->getPreferredBackBufferHeightProperty() / 800.0f, 1.0f);

        // This draws game components, including the currently active menu screen.
        Game::Draw(gameTime);

        DrawTransitionEffect();
        DrawZoomyText();

        // F1 help overlay (CNA addition -- see CLAUDE.md), drawn last, on top of everything.
        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = static_cast<float>((vp.getWidthProperty() - hw) / 2);
            float sy = static_cast<float>((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Begin();
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
            spriteBatch_->End();
        }
    }

private:
    // Begins a transition effect, capturing a copy of the current screen into the
    // transitionRenderTarget.
    void BeginTransition(int oldMenuIndex, int newMenuIndex) {
        scaleMatrix_ = Matrix::getIdentityProperty();

        getGraphicsDeviceProperty().SetRenderTarget(transitionRenderTarget_.get());

        // Draw the old menu screen into the rendertarget.
        menuComponents_[static_cast<std::size_t>(oldMenuIndex)]->Draw(*currentGameTime_);

        // Force the rendertarget alpha channel to fully opaque.
        spriteBatch_->Begin(SpriteSortMode::Deferred, BlendState::Additive);
        spriteBatch_->Draw(*blankTexture_, Rectangle(0, 0, 480, 800), Color(0, 0, 0, 255));
        spriteBatch_->End();

        getGraphicsDeviceProperty().SetRenderTarget(nullptr);

        // Initialize the transition state.
        transitionTimer_ = static_cast<float>(getTargetElapsedTimeProperty().getTotalSecondsProperty());
        transitionMode_ = newMenuIndex;
    }

    // Draws the transition effect, displaying various animating pieces of the
    // rendertarget which contains the previous scene image over the top of the new
    // scene. There are various different effects which animate these pieces in
    // different ways.
    void DrawTransitionEffect() {
        if (transitionTimer_ >= TransitionSpeed) return;

        spriteBatch_->Begin();

        float mu = transitionTimer_ / TransitionSpeed;
        float alpha = 1.0f - mu;

        switch (transitionMode_) {
            case 1:
                // BasicEffect
                DrawOpenCurtainsTransition(alpha);
                break;

            case 2:
            case 5:
                // DualTexture
                // EnvironmentMap
                DrawSpinningSquaresTransition(mu, alpha);
                break;

            case 3:
            case 4:
                // AlphaTest and Skinning
                DrawChequeredAppearTransition(mu);
                break;

            case 6:
                // Particles
                DrawFallingLinesTransition(mu);
                break;

            default:
                // Returning to menu.
                DrawShrinkAndSpinTransition(mu, alpha);
                break;
        }

        spriteBatch_->End();
    }

    // Transition effect where the screen splits in half, opening down the middle.
    void DrawOpenCurtainsTransition(float alpha) {
        int w = static_cast<int>(240 * alpha * alpha);

        Color tint = Color(255, 255, 255, 255) * alpha;
        spriteBatch_->Draw(*transitionRenderTarget_, Rectangle(0, 0, w, 800), Rectangle(0, 0, 240, 800), tint);
        spriteBatch_->Draw(*transitionRenderTarget_, Rectangle(480 - w, 0, w, 800), Rectangle(240, 0, 240, 800), tint);
    }

    // Transition effect where the screen splits into pieces, each spinning off in a
    // different direction.
    void DrawSpinningSquaresTransition(float mu, float alpha) {
        System::Random random(23);

        for (int x = 0; x < 4; x++) {
            for (int y = 0; y < 8; y++) {
                Rectangle rect(480 * x / 4, 800 * y / 8, 480 / 4, 800 / 8);

                Vector2 origin(rect.Width / 2.0f, rect.Height / 2.0f);

                float rotation = static_cast<float>(random.NextDouble() - 0.5) * mu * mu * 2;
                float scale = 1.0f + static_cast<float>(random.NextDouble() - 0.5) * mu * mu;

                Vector2 pos(static_cast<float>(rect.X + rect.Width / 2), static_cast<float>(rect.Y + rect.Height / 2));

                pos.X += static_cast<float>(random.NextDouble() - 0.5) * mu * mu * 400;
                pos.Y += static_cast<float>(random.NextDouble() - 0.5) * mu * mu * 400;

                spriteBatch_->Draw(*transitionRenderTarget_, pos, rect, Color(255, 255, 255, 255) * alpha, rotation, origin,
                                    scale, SpriteEffects::None, 0.0f);
            }
        }
    }

    // Transition effect where each square of the image appears at a different time.
    void DrawChequeredAppearTransition(float mu) {
        System::Random random(23);

        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 16; y++) {
                Rectangle rect(480 * x / 8, 800 * y / 16, 480 / 8, 800 / 16);

                if (random.NextDouble() > mu * mu) {
                    spriteBatch_->Draw(*transitionRenderTarget_, rect, rect, Color::White);
                }
            }
        }

        // The zoomy text effect doesn't look so good with this particular transition
        // effect, so we temporarily disable it.
        zoomyTexts_.clear();
    }

    // Transition effect where the image dissolves into a sequence of vertically falling
    // lines.
    void DrawFallingLinesTransition(float mu) {
        System::Random random(23);

        constexpr int segments = 60;

        for (int x = 0; x < segments; x++) {
            Rectangle rect(480 * x / segments, 0, 480 / segments, 800);

            Vector2 pos(static_cast<float>(rect.X), 0.0f);

            pos.Y += 800.0f * static_cast<float>(std::pow(mu, random.NextDouble() * 10));

            spriteBatch_->Draw(*transitionRenderTarget_, pos, rect, Color::White);
        }
    }

    // Transition effect where the image spins off toward the bottom left of the screen.
    void DrawShrinkAndSpinTransition(float mu, float alpha) {
        Vector2 origin(240.0f, 400.0f);
        Vector2 translate = (Vector2(32.0f, 800.0f - 32.0f) - origin) * (mu * mu);

        float rotation = mu * mu * -4.0f;
        float scale = alpha * alpha;

        Color tint = Color::White * std::sqrt(alpha);

        spriteBatch_->Draw(*transitionRenderTarget_, origin + translate, std::optional<Rectangle>(), tint, rotation,
                            origin, scale, SpriteEffects::None, 0.0f);
    }

    // Updates the zoomy text animations.
    static void UpdateZoomyText(const GameTime& gameTime) {
        std::size_t i = 0;

        while (i < zoomyTexts_.size()) {
            zoomyTexts_[i].Age += static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());

            if (zoomyTexts_[i].Age >= ZoomyTextLifespan) {
                zoomyTexts_.erase(zoomyTexts_.begin() + static_cast<long>(i));
            } else {
                ++i;
            }
        }
    }

    // Draws the zoomy text animations.
    void DrawZoomyText() {
        if (zoomyTexts_.empty()) return;

        spriteBatch_->Begin(SpriteSortMode::Deferred, BlendState::AlphaBlend, nullptr, nullptr, nullptr, nullptr,
                             scaleMatrix_);

        for (auto& zoomyText : zoomyTexts_) {
            Vector2 pos = zoomyText.Position + font_->MeasureString(zoomyText.Text) / 2.0f;

            float age = zoomyText.Age / ZoomyTextLifespan;
            float sqrtAge = std::sqrt(age);

            float scale = 0.333f + sqrtAge * 2.0f;

            float alpha = 1.0f - age;

            SpriteFont* font = &bigFont_.value();

            // Our BigFont only contains characters a-z, so if the text contains any
            // numbers, we have to use the other font instead.
            for (char ch : zoomyText.Text) {
                if (std::isdigit(static_cast<unsigned char>(ch))) {
                    font = &font_.value();
                    scale *= 2.0f;
                    break;
                }
            }

            Vector2 origin = font->MeasureString(zoomyText.Text) / 2.0f;

            Color tint = Color::Lerp(Color(64, 64, 255, 255), Color::White, sqrtAge) * alpha;

            spriteBatch_->DrawString(*font, zoomyText.Text, pos, tint, 0.0f, origin, scale, SpriteEffects::None, 0.0f);
        }

        spriteBatch_->End();
    }

    // Constants.
    static constexpr float TransitionSpeed = 1.5f;
    static constexpr float ZoomyTextLifespan = 0.75f;

    // Properties (storage).
    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont> font_;
    std::optional<SpriteFont> bigFont_;
    std::optional<Texture2D> blankTexture_;
    Matrix scaleMatrix_ = Matrix::getIdentityProperty();

    // Fields.
    std::vector<std::shared_ptr<MenuComponent>> menuComponents_;

    const GameTime* currentGameTime_ = nullptr;

    // Transition effects provide swooshy crossfades when moving from one screen to another.
    float transitionTimer_ = std::numeric_limits<float>::max();
    int transitionMode_ = 0;
    std::unique_ptr<RenderTarget2D> transitionRenderTarget_;

    // Zoomy text provides visual feedback when selecting menu items. This is
    // implemented by the main game, rather than any individual menu screen, because the
    // zoomy effect from selecting a menu item needs to display across the transition
    // while that menu makes way for a new one.
    struct ZoomyText {
        std::string Text;
        Vector2 Position;
        float Age = 0.0f;
    };

    inline static std::vector<ZoomyText> zoomyTexts_;

    // F1 help overlay (CNA addition -- see CLAUDE.md).
    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;
};

// ---------------------------------------------------------------------------------
// Out-of-line definitions of MenuComponent's/MenuEntry's methods that were only
// declared in MenuComponent.hpp/MenuEntry.hpp (see the comment at the top of this file
// and in MenuComponent.hpp/MenuEntry.hpp for the full reasoning).
// ---------------------------------------------------------------------------------

inline MenuComponent::MenuComponent(DemoGame& game) : DrawableGameComponent(game), game_(game) {}

inline SpriteBatch& MenuComponent::GetSpriteBatch() const { return game_.GetSpriteBatch(); }
inline SpriteFont& MenuComponent::GetFont() const { return game_.GetFont(); }
inline SpriteFont& MenuComponent::GetBigFont() const { return game_.GetBigFont(); }

inline void MenuComponent::Update(GameTime& gameTime) {
    // We read input using the mouse API, which will report the first touch point when
    // run on the phone, but also works on Windows using a regular mouse.
    MouseState input = game_.getIsActiveProperty() ? Mouse::GetState() : MouseState();

    // Scale input if we are running in an unusual screen resolution.
    int touchX = input.getXProperty() * 480 / game_.GetGraphics().getPreferredBackBufferWidthProperty();
    int touchY = input.getYProperty() * 800 / game_.GetGraphics().getPreferredBackBufferHeightProperty();

    // Process the input.
    if (input.getLeftButtonProperty() == ButtonState::Pressed) {
        HandleTouchDown(touchX, touchY);
    } else {
        HandleTouchUp();
    }

    HandleAttractMode(gameTime, input);
}

inline void MenuComponent::Draw(const GameTime& /*gameTime*/) {
    GetSpriteBatch().Begin(SpriteSortMode::Deferred, BlendState::AlphaBlend, nullptr, nullptr, nullptr, nullptr,
                            game_.GetScaleMatrix());

    for (auto& entry : Entries) {
        entry->Draw(GetSpriteBatch(), GetFont(), game_.GetBlankTexture());
    }

    GetSpriteBatch().End();
}

inline void MenuComponent::DrawTitle(const std::string& title, std::optional<Color> backgroundColor, Color titleColor) {
    if (backgroundColor.has_value()) {
        getGraphicsDeviceProperty().Clear(backgroundColor.value());
    }

    GetSpriteBatch().Begin(SpriteSortMode::Deferred, BlendState::AlphaBlend, nullptr, nullptr, nullptr, nullptr,
                            game_.GetScaleMatrix());
    GetSpriteBatch().DrawString(GetBigFont(), title, Vector2(480.0f, 24.0f), titleColor, MathHelper::PiOver2,
                                 Vector2::Zero, 1.0f, SpriteEffects::None, 0.0f);
    GetSpriteBatch().End();
}

inline void MenuEntry::OnClicked() {
    // If we have a click delegate, call that now.
    if (Clicked) Clicked();

    // If we are not draggable, spawn a visual feedback effect.
    if (!IsDraggable) {
        DemoGame::SpawnZoomyText(GetText(), Position + positionOffset_);
    }
}

} // namespace ReachGraphicsDemoSample
