#pragma once

// Ported from TrianglePickingSample.TrianglePickingGame (Game.cs). This sample shows
// how to implement per-triangle mesh picking: a mouse ray is first tested against each
// on-table model's bounding sphere (a fast reject), and only if that passes is the ray
// tested against every individual triangle in the model (a Moller-Trumbore ray-triangle
// intersection test), so the exact triangle under the cursor can be outlined in
// magenta wireframe. Close sibling of PickingSample (#047, same original author,
// several shared FBX assets -- see missing.md) but replaces its simpler per-object
// BoundingSphere-only test with real per-triangle geometry.
//
// The C# original gets its per-triangle vertex data from a Dictionary attached to
// Model.Tag by a custom content-pipeline processor (TrianglePickingProcessor) at
// content-build time. CNA has neither Model.Tag/custom-ContentProcessor extensibility
// (DEFERRED.md item #18) nor a way to read vertex/index data back out of a
// VertexBuffer/IndexBuffer at runtime (confirmed: both classes only expose SetData(),
// never GetData()) -- so this port instead loads a small sidecar binary file
// (<Model>_picking.bin), generated once offline by tools/fbx_ascii2model.py's new
// --picking option from the exact same source data. See TrianglePickingData.hpp and
// missing.md for the full account.

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
#include <Microsoft/Xna/Framework/Graphics/RasterizerState.hpp>
#include <Microsoft/Xna/Framework/Graphics/CullMode.hpp>
#include <Microsoft/Xna/Framework/Graphics/FillMode.hpp>
#include <Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp>
#include <Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp>
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

#include <System/Single.hpp>

#include "Cursor.hpp"
#include "TrianglePickingData.hpp"

#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

namespace TrianglePicking {

class TrianglePickingGame : public Game {
public:
    TrianglePickingGame() {
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        getContentProperty().setRootDirectoryProperty("Content");

        // Set up the world transforms that each model will use. They'll be positioned
        // in a line along the x axis.
        for (std::size_t i = 0; i < ModelFilenames.size(); ++i) {
            float x = static_cast<float>(static_cast<int>(i) - static_cast<int>(ModelFilenames.size()) / 2);
            modelWorldTransforms_[i] = Matrix::CreateTranslation(Vector3(x, 0.0f, 0.0f));
        }

        // NOTE: unlike PickingSample/Graphics3D, this sample's C# original adds its
        // Cursor component from the *constructor* (Game.cs's own TrianglePickingGame()),
        // not from Initialize() -- so it doesn't hit DEFERRED.md item #23 (Components_
        // .ComponentAdded wired up after Initialize() runs): cna's own base
        // Game::Initialize() (Game.cpp) separately loops over every component already
        // present in Components_ and calls each one's Initialize() directly, and since
        // this component is added here (well before DoInitialize() ever runs), it is
        // already present by the time Game::Initialize() makes that pass. Confirmed
        // live -- see missing.md for the full account of why this differs from the
        // other two samples that hit item #23.
        cursor_ = std::make_unique<Cursor>(*this);
        getComponentsProperty().Add(cursor_.get());
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "TrianglePickingGame";
        return name;
    }

protected:
    void Initialize() override {
        // now that the GraphicsDevice has been created, we can create the projection matrix.
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        projectionMatrix_ = Matrix::CreatePerspectiveFieldOfView(
            MathHelper::ToRadians(45.0f), vp.getAspectRatioProperty(), 0.01f, 1000.0f);

        Game::Initialize();
    }

    void LoadContent() override {
        // load all of the models that will appear on the table:
        for (std::size_t i = 0; i < ModelFilenames.size(); ++i) {
            models_.push_back(getContentProperty().Load<Model>(ModelFilenames[i]));

            std::vector<Matrix> boneTransforms(models_[i].getBonesProperty().getCountProperty());
            models_[i].CopyAbsoluteBoneTransformsTo(boneTransforms);
            modelAbsoluteBoneTransforms_.push_back(std::move(boneTransforms));

            // Load this model's per-triangle picking data (see TrianglePickingData.hpp)
            // and precompute its bounding sphere, mirroring what the C# original's
            // TrianglePickingProcessor computes once at content-build time and stores
            // in Model.Tag (BoundingSphere.CreateFromPoints(vertices)).
            std::vector<Vector3> pickingVerts = LoadPickingVertices(
                getContentProperty().getRootDirectoryProperty() + "/" + ModelFilenames[i] + "_picking.bin");
            pickingBoundingSpheres_.push_back(BoundingSphere::CreateFromPoints(pickingVerts));
            pickingVertices_.push_back(std::move(pickingVerts));
        }

        // now that we've loaded in the models that will sit on the table, go through the
        // same procedure for the table itself. (Note: the original loads this asset via
        // Content.Load<Model>("Table") -- capital T -- even though its source FBX is
        // "table.FBX"/its content-item Name is lowercase "table"; kept the identical
        // "Table" string here and named the converted asset files to match, same
        // precedent as PickingSample -- see missing.md.)
        table_.emplace(getContentProperty().Load<Model>("Table"));
        tableAbsoluteBoneTransforms_.resize(table_->getBonesProperty().getCountProperty());
        table_->CopyAbsoluteBoneTransformsTo(tableAbsoluteBoneTransforms_);

        // create a spritebatch and load the font, which we'll use to draw the models' names.
        spriteBatch_.emplace(getGraphicsDeviceProperty());
        spriteFont_.emplace(getContentProperty().Load<SpriteFont>("hudFont"));

        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        // create the effect for drawing the picked triangle.
        lineEffect_.emplace(getGraphicsDeviceProperty());
        lineEffect_->VertexColorEnabled = true;

        // Custom rasterizer state for drawing in wireframe.
        wireFrameState_.setFillModeProperty(FillMode::WireFrame);
        wireFrameState_.setCullModeProperty(CullMode::None);
    }

    void Update(GameTime& gameTime) override {
        // F1 help overlay (CNA addition, see CLAUDE.md).
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput();
        UpdateCamera(gameTime);
        UpdatePicking();

        // base.Update will update all of the components in the .Components collection,
        // including the cursor.
        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        GraphicsDevice& device = getGraphicsDeviceProperty();

        device.Clear(Color(100, 149, 237, 255)); // CornflowerBlue

        device.setBlendStateProperty(BlendState::Opaque);
        device.setDepthStencilStateProperty(DepthStencilState::Default);

        // Draw the table.
        DrawModel(*table_, Matrix::getIdentityProperty(), tableAbsoluteBoneTransforms_);

        // Use the same DrawModel function to draw all of the models on the table.
        for (std::size_t i = 0; i < models_.size(); ++i) {
            DrawModel(models_[i], modelWorldTransforms_[i], modelAbsoluteBoneTransforms_[i]);
        }

        // Draw the outline of the triangle under the cursor.
        DrawPickedTriangle();

        // Draw text describing the picking results.
        DrawText();

        DrawHelpOverlay();

        Game::Draw(gameTime);
    }

private:
    // ModelFilenames is the list of models that we will be putting on top of the
    // table. These strings will be used as arguments to content.Load<Model> and will
    // be drawn (and picking-tested) at all times.
    static inline const std::array<std::string, 4> ModelFilenames = {
        "Sphere", "Cats", "P2Wedge", "Cylinder",
    };

    // the following constants control the speed at which the camera moves.
    static constexpr float CameraRotateSpeed = 0.1f;
    static constexpr float CameraZoomSpeed = 0.01f;
    static constexpr float CameraMaxDistance = 10.0f;
    static constexpr float CameraMinDistance = 1.2f;

    // the following constants control the camera's default position.
    static constexpr float CameraDefaultArc = -30.0f;
    static constexpr float CameraDefaultRotation = 225.0f;
    static constexpr float CameraDefaultDistance = 3.5f;

    std::unique_ptr<GraphicsDeviceManager> graphics_;

    // the current input states, updated in HandleInput and used in UpdateCamera.
    KeyboardState currentKeyboardState_;
    GamePadState currentGamePadState_;

    // a SpriteBatch and SpriteFont, used to draw the objects' names when selected.
    std::optional<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont> spriteFont_;

    // The cursor is used to tell what the user's pointer/mouse is over.
    std::unique_ptr<Cursor> cursor_;

    // the table that all of the objects are drawn on, and its absolute bone
    // transforms. Since the table isn't animated, these are calculated once.
    std::optional<Model> table_;
    std::vector<Matrix> tableAbsoluteBoneTransforms_;

    // the models drawn on top of the table, their bone transforms, and a world
    // transform placing each model at a different location.
    std::vector<Model> models_;
    std::vector<std::vector<Matrix>> modelAbsoluteBoneTransforms_;
    std::array<Matrix, 4> modelWorldTransforms_;

    // per-model per-triangle picking data (see TrianglePickingData.hpp) and a
    // precomputed bounding sphere over each model's picking vertices -- the NOXNA
    // stand-in for the C# original's Model.Tag dictionary (see missing.md).
    std::vector<std::vector<Vector3>> pickingVertices_;
    std::vector<BoundingSphere> pickingBoundingSpheres_;

    // arc-ball camera state.
    float cameraArc_ = CameraDefaultArc;
    float cameraRotation_ = CameraDefaultRotation;
    float cameraDistance_ = CameraDefaultDistance;
    Matrix viewMatrix_;
    Matrix projectionMatrix_;

    // Which models passed the initial (fast) bounding-sphere test this frame --
    // lets you see the difference between that approximation and the more accurate
    // per-triangle test.
    std::vector<std::string> insideBoundingSpheres_;

    // Name of the model underneath the cursor (per-triangle test), if any.
    std::optional<std::string> pickedModelName_;

    // Vertex array storing exactly which triangle was picked, for wireframe drawing.
    std::array<VertexPositionColor, 3> pickedTriangle_ = {
        VertexPositionColor(Vector3::Zero, Color::Magenta),
        VertexPositionColor(Vector3::Zero, Color::Magenta),
        VertexPositionColor(Vector3::Zero, Color::Magenta),
    };

    // Effect for drawing the picked triangle.
    std::optional<BasicEffect> lineEffect_;

    // Custom rasterizer state for drawing in wireframe.
    RasterizerState wireFrameState_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;

    // ---------------------------------------------------------------------------
    // Update
    // ---------------------------------------------------------------------------

    // Handles input for quitting the game.
    void HandleInput() {
        currentKeyboardState_ = Keyboard::GetState();
        currentGamePadState_ = GamePad::GetState(PlayerIndex::One);

        if (currentKeyboardState_.IsKeyDown(Keys::Escape) ||
            currentGamePadState_.getButtonsProperty().getBackProperty() == ButtonState::Pressed) {
            Exit();
        }
    }

    // Handles input for moving the camera.
    void UpdateCamera(GameTime& gameTime) {
        float time = (float)gameTime.getElapsedGameTimeProperty().getTotalMillisecondsProperty();

        // should we reset the camera?
        if (currentKeyboardState_.IsKeyDown(Keys::R) ||
            currentGamePadState_.getButtonsProperty().getRightStickProperty() == ButtonState::Pressed) {
            cameraArc_ = CameraDefaultArc;
            cameraDistance_ = CameraDefaultDistance;
            cameraRotation_ = CameraDefaultRotation;
        }

        // Check for input to rotate the camera up and down around the model.
        if (currentKeyboardState_.IsKeyDown(Keys::Up) || currentKeyboardState_.IsKeyDown(Keys::W)) {
            cameraArc_ += time * CameraRotateSpeed;
        }
        if (currentKeyboardState_.IsKeyDown(Keys::Down) || currentKeyboardState_.IsKeyDown(Keys::S)) {
            cameraArc_ -= time * CameraRotateSpeed;
        }

        cameraArc_ += currentGamePadState_.getThumbSticksProperty().getRightProperty().Y * time * CameraRotateSpeed;

        // Limit the arc movement.
        cameraArc_ = MathHelper::Clamp(cameraArc_, -90.0f, 90.0f);

        // Check for input to rotate the camera around the model.
        if (currentKeyboardState_.IsKeyDown(Keys::Right) || currentKeyboardState_.IsKeyDown(Keys::D)) {
            cameraRotation_ += time * CameraRotateSpeed;
        }
        if (currentKeyboardState_.IsKeyDown(Keys::Left) || currentKeyboardState_.IsKeyDown(Keys::A)) {
            cameraRotation_ -= time * CameraRotateSpeed;
        }

        cameraRotation_ += currentGamePadState_.getThumbSticksProperty().getRightProperty().X * time * CameraRotateSpeed;

        // Check for input to zoom camera in and out.
        if (currentKeyboardState_.IsKeyDown(Keys::Z)) cameraDistance_ += time * CameraZoomSpeed;
        if (currentKeyboardState_.IsKeyDown(Keys::X)) cameraDistance_ -= time * CameraZoomSpeed;

        cameraDistance_ += currentGamePadState_.getTriggersProperty().getLeftProperty() * time * CameraZoomSpeed;
        cameraDistance_ -= currentGamePadState_.getTriggersProperty().getRightProperty() * time * CameraZoomSpeed;

        // clamp the camera distance so it doesn't get too close or too far away.
        cameraDistance_ = MathHelper::Clamp(cameraDistance_, CameraMinDistance, CameraMaxDistance);

        Matrix unrotatedView = Matrix::CreateLookAt(
            Vector3(0.0f, 0.0f, -cameraDistance_), Vector3::Zero, Vector3::Up);

        viewMatrix_ = Matrix::CreateRotationY(MathHelper::ToRadians(cameraRotation_)) *
                      Matrix::CreateRotationX(MathHelper::ToRadians(cameraArc_)) *
                      unrotatedView;
    }

    // Runs a per-triangle picking algorithm over all the models in the scene, storing
    // which triangle is currently under the cursor.
    void UpdatePicking() {
        // Look up a collision ray based on the current cursor position.
        Ray cursorRay = cursor_->CalculateCursorRay(projectionMatrix_, viewMatrix_);

        // Clear the previous picking results.
        insideBoundingSpheres_.clear();
        pickedModelName_.reset();

        // Keep track of the closest object we have seen so far, so we can choose the
        // closest one if there are several models under the cursor.
        float closestIntersection = std::numeric_limits<float>::max();

        // Loop over all our models.
        for (std::size_t i = 0; i < models_.size(); ++i) {
            bool insideBoundingSphere = false;
            Vector3 vertex1, vertex2, vertex3;

            // Perform the ray to model intersection test.
            std::optional<float> intersection = RayIntersectsModel(
                cursorRay, i, modelWorldTransforms_[i], insideBoundingSphere, vertex1, vertex2, vertex3);

            // If this model passed the initial bounding sphere test, remember that so
            // we can display it at the top of the screen.
            if (insideBoundingSphere) {
                insideBoundingSpheres_.push_back(ModelFilenames[i]);
            }

            // Do we have a per-triangle intersection with this model?
            if (intersection.has_value() && intersection.value() < closestIntersection) {
                // If so, is it closer than any other model we might have previously
                // intersected?
                closestIntersection = intersection.value();

                pickedModelName_ = ModelFilenames[i];

                // Store vertex positions so we can display the picked triangle.
                pickedTriangle_[0].Position = vertex1;
                pickedTriangle_[1].Position = vertex2;
                pickedTriangle_[2].Position = vertex3;
            }
        }
    }

    // Checks whether a ray intersects a model. Uses the picking vertex data loaded in
    // LoadContent() (this port's stand-in for the C# original's Model.Tag dictionary --
    // see TrianglePickingData.hpp). Returns the distance along the ray to the point of
    // intersection, or empty if there is no intersection.
    std::optional<float> RayIntersectsModel(Ray ray, std::size_t modelIndex, const Matrix& modelTransform,
                                             bool& insideBoundingSphere,
                                             Vector3& vertex1, Vector3& vertex2, Vector3& vertex3) {
        vertex1 = vertex2 = vertex3 = Vector3::Zero;

        // The input ray is in world space, but our picking vertex data is stored in
        // object space. Transforming the ray by the inverse modelTransform moves it
        // into object space instead of transforming every triangle into world space --
        // there's only one ray but typically many triangles, so this is much cheaper.
        Matrix inverseTransform = Matrix::Invert(modelTransform);

        ray.Position = Vector3::Transform(ray.Position, inverseTransform);
        ray.Direction = Vector3::TransformNormal(ray.Direction, inverseTransform);

        // Start off with a fast bounding sphere test.
        const BoundingSphere& boundingSphere = pickingBoundingSpheres_[modelIndex];

        if (!boundingSphere.Intersects(ray).has_value()) {
            // If the ray does not intersect the bounding sphere, we cannot possibly
            // have picked this model, so there is no need to even bother looking at
            // the individual triangle data.
            insideBoundingSphere = false;
            return std::nullopt;
        }

        // The bounding sphere test passed, so we need to do a full triangle picking test.
        insideBoundingSphere = true;

        // Keep track of the closest triangle we found so far, so we can always return
        // the closest one.
        std::optional<float> closestIntersection;

        const std::vector<Vector3>& vertices = pickingVertices_[modelIndex];

        // Loop over the vertex data, 3 at a time (3 vertices = 1 triangle).
        for (std::size_t i = 0; i + 2 < vertices.size(); i += 3) {
            std::optional<float> intersection =
                RayIntersectsTriangle(ray, vertices[i], vertices[i + 1], vertices[i + 2]);

            // Does the ray intersect this triangle?
            if (intersection.has_value() &&
                (!closestIntersection.has_value() || intersection.value() < closestIntersection.value())) {
                // Store the distance to this triangle.
                closestIntersection = intersection;

                // Transform the three vertex positions into world space, and store
                // them into the output vertex parameters.
                vertex1 = Vector3::Transform(vertices[i], modelTransform);
                vertex2 = Vector3::Transform(vertices[i + 1], modelTransform);
                vertex3 = Vector3::Transform(vertices[i + 2], modelTransform);
            }
        }

        return closestIntersection;
    }

    // Checks whether a ray intersects a triangle. This uses the algorithm developed by
    // Tomas Moller and Ben Trumbore ("Fast, Minimum Storage Ray-Triangle
    // Intersection", Journal of Graphics Tools vol. 2). Ported from the by-ref C#
    // version to plain by-value C++ (a natural adjustment -- the original's own
    // comment notes the by-ref overloads only exist there for a tight-inner-loop
    // performance reason, not for readability).
    static std::optional<float> RayIntersectsTriangle(const Ray& ray,
                                                       const Vector3& vertex1,
                                                       const Vector3& vertex2,
                                                       const Vector3& vertex3) {
        // Compute vectors along two edges of the triangle.
        Vector3 edge1 = Vector3::Subtract(vertex2, vertex1);
        Vector3 edge2 = Vector3::Subtract(vertex3, vertex1);

        // Compute the determinant.
        Vector3 directionCrossEdge2 = Vector3::Cross(ray.Direction, edge2);
        float determinant = Vector3::Dot(edge1, directionCrossEdge2);

        // If the ray is parallel to the triangle plane, there is no collision. Uses
        // System::Single::Epsilon (sharp-runtime's C++ counterpart of .NET's
        // float.Epsilon -- the smallest positive denormalized float, NOT machine
        // epsilon) to match the C# original's comparison exactly.
        if (determinant > -System::Single::Epsilon && determinant < System::Single::Epsilon) {
            return std::nullopt;
        }

        float inverseDeterminant = 1.0f / determinant;

        // Calculate the U parameter of the intersection point.
        Vector3 distanceVector = Vector3::Subtract(ray.Position, vertex1);
        float triangleU = Vector3::Dot(distanceVector, directionCrossEdge2) * inverseDeterminant;

        // Make sure it is inside the triangle.
        if (triangleU < 0.0f || triangleU > 1.0f) {
            return std::nullopt;
        }

        // Calculate the V parameter of the intersection point.
        Vector3 distanceCrossEdge1 = Vector3::Cross(distanceVector, edge1);
        float triangleV = Vector3::Dot(ray.Direction, distanceCrossEdge1) * inverseDeterminant;

        // Make sure it is inside the triangle.
        if (triangleV < 0.0f || triangleU + triangleV > 1.0f) {
            return std::nullopt;
        }

        // Compute the distance along the ray to the triangle.
        float rayDistance = Vector3::Dot(edge2, distanceCrossEdge1) * inverseDeterminant;

        // Is the triangle behind the ray origin?
        if (rayDistance < 0.0f) {
            return std::nullopt;
        }

        return rayDistance;
    }

    // ---------------------------------------------------------------------------
    // Draw
    // ---------------------------------------------------------------------------

    // NOXNA helper: CNA's ModelTypeReader (ContentManager.cpp) builds one synthetic
    // "Root" ModelBone for every .model.json but never assigns any mesh's ParentBone
    // to it (ModelMesh has no setter for it at all -- see DEFERRED.md item #6's
    // "multi-bone rigid-part" note), so mesh->getParentBoneProperty() is always
    // nullptr for every model in this sample. CNA's own Model::Draw() already works
    // around this the same way (falls back to bone index 0); every model here is
    // logically single-bone (CopyAbsoluteBoneTransformsTo produces exactly one "Root"
    // transform), so index 0 is exactly the right fallback, not a guess. Same pattern
    // as PickingSample's identically-named helper -- see missing.md.
    static int BoneIndexOf(const ModelMesh* mesh) {
        return mesh->getParentBoneProperty() ? mesh->getParentBoneProperty()->getIndexProperty() : 0;
    }

    // DrawModel is a helper function that takes a model, world matrix, and bone
    // transforms, and draws the model.
    void DrawModel(Model& model, const Matrix& worldTransform, const std::vector<Matrix>& absoluteBoneTransforms) {
        for (ModelMesh* mesh : model.getMeshesProperty()) {
            for (Effect* effect : mesh->getEffectsPropertyMutable()) {
                auto* basicEffect = static_cast<BasicEffect*>(effect);
                basicEffect->EnableDefaultLighting();
                basicEffect->setPreferPerPixelLightingProperty(true);

                basicEffect->View = viewMatrix_;
                basicEffect->Projection = projectionMatrix_;
                basicEffect->World = absoluteBoneTransforms[BoneIndexOf(mesh)] * worldTransform;
            }

            mesh->Draw();
        }
    }

    // Helper for drawing the outline of the triangle currently under the cursor.
    void DrawPickedTriangle() {
        if (!pickedModelName_.has_value()) {
            return;
        }

        GraphicsDevice& device = getGraphicsDeviceProperty();

        // Set line drawing renderstates. We disable backface culling and turn off the
        // depth buffer because we want to be able to see the picked triangle outline
        // regardless of which way it is facing, and even if there is other geometry
        // in front of it.
        device.setRasterizerStateProperty(wireFrameState_);
        device.setDepthStencilStateProperty(DepthStencilState::None);

        // Activate the line drawing BasicEffect.
        lineEffect_->Projection = projectionMatrix_;
        lineEffect_->View = viewMatrix_;

        lineEffect_->getCurrentTechniqueProperty()->getPassesProperty()[0].Apply();

        // Draw the triangle.
        device.DrawUserPrimitives(PrimitiveType::TriangleList, pickedTriangle_.data(), 0, 1);

        // Reset renderstates to their default values.
        device.setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
        device.setDepthStencilStateProperty(DepthStencilState::Default);
    }

    // Helper for drawing text showing the current picking results.
    void DrawText() {
        // Draw the text twice to create a drop-shadow effect, first in black one
        // pixel down and to the right, then again in white at the real position.
        Vector2 shadowOffset(1.0f, 1.0f);

        spriteBatch_->Begin();

        // Draw a list of which models passed the initial bounding sphere test.
        if (!insideBoundingSpheres_.empty()) {
            std::string text = "Inside bounding sphere: ";
            for (std::size_t i = 0; i < insideBoundingSpheres_.size(); ++i) {
                if (i > 0) text += ", ";
                text += insideBoundingSpheres_[i];
            }

            Vector2 position(50.0f, 50.0f);

            spriteBatch_->DrawString(*spriteFont_, text, position + shadowOffset, Color::Black);
            spriteBatch_->DrawString(*spriteFont_, text, position, Color::White);
        }

        // Draw the name of the model that passed the per-triangle picking test.
        if (pickedModelName_.has_value()) {
            Vector2 position = cursor_->getPositionProperty();

            // Draw the text below the cursor position.
            position.Y += 32.0f;

            // Center the string.
            position = position - spriteFont_->MeasureString(*pickedModelName_) / 2.0f;

            spriteBatch_->DrawString(*spriteFont_, *pickedModelName_, position + shadowOffset, Color::Black);
            spriteBatch_->DrawString(*spriteFont_, *pickedModelName_, position, Color::White);
        }

        spriteBatch_->End();
    }

    void DrawHelpOverlay() {
        if (helpTimer_ <= 0.0f || !helpTexture_.has_value()) return;
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        int hw = helpTexture_->getWidthProperty();
        int hh = helpTexture_->getHeightProperty();
        float sx = (float)((vp.getWidthProperty() - hw) / 2);
        float sy = (float)((vp.getHeightProperty() - hh) / 2);
        spriteBatch_->Begin();
        spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        spriteBatch_->End();
    }
};

} // namespace TrianglePicking
