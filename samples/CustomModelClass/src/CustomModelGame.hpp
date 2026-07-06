#pragma once

#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/Model.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelMesh.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelEffectCollection.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/GamePad.hpp>
#include <Microsoft/Xna/Framework/Input/GamePadState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <Microsoft/Xna/Framework/Input/Buttons.hpp>
#include <Microsoft/Xna/Framework/PlayerIndex.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/MathHelper.hpp>

#include <memory>
#include <optional>
#include <string>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

namespace CustomModelSample {

// Ported using CNA's stock Model/BasicEffect instead of the C# original's own
// CustomModel/ModelPart replacement class — see missing.md: CNA has no build-time
// custom-ContentProcessor extensibility to replicate CustomModelProcessor faithfully
// (DEFERRED.md item #18), and the runtime CustomModel class only ever draws stock
// BasicEffect geometry anyway, so a stock Model produces identical on-screen results.
class CustomModelGame : public Game {
public:
    CustomModelGame() {
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        getContentProperty().setRootDirectoryProperty("Content");
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "CustomModelGame";
        return name;
    }

protected:
    void LoadContent() override {
        spriteBatch_.emplace(getGraphicsDeviceProperty());
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        model_.emplace(getContentProperty().Load<Model>("tank"));

        // Every mesh part uses a stock BasicEffect (confirmed: tank.model.json's
        // meshes all specify "effect": "BasicEffect") — enable per-vertex lighting
        // once here, matching the original's effect.EnableDefaultLighting() call,
        // since it's a persistent effect property rather than per-frame state.
        for (ModelMesh* mesh : model_->getMeshesProperty()) {
            for (Effect* effect : mesh->getEffectsPropertyMutable()) {
                static_cast<BasicEffect*>(effect)->EnableDefaultLighting();
            }
        }

        // Calculate camera view and projection matrices.
        view_ = Matrix::CreateLookAt(Vector3(1000.0f, 500.0f, 0.0f),
                                      Vector3(0.0f, 150.0f, 0.0f), Vector3::Up);

        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        float aspect = (float)vp.getWidthProperty() / (float)vp.getHeightProperty();
        projection_ = Matrix::CreatePerspectiveFieldOfView(MathHelper::PiOver4, aspect,
                                                            10.0f, 10000.0f);
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        // F1 help overlay
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput();

        // Update the world transform to make the model rotate.
        float time = (float)gameTime.getTotalGameTimeProperty().getTotalSecondsProperty();
        world_ = Matrix::CreateRotationY(time * 0.1f);

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255)); // CornflowerBlue

        model_->Draw(world_, view_, projection_);

        DrawHelpOverlay();

        Game::Draw(gameTime);
    }

private:
    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::optional<SpriteBatch> spriteBatch_;
    std::optional<Model> model_;

    Matrix world_;
    Matrix view_;
    Matrix projection_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

    // Handles input for quitting the game.
    void HandleInput() {
        KeyboardState currentKeyboardState = Keyboard::GetState();
        GamePadState  currentGamePadState  = GamePad::GetState(PlayerIndex::One);

        if (currentKeyboardState.IsKeyDown(Keys::Escape) ||
            currentGamePadState.getButtonsProperty().getBackProperty() == ButtonState::Pressed) {
            Exit();
        }
    }

    void DrawHelpOverlay() {
        if (helpTimer_ <= 0.0f || !helpTexture_.has_value()) return;
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        int hw = helpTexture_->getWidthProperty();
        int hh = helpTexture_->getHeightProperty();
        float sx = (float)((vp.getWidthProperty()  - hw) / 2);
        float sy = (float)((vp.getHeightProperty() - hh) / 2);
        spriteBatch_->Begin();
        spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        spriteBatch_->End();
    }
};

} // namespace CustomModelSample
