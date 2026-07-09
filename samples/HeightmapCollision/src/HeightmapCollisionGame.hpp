#pragma once

// Ported from HeightmapCollision.HeightmapCollisionGame (HeightmapCollision.cs). Sample
// showing how to get the height of a programmatically generated heightmap, and use it to
// keep a rolling ball (and the camera) on top of the terrain. See missing.md for the full
// port account, in particular why the terrain is built at runtime instead of via
// Content.Load<Model> (Terrain.hpp) even though the sphere is loaded normally.

#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/Model.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelMesh.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelBone.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelEffectCollection.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/GamePad.hpp>
#include <Microsoft/Xna/Framework/Input/GamePadState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <Microsoft/Xna/Framework/Input/Buttons.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>
#include <Microsoft/Xna/Framework/PlayerIndex.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/MathHelper.hpp>

#include "HeightMapInfo.hpp"
#include "Terrain.hpp"

#include <optional>
#include <string>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

namespace HeightmapCollisionSample {

class HeightmapCollisionGame : public Game {
public:
    HeightmapCollisionGame() {
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        getContentProperty().setRootDirectoryProperty("Content");
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "HeightmapCollisionGame";
        return name;
    }

protected:
    void Initialize() override {
        // now that the GraphicsDevice has been created, we can create the projection matrix.
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        projectionMatrix_ = Matrix::CreatePerspectiveFieldOfView(
            MathHelper::ToRadians(45.0f), vp.getAspectRatioProperty(), 1.0f, 10000.0f);

        Game::Initialize();
    }

    void LoadContent() override {
        terrain_.Load(getContentProperty(), getGraphicsDeviceProperty());

        sphere_.emplace(getContentProperty().Load<Model>("sphere"));

        spriteBatch_.emplace(getGraphicsDeviceProperty());
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        // F1 help overlay
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput();

        UpdateCamera();

        Game::Update(gameTime);
    }

    // This is called when the game should draw itself.
    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(0, 0, 0, 255)); // Black

        // NOXNA: drawing sprites (the F1 help overlay) changes some render states around,
        // which don't play nicely with 3d models -- reset before drawing the scene, matching
        // PickingSample/TrianglePicking's own defensive precedent (this sample's original
        // never draws any 2D sprites at all, so the C# source has nothing analogous here).
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::Default);
        getGraphicsDeviceProperty().setBlendStateProperty(BlendState::Opaque);

        terrain_.Draw(viewMatrix_, projectionMatrix_);

        DrawModel(*sphere_, sphereRollingMatrix_ * Matrix::CreateTranslation(spherePosition_));

        // If there was any alpha blended translucent geometry in the scene, that would be
        // drawn here.

        DrawHelpOverlay();

        Game::Draw(gameTime);
    }

private:
    // This constant controls how quickly the sphere can move forward and backward.
    static constexpr float SphereVelocity = 2.0f;

    // how quickly the sphere can turn from side to side.
    static constexpr float SphereTurnSpeed = 0.025f;

    // the radius of the sphere. We'll use this to keep the sphere above the ground, and when
    // computing how far the sphere has rolled.
    static constexpr float SphereRadius = 12.0f;

    // This vector controls how much the camera's position is offset from the sphere. This
    // value can be changed to move the camera further away from or closer to the sphere.
    static inline const Vector3 CameraPositionOffset = Vector3(0.0f, 40.0f, 150.0f);

    // This value controls the point the camera will aim at. This value is an offset from the
    // sphere's position.
    static inline const Vector3 CameraTargetOffset = Vector3(0.0f, 30.0f, 0.0f);

    std::unique_ptr<GraphicsDeviceManager> graphics_;

    // NOXNA: the C# original's `terrain` field is a Model loaded via Content.Load<Model> and
    // its `heightMapInfo` field is read back from that Model's Tag. CNA has no Model.Tag/
    // custom-ContentProcessor equivalent (DEFERRED.md item #18), so both are produced
    // together by this one runtime helper instead -- see Terrain.hpp.
    Terrain terrain_;

    Matrix projectionMatrix_;
    Matrix viewMatrix_;

    Vector3 spherePosition_;
    float sphereFacingDirection_ = 0.0f;
    Matrix sphereRollingMatrix_ = Matrix::getIdentityProperty();

    std::optional<Model> sphere_;

    std::optional<SpriteBatch> spriteBatch_;
    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

    // this function will calculate the camera's position and the position of its target.
    // From those, we'll update the viewMatrix.
    void UpdateCamera() {
        // The camera's position depends on the sphere's facing direction: when the sphere
        // turns, the camera needs to stay behind it. So, we'll calculate a rotation matrix
        // using the sphere's facing direction, and use it to transform the two offset values
        // that control the camera.
        Matrix cameraFacingMatrix = Matrix::CreateRotationY(sphereFacingDirection_);
        Vector3 positionOffset = Vector3::Transform(CameraPositionOffset, cameraFacingMatrix);
        Vector3 targetOffset   = Vector3::Transform(CameraTargetOffset, cameraFacingMatrix);

        // once we've transformed the camera's position offset vector, it's easy to figure
        // out where we think the camera should be.
        Vector3 cameraPosition = spherePosition_ + positionOffset;

        // We don't want the camera to go beneath the heightmap, so if the camera is over the
        // terrain, we'll move it up.
        if (terrain_.GetHeightMapInfo().IsOnHeightmap(cameraPosition)) {
            // we don't want the camera to go beneath the terrain's height + a small offset.
            float minimumHeight =
                terrain_.GetHeightMapInfo().GetHeight(cameraPosition) + CameraPositionOffset.Y;

            if (cameraPosition.Y < minimumHeight) {
                cameraPosition.Y = minimumHeight;
            }
        }

        // next, we need to calculate the point that the camera is aiming at. That's simple
        // enough -- the camera is aiming at the sphere, and has to take the targetOffset into
        // account.
        Vector3 cameraTarget = spherePosition_ + targetOffset;

        // with those values, we'll calculate the viewMatrix.
        viewMatrix_ = Matrix::CreateLookAt(cameraPosition, cameraTarget, Vector3::Up);
    }

    // NOXNA helper: CNA's ModelTypeReader (ContentManager.cpp) builds one synthetic "Root"
    // ModelBone for every .model.json but never assigns any mesh's ParentBone to it
    // (ModelMesh has no setter for it at all -- see DEFERRED.md item #6's "multi-bone
    // rigid-part" note), so mesh->getParentBoneProperty() is always nullptr here. CNA's own
    // Model::Draw() already works around this the same way (falls back to bone index 0); the
    // sphere in this sample is logically single-bone, so index 0 is exactly the right
    // fallback, not a guess. Same pattern already established in PickingSample/TrianglePicking.
    static int BoneIndexOf(const ModelMesh* mesh) {
        return mesh->getParentBoneProperty() ? mesh->getParentBoneProperty()->getIndexProperty() : 0;
    }

    // Helper for drawing the sphere model.
    void DrawModel(Model& model, const Matrix& worldMatrix) {
        std::vector<Matrix> boneTransforms(static_cast<std::size_t>(model.getBonesProperty().getCountProperty()));
        model.CopyAbsoluteBoneTransformsTo(boneTransforms);

        for (ModelMesh* mesh : model.getMeshesProperty()) {
            for (Effect* effect : mesh->getEffectsPropertyMutable()) {
                auto* basicEffect = static_cast<BasicEffect*>(effect);

                basicEffect->World      = boneTransforms[static_cast<std::size_t>(BoneIndexOf(mesh))] * worldMatrix;
                basicEffect->View       = viewMatrix_;
                basicEffect->Projection = projectionMatrix_;

                basicEffect->EnableDefaultLighting();
                basicEffect->setPreferPerPixelLightingProperty(true);

                // Set the fog to match the black background color.
                basicEffect->setFogEnabledProperty(true);
                basicEffect->setFogColorProperty(Vector3::Zero);
                basicEffect->setFogStartProperty(1000.0f);
                basicEffect->setFogEndProperty(3200.0f);
            }

            mesh->Draw();
        }
    }

    // Handles input for moving the sphere and quitting the game.
    void HandleInput() {
        KeyboardState currentKeyboardState = Keyboard::GetState();
        GamePadState currentGamePadState = GamePad::GetState(PlayerIndex::One);

        // Check for exit.
        if (currentKeyboardState.IsKeyDown(Keys::Escape) ||
            currentGamePadState.IsButtonDown(Buttons::Back)) {
            Exit();
        }

        // Now move the sphere. First, we want to check to see if the sphere should turn.
        // turnAmount will be an accumulation of all the different possible inputs.
        float turnAmount = -currentGamePadState.getThumbSticksProperty().getLeftProperty().X;
        if (currentKeyboardState.IsKeyDown(Keys::A) ||
            currentKeyboardState.IsKeyDown(Keys::Left) ||
            currentGamePadState.getDPadProperty().getLeftProperty() == ButtonState::Pressed) {
            turnAmount += 1;
        }
        if (currentKeyboardState.IsKeyDown(Keys::D) ||
            currentKeyboardState.IsKeyDown(Keys::Right) ||
            currentGamePadState.getDPadProperty().getRightProperty() == ButtonState::Pressed) {
            turnAmount -= 1;
        }

        // clamp the turn amount between -1 and 1, and then use the finished value to turn
        // the sphere.
        turnAmount = MathHelper::Clamp(turnAmount, -1.0f, 1.0f);
        sphereFacingDirection_ += turnAmount * SphereTurnSpeed;

        // Next, we want to move the sphere forward or back. To do this, we'll create a
        // Vector3 and use the user's input to modify the Z component, which corresponds to
        // the forward direction.
        Vector3 movement = Vector3::Zero;
        movement.Z = -currentGamePadState.getThumbSticksProperty().getLeftProperty().Y;

        if (currentKeyboardState.IsKeyDown(Keys::W) ||
            currentKeyboardState.IsKeyDown(Keys::Up) ||
            currentGamePadState.getDPadProperty().getUpProperty() == ButtonState::Pressed) {
            movement.Z = -1;
        }
        if (currentKeyboardState.IsKeyDown(Keys::S) ||
            currentKeyboardState.IsKeyDown(Keys::Down) ||
            currentGamePadState.getDPadProperty().getDownProperty() == ButtonState::Pressed) {
            movement.Z = 1;
        }

        // next, we'll create a rotation matrix from the sphereFacingDirection, and use it to
        // transform the vector. If we didn't do this, pressing "up" would always move the
        // ball along +Z. By transforming it, we can move in the direction the sphere is
        // "facing."
        Matrix sphereFacingMatrix = Matrix::CreateRotationY(sphereFacingDirection_);
        Vector3 velocity = Vector3::Transform(movement, sphereFacingMatrix) * SphereVelocity;

        // Now we know how much the user wants to move. We'll construct a temporary vector,
        // newSpherePosition, which will represent where the user wants to go. If that value
        // is on the heightmap, we'll allow the move.
        Vector3 newSpherePosition = spherePosition_ + velocity;
        if (terrain_.GetHeightMapInfo().IsOnHeightmap(newSpherePosition)) {
            // finally, we need to see how high the terrain is at the sphere's new position.
            // GetHeight will give us that information, which is offset by the size of the
            // sphere. If we didn't offset by the size of the sphere, it would be drawn
            // halfway through the world, which looks a little odd.
            newSpherePosition.Y = terrain_.GetHeightMapInfo().GetHeight(newSpherePosition) + SphereRadius;
        } else {
            newSpherePosition = spherePosition_;
        }

        // now we need to roll the ball "forward." To do this, we first calculate how far it
        // has moved.
        float distanceMoved = Vector3::Distance(spherePosition_, newSpherePosition);

        // The length of an arc on a circle or sphere is defined as L = theta * r, where theta
        // is the angle that defines the arc, and r is the radius of the circle. we know L,
        // that's the distance the sphere has moved. we know r, that's our constant
        // "SphereRadius". We want to know theta -- that will tell us how much to rotate the
        // sphere. we rearrange the equation to get...
        float theta = distanceMoved / SphereRadius;

        // now that we know how much to rotate the sphere, we have to figure out whether it
        // will roll forward or backward. We'll base this on the user's input.
        int rollDirection = movement.Z > 0 ? 1 : -1;

        // finally, we'll roll it by rotating around the sphere's "right" vector.
        sphereRollingMatrix_ = sphereRollingMatrix_ *
            Matrix::CreateFromAxisAngle(sphereFacingMatrix.getRightProperty(), theta * rollDirection);

        // once we've finished all computations, we can set spherePosition to the new
        // position that we calculated.
        spherePosition_ = newSpherePosition;
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

} // namespace HeightmapCollisionSample
