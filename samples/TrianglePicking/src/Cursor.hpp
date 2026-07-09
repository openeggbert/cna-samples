#pragma once

// Ported from TrianglePickingSample.Cursor (Cursor.cs). The original supports Xbox/
// Windows/Windows-Phone input branches (#if XBOX / WINDOWS / WINDOWS_PHONE); this
// desktop port keeps only the Windows (mouse) branch, matching the .htm's own control
// scheme -- see missing.md. Structurally identical to PickingSample's own Cursor.hpp
// (same original author/sample family, same Cursor.cs shape) but kept as its own,
// independent copy per this repo's "no shared samples/common" rule.

#include <Microsoft/Xna/Framework/DrawableGameComponent.hpp>
#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>
#include <Microsoft/Xna/Framework/Ray.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/Viewport.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>
#include <Microsoft/Xna/Framework/Input/MouseState.hpp>

#include <optional>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

namespace TrianglePicking {

// Cursor is a DrawableGameComponent that draws a cursor on the screen.
class Cursor : public DrawableGameComponent {
public:
    explicit Cursor(Game& game) : DrawableGameComponent(game) {}

    const std::string& GetTypeName() const override {
        static const std::string name = "Cursor";
        return name;
    }

    // Position is the cursor position, in screen space.
    Vector2 getPositionProperty() const { return position_; }

    void Update(GameTime& gameTime) override {
        (void)gameTime;
        UpdateWindowsInput();
        DrawableGameComponent::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        spriteBatch_->Begin();

        // use textureCenter as the origin of the sprite, so that the cursor is drawn
        // centered around Position.
        spriteBatch_->Draw(*cursorTexture_, position_, std::nullopt, Color::White, 0.0f,
                           textureCenter_, 1.0f, SpriteEffects::None, 0.0f);

        spriteBatch_->End();
        DrawableGameComponent::Draw(gameTime);
    }

    // CalculateCursorRay calculates a world space ray starting at the camera's "eye" and
    // pointing in the direction of the cursor. Viewport::Unproject is used to accomplish
    // this. See the accompanying documentation for more explanation of the math behind
    // this function.
    Ray CalculateCursorRay(const Matrix& projectionMatrix, const Matrix& viewMatrix) {
        // create 2 positions in screenspace using the cursor position. 0 is as close as
        // possible to the camera, 1 is as far away as possible.
        Vector3 nearSource(position_, 0.0f);
        Vector3 farSource(position_, 1.0f);

        // use Viewport::Unproject to tell what those two screen space positions would be
        // in world space. we'll need the projection matrix and view matrix, which we have
        // saved as member variables. we also need a world matrix, which can just be identity.
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        Vector3 nearPoint = vp.Unproject(nearSource, projectionMatrix, viewMatrix, Matrix::getIdentityProperty());
        Vector3 farPoint  = vp.Unproject(farSource,  projectionMatrix, viewMatrix, Matrix::getIdentityProperty());

        // find the direction vector that goes from the nearPoint to the farPoint and
        // normalize it...
        Vector3 direction = farPoint - nearPoint;
        direction.Normalize();

        // and then create a new ray using nearPoint as the source.
        return Ray(nearPoint, direction);
    }

protected:
    // LoadContent needs to load the cursor texture and find its center. also, we need to
    // create a SpriteBatch.
    void LoadContent() override {
        cursorTexture_.emplace(getGameProperty().getContentProperty().Load<Texture2D>("cursor"));
        textureCenter_ = Vector2((float)(cursorTexture_->getWidthProperty() / 2),
                                 (float)(cursorTexture_->getHeightProperty() / 2));

        spriteBatch_.emplace(getGraphicsDeviceProperty());

        // we want to default the cursor to start in the center of the screen
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        position_.X = (float)(vp.getXProperty() + (vp.getWidthProperty() / 2));
        position_.Y = (float)(vp.getYProperty() + (vp.getHeightProperty() / 2));

        DrawableGameComponent::LoadContent();
    }

private:
    // this spritebatch is created internally, and is used to draw the cursor.
    std::optional<SpriteBatch> spriteBatch_;

    // this is the sprite that is drawn at the current cursor position. textureCenter_ is
    // used to center the sprite when drawing.
    std::optional<Texture2D> cursorTexture_;
    Vector2 textureCenter_;

    // Position is the cursor position, and is in screen space.
    Vector2 position_;

    // Handles input for Windows.
    void UpdateWindowsInput() {
        MouseState mouseState = Mouse::GetState();
        position_.X = (float)mouseState.getXProperty();
        position_.Y = (float)mouseState.getYProperty();
    }
};

} // namespace TrianglePicking
