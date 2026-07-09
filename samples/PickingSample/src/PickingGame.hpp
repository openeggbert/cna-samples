#pragma once

// Ported from PickingSample.PickingSampleGame (Game.cs). This sample shows how to see if a
// user's cursor is over an object, and how to find out where on the screen an object is. It
// puts several objects (Sphere/Cats/P2Wedge/Cylinder) on a table; when the cursor is over an
// object, that object's name is displayed above it -- see missing.md for the full port account.

#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteFont.hpp>
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
#include <Microsoft/Xna/Framework/BoundingSphere.hpp>
#include <Microsoft/Xna/Framework/Ray.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/MathHelper.hpp>

#include "BoundingSphereRenderer.hpp"
#include "Cursor.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

namespace PickingSample {

class PickingGame : public Game {
public:
    PickingGame() {
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        getContentProperty().setRootDirectoryProperty("Content");
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "PickingGame";
        return name;
    }

protected:
    void Initialize() override {
        // Set up the world transforms that each model will use. They'll be positioned in
        // a line along the x axis.
        modelWorldTransforms_[0] = Matrix::CreateTranslation(Vector3(-1.5f, 0.0f, 0.0f));
        modelWorldTransforms_[1] = Matrix::CreateTranslation(Vector3(-0.5f, 0.0f, 0.0f));
        modelWorldTransforms_[2] = Matrix::CreateTranslation(Vector3(0.5f, 0.0f, 0.0f));
        modelWorldTransforms_[3] = Matrix::CreateTranslation(Vector3(1.5f, 0.0f, 0.0f));

        cursor_ = std::make_unique<Cursor>(*this);
        AddComponent(cursor_.get());

        Game::Initialize();
    }

    void LoadContent() override {
        // load all of the models that will appear on the table:
        for (std::size_t i = 0; i < ModelFilenames.size(); ++i) {
            models_.push_back(getContentProperty().Load<Model>(ModelFilenames[i]));

            std::vector<Matrix> boneTransforms(models_[i].getBonesProperty().getCountProperty());
            models_[i].CopyAbsoluteBoneTransformsTo(boneTransforms);
            modelAbsoluteBoneTransforms_.push_back(std::move(boneTransforms));
        }

        // now that we've loaded in the models that will sit on the table, go through the
        // same procedure for the table itself. (Note: the original loads this asset via
        // Content.Load<Model>("Table") -- capital T -- even though its source FBX is
        // "table.FBX"/its content-item Name is lowercase "table"; this only worked in the
        // original because Windows content loading is case-insensitive. Kept the identical
        // "Table" string here and named the converted asset files to match -- see missing.md.)
        table_.emplace(getContentProperty().Load<Model>("Table"));
        tableAbsoluteBoneTransforms_.resize(table_->getBonesProperty().getCountProperty());
        table_->CopyAbsoluteBoneTransformsTo(tableAbsoluteBoneTransforms_);

        // create a spritebatch and load the font, which we'll use to draw the models' names.
        spriteBatch_.emplace(getGraphicsDeviceProperty());
        spriteFont_.emplace(getContentProperty().Load<SpriteFont>("hudFont"));

        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        // calculate the projection matrix now that the graphics device is created.
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        projectionMatrix_ = Matrix::CreatePerspectiveFieldOfView(
            MathHelper::PiOver4, vp.getAspectRatioProperty(), 0.01f, 1000.0f);

        // Initialize the renderer for our bounding spheres
        boundingSphereRenderer_.Initialize(getGraphicsDeviceProperty(), 45);
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        // F1 help overlay
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        // Check for exit.
        if (Keyboard::GetState().IsKeyDown(Keys::Escape) ||
            GamePad::GetState(PlayerIndex::One).getButtonsProperty().getBackProperty() == ButtonState::Pressed) {
            Exit();
        }

        // we rotate our view around the models over time
        float timeMs = (float)gameTime.getElapsedGameTimeProperty().getTotalMillisecondsProperty();
        cameraRotation_ += timeMs * CameraRotateSpeed;
        Matrix unrotatedView = Matrix::CreateLookAt(
            Vector3(0.0f, 0.0f, -CameraDefaultDistance), Vector3::Zero, Vector3::Up);
        viewMatrix_ = Matrix::CreateRotationY(MathHelper::ToRadians(cameraRotation_)) *
                      Matrix::CreateRotationX(MathHelper::ToRadians(CameraDefaultArc)) *
                      unrotatedView;

        // base.Update will update all of the components in the .Components collection,
        // including the cursor.
        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255)); // CornflowerBlue

        // drawing sprites changes some render states around, which don't play nicely with
        // 3d models. In particular, we want to enable the depth buffer and turn off alpha
        // blending.
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::Default);
        getGraphicsDeviceProperty().setBlendStateProperty(BlendState::Opaque);

        // draw the table. DrawModel draws a model using a world matrix and the model's
        // bone transforms.
        DrawModel(*table_, Matrix::getIdentityProperty(), tableAbsoluteBoneTransforms_, false);

        // use the same DrawModel function to draw all of the models on the table.
        for (std::size_t i = 0; i < models_.size(); ++i) {
            DrawModel(models_[i], modelWorldTransforms_[i], modelAbsoluteBoneTransforms_[i],
                      drawBoundingSphere_);
        }

        // now we'll check to see if the cursor is over any of the models, and draw their
        // names if it is.
        DrawModelNames();

        DrawHelpOverlay();

        Game::Draw(gameTime);
    }

private:
    // ModelFilenames is the list of models that we will be putting on top of the table.
    // These strings will be used as arguments to content.Load<Model> and will be drawn
    // when the cursor is over an object.
    static inline const std::array<std::string, 4> ModelFilenames = {
        "Sphere", "Cats", "P2Wedge", "Cylinder",
    };

    // the following constants control the speed at which the camera moves: how fast does
    // the camera move up, down, left, and right?
    static constexpr float CameraRotateSpeed = 0.01f;

    // the following constants control the camera's default position
    static constexpr float CameraDefaultArc = -15.0f;
    static constexpr float CameraDefaultRotation = 185.0f;
    static constexpr float CameraDefaultDistance = 4.3f;

    std::unique_ptr<GraphicsDeviceManager> graphics_;

    // a SpriteBatch and SpriteFont, which we will use to draw the objects' names when
    // they are selected.
    std::optional<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont> spriteFont_;

    // The cursor is used to tell what the user's pointer/mouse is over.
    std::unique_ptr<Cursor> cursor_;

    // the table that all of the objects are drawn on, and the table model's absolute bone
    // transforms. Since the table is not animated, these can be calculated once and saved.
    std::optional<Model> table_;
    std::vector<Matrix> tableAbsoluteBoneTransforms_;

    // these are the models that we will draw on top of the table, along with their bone
    // transforms and a world transform placing each model at a different location.
    std::vector<Model> models_;
    std::vector<std::vector<Matrix>> modelAbsoluteBoneTransforms_;
    std::array<Matrix, 4> modelWorldTransforms_;

    Matrix viewMatrix_;
    Matrix projectionMatrix_;

    // this variable stores the current rotation value as the camera rotates around the scene
    float cameraRotation_ = CameraDefaultRotation;

    // this variable tells our game whether or not to draw a mesh's bounding sphere
    bool drawBoundingSphere_ = true;

    BoundingSphereRenderer boundingSphereRenderer_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;

    // NOXNA workaround for a CNA lifecycle gap (DEFERRED.md item #23):
    // Game::DoInitialize() only wires up Components_'s ComponentAdded event *after*
    // calling the user's own Initialize() override, so a component added to Components
    // from within Initialize() -- exactly what the C# original does for `cursor` -- never
    // gets its own Initialize()/LoadContent() called via that event, leaving its optionals
    // unset and crashing on first Draw()/Update(). Real XNA/FNA wires this subscription in
    // the Game constructor itself, before Initialize() can run. Calling Initialize()
    // explicitly here is safe even if the framework is later fixed to also call it, since
    // DrawableGameComponent::Initialize() already guards against double-initialization.
    // See missing.md and Graphics3D's identical workaround.
    void AddComponent(Cursor* component) {
        getComponentsProperty().Add(component);
        component->Initialize();
    }

    // Handles input for quitting the game.
    void DrawModelNames() {
        // begin on the spritebatch, because we're going to be drawing some text.
        spriteBatch_->Begin();

        // If the cursor is over a model, we'll draw its name. To figure out if the cursor
        // is over a model, we'll use cursor.CalculateCursorRay. That function gives us a
        // world space ray that starts at the "eye" of the camera, and shoots out in the
        // direction pointed to by the cursor.
        Ray cursorRay = cursor_->CalculateCursorRay(projectionMatrix_, viewMatrix_);

        // go through all of the models...
        for (std::size_t i = 0; i < models_.size(); ++i) {
            // check to see if the cursorRay intersects the model....
            if (RayIntersectsModel(cursorRay, models_[i], modelWorldTransforms_[i],
                                   modelAbsoluteBoneTransforms_[i])) {

                // now we know that we want to draw the model's name. We want to draw the
                // name a little bit above the model: but where's that? SpriteBatch::DrawString
                // takes screen space coordinates, but the model's position is stored in
                // world space.

                // we'll use Viewport::Project, which will project a world space point into
                // screen space. We'll project the vector (0,0,0) using the model's world
                // matrix, and the view and projection matrices. That will tell us where
                // the model's origin is on the screen.
                Vector3 screenSpace = getGraphicsDeviceProperty().getViewportProperty().Project(
                    Vector3::Zero, projectionMatrix_, viewMatrix_, modelWorldTransforms_[i]);

                // we want to draw the text a little bit above that, so we'll use the
                // screen space position - 60 to move up a little bit. A better approach
                // would be to calculate where the top of the model is, and draw there.
                // It's not that much harder to do, but to keep the sample easy, we'll take
                // the easy way out.
                Vector2 textPosition(screenSpace.X, screenSpace.Y - 60.0f);

                // we want to draw the text centered around textPosition, so we'll
                // calculate the center of the string, and use that as the origin argument
                // to spriteBatch.DrawString. DrawString automatically centers text around
                // the vector specified by the origin argument.
                Vector2 stringCenter = spriteFont_->MeasureString(ModelFilenames[i]) / 2.0f;

                // to make the text readable, we'll draw the same thing twice, once white
                // and once black, with a little offset to get a drop shadow effect.

                // first we'll draw the shadow...
                Vector2 shadowOffset(1.0f, 1.0f);
                spriteBatch_->DrawString(*spriteFont_, ModelFilenames[i],
                    textPosition + shadowOffset, Color::Black, 0.0f,
                    stringCenter, 1.0f, SpriteEffects::None, 0.0f);

                // ...and then the real text on top.
                spriteBatch_->DrawString(*spriteFont_, ModelFilenames[i],
                    textPosition, Color::White, 0.0f,
                    stringCenter, 1.0f, SpriteEffects::None, 0.0f);
            }
        }

        spriteBatch_->End();
    }

    // NOXNA helper: CNA's ModelTypeReader (ContentManager.cpp) builds one synthetic
    // "Root" ModelBone for every .model.json but never assigns any mesh's ParentBone to
    // it (ModelMesh has no setter for it at all -- see DEFERRED.md item #6's "multi-bone
    // rigid-part" note), so mesh->getParentBoneProperty() is always nullptr for every
    // model in this sample. CNA's own Model::Draw() already works around this the same
    // way (falls back to bone index 0); every model here is logically single-bone
    // (CopyAbsoluteBoneTransformsTo produces exactly one "Root" transform), so index 0
    // is exactly the right fallback, not a guess. See missing.md.
    static int BoneIndexOf(const ModelMesh* mesh) {
        return mesh->getParentBoneProperty() ? mesh->getParentBoneProperty()->getIndexProperty() : 0;
    }

    // DrawModel is a helper function that takes a model, world matrix, and bone
    // transforms. It does just what its name implies, and draws the model.
    void DrawModel(Model& model, const Matrix& worldTransform,
                   const std::vector<Matrix>& absoluteBoneTransforms, bool drawBoundingSphere) {
        // nothing tricky in here; this is the same model drawing code that we see
        // everywhere. we'll loop over all of the meshes in the model, set up their
        // effects, and then draw them.
        for (ModelMesh* mesh : model.getMeshesProperty()) {
            for (Effect* effect : mesh->getEffectsPropertyMutable()) {
                auto* basicEffect = static_cast<BasicEffect*>(effect);
                basicEffect->EnableDefaultLighting();

                basicEffect->View = viewMatrix_;
                basicEffect->Projection = projectionMatrix_;
                basicEffect->World = absoluteBoneTransforms[BoneIndexOf(mesh)] * worldTransform;
            }
            mesh->Draw();

            if (drawBoundingSphere) {
                // the mesh's BoundingSphere is stored relative to the mesh itself (mesh
                // space). We want to get this BoundingSphere in terms of world
                // coordinates. To do this, we calculate a matrix that will transform from
                // mesh space into world space....
                Matrix world = absoluteBoneTransforms[BoneIndexOf(mesh)] * worldTransform;

                // ... and then transform the BoundingSphere using that matrix.
                BoundingSphere sphere = TransformBoundingSphere(mesh->getBoundingSphereProperty(), world);

                // now draw the sphere with our renderer
                boundingSphereRenderer_.Draw(sphere, viewMatrix_, projectionMatrix_);
            }
        }
    }

    // This helper function checks to see if a ray will intersect with a model. The
    // model's bounding spheres are used, and the model is transformed using the matrix
    // specified in the worldTransform argument.
    static bool RayIntersectsModel(const Ray& ray, Model& model, const Matrix& worldTransform,
                                    const std::vector<Matrix>& absoluteBoneTransforms) {
        // Each ModelMesh in a Model has a bounding sphere, so to check for an
        // intersection in the Model, we have to check every mesh.
        for (ModelMesh* mesh : model.getMeshesProperty()) {
            // the mesh's BoundingSphere is stored relative to the mesh itself (mesh
            // space). We want to get this BoundingSphere in terms of world coordinates.
            // To do this, we calculate a matrix that will transform from mesh space into
            // world space....
            Matrix world = absoluteBoneTransforms[BoneIndexOf(mesh)] * worldTransform;

            // ... and then transform the BoundingSphere using that matrix.
            BoundingSphere sphere = TransformBoundingSphere(mesh->getBoundingSphereProperty(), world);

            // now that we have a sphere in world coordinates, we can just use the
            // BoundingSphere class's Intersects function. Intersects returns a nullable
            // float (std::optional<float>). This value is the distance at which the ray
            // intersects the BoundingSphere, or empty if there is no intersection. So, if
            // the value is not empty, we have a collision.
            if (sphere.Intersects(ray).has_value()) {
                return true;
            }
        }

        // if we've gotten this far, we've made it through every BoundingSphere, and none
        // of them intersected the ray. This means that there was no collision, and we
        // should return false.
        return false;
    }

    // This helper function takes a BoundingSphere and a transform matrix, and returns a
    // transformed version of that BoundingSphere.
    static BoundingSphere TransformBoundingSphere(const BoundingSphere& sphere, const Matrix& transform) {
        BoundingSphere transformedSphere;

        // the transform can contain different scales on the x, y, and z components. this
        // has the effect of stretching and squishing our bounding sphere along different
        // axes. Obviously, this is no good: a bounding sphere has to be a SPHERE. so, the
        // transformed sphere's radius must be the maximum of the scaled x, y, and z radii.

        // to calculate how the transform matrix will affect the x, y, and z components of
        // the sphere, we'll create a vector3 with x y and z equal to the sphere's radius...
        Vector3 scale3(sphere.Radius, sphere.Radius, sphere.Radius);

        // then transform that vector using the transform matrix. we use TransformNormal
        // because we don't want to take translation into account.
        scale3 = Vector3::TransformNormal(scale3, transform);

        // scale3 contains the x, y, and z radii of a squished and stretched sphere. we'll
        // set the finished sphere's radius to the maximum of the x y and z radii,
        // creating a sphere that is large enough to contain the original squished sphere.
        transformedSphere.Radius = std::max(scale3.X, std::max(scale3.Y, scale3.Z));

        // transforming the center of the sphere is much easier. we can just use
        // Vector3::Transform to transform the center vector. notice that we're using
        // Transform instead of TransformNormal because in this case we DO want to take
        // translation into account.
        transformedSphere.Center = Vector3::Transform(sphere.Center, transform);

        return transformedSphere;
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

} // namespace PickingSample
