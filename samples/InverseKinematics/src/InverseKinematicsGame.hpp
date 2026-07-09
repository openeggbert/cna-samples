#pragma once

// Ported from InverseKinematicsSample.IKSample (IKSample.cs). Demonstrates the Cyclic
// Coordinate Descent (CCD) inverse-kinematics algorithm two ways: a 20-link chain of lit
// cylinder models reaching for a billboarded "cat" sprite, and the same CCD chain driving
// an Xbox LIVE avatar's arm/head to reach for (and look at) the cat.
//
// The avatar half is ported faithfully but, off a signed-in Xbox LIVE/Games-for-Windows-
// Live session (real XNA/FNA's off-Xbox behavior, which CNA's AvatarRenderer reproduces
// exactly -- see AvatarRenderer.hpp's class remarks), AvatarRenderer::State() always
// reports Unavailable. The C# original's own UpdateAvatarIK()/DrawAvatar() already
// defensively no-op in that case -- this port preserves that guard exactly, so the
// avatar half legitimately renders nothing here, matching a real Windows XNA build with
// no Live avatar signed in. This is not a CNA gap. See missing.md for the full account.

#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteFont.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>
#include <Microsoft/Xna/Framework/GamerServices/GamerServicesComponent.hpp>
#include <Microsoft/Xna/Framework/GamerServices/AvatarRenderer.hpp>
#include <Microsoft/Xna/Framework/GamerServices/AvatarRendererState.hpp>
#include <Microsoft/Xna/Framework/GamerServices/AvatarDescription.hpp>
#include <Microsoft/Xna/Framework/GamerServices/AvatarBone.hpp>
#include <Microsoft/Xna/Framework/GamerServices/AvatarExpression.hpp>
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

#include "Cat.hpp"
#include "CylinderModel.hpp"

#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;
using namespace Microsoft::Xna::Framework::GamerServices;

namespace InverseKinematicsSample {

class InverseKinematicsGame : public Game {
public:
    InverseKinematicsGame() {
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        graphics_->setPreferredBackBufferWidthProperty(853);
        graphics_->setPreferredBackBufferHeightProperty(480);
        graphics_->setPreferMultiSamplingProperty(true);

        gamerServices_ = std::make_unique<GamerServicesComponent>(*this);
        getComponentsProperty().Add(gamerServices_.get());

        getContentProperty().setRootDirectoryProperty("Content");
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "InverseKinematicsGame";
        return name;
    }

protected:
    // Creates the IK chains for the avatar and the cylinder chain.
    void LoadContent() override {
        spriteBatch_.emplace(getGraphicsDeviceProperty());
        font_.emplace(getContentProperty().Load<SpriteFont>("font"));

        LoadCylinderModel();

        // Create the cat.
        catTexture_.emplace(getContentProperty().Load<Texture2D>("cat"));
        cat_.emplace(getGraphicsDeviceProperty());
        cat_->Scale = 0.3f;
        cat_->Position = Vector3(-1.0f, 0.25f, -2.0f);
        cat_->Texture = &catTexture_.value();

        LoadAvatar();
        InitializeCylinderChain();

        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        // F1 help overlay
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput(gameTime);

        UpdateCamera();

        // Only run the IK simulation if the user has unpaused the simulation or chosen
        // to step through it.
        if (runSimulation_ || IsTriggered(Buttons::B) || IsTriggered(Keys::Space)) {
            UpdateAvatarIK();
            UpdateCylinderChainIK();
        }

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color::CornflowerBlue);

        getGraphicsDeviceProperty().setBlendStateProperty(BlendState::AlphaBlend);
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::Default);
        DrawCylinderChain();
        DrawAvatar();

        // Draw the cat.
        Vector3 cameraPosition = Matrix::Invert(view_).getTranslationProperty();
        cat_->Draw(cameraPosition, view_, projection_);

        DrawHUD();

        Game::Draw(gameTime);
    }

private:
    // The number of links in the cylinder chain.
    static constexpr int CylinderCount = 20;

    // Rendering stuff
    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::optional<SpriteBatch> spriteBatch_;
    // NOXNA: built directly from cylinder_verts.bin/cylinder_idx.bin instead of via
    // Content.Load<Model>("cylinder") -- see CylinderModel.hpp's own header comment and
    // missing.md for why (a real CNA ModelTypeReader vertex-stride/vtable bug, not a
    // conversion or asset problem).
    std::optional<CylinderModel> cylinderModel_;
    std::optional<SpriteFont> font_;
    std::unique_ptr<GamerServicesComponent> gamerServices_;

    // Keeps track of the input for this sample.
    KeyboardState currentKeyboardState_;
    GamePadState currentGamePadState_;
    GamePadState prevGamePadState_;
    KeyboardState prevKeyboardState_;

    // Camera related controls.
    float cameraRotX_ = 0.0f;
    float cameraRotY_ = 0.0f;
    float cameraRadius_ = 5.0f;
    Matrix view_;
    Matrix projection_;

    // The player controlled object that the IK chain attempts to reach.
    std::optional<Texture2D> catTexture_;
    std::optional<Cat> cat_;

    // When true, the simulation runs continuously.
    bool runSimulation_ = true;

    // Allows the player to step through each bone update of the IK chain. By default,
    // the entire IK chain is updated once per frame.
    bool singleStep_ = false;

    // The avatar renderer object used for drawing the avatar and accessing the avatar's
    // bind pose.
    std::unique_ptr<AvatarRenderer> avatarRenderer_;

    // The list of bones to be updated as part of the IK chain. The order and number of
    // bones in the IK chain will affect the way it animates to reach the cat. The
    // default IK chain for the avatar in this sample is: FingerMiddle3Left, WristLeft,
    // ElbowLeft, and ShoulderLeft.
    std::vector<int> avatarBoneChain_;

    // The list of bone transformation offsets from the avatar's bind pose.
    std::vector<Matrix> avatarBoneTransforms_;

    // Stores the entire list of world transforms for the avatar.
    std::vector<Matrix> avatarWorldTransforms_;

    // The list of local transforms for the avatar.
    std::vector<Matrix> avatarLocalTransforms_;

    // The index into avatarBoneChain for the currently updating bone.
    int avatarChainIndex_ = 1;

    // Used for initializing the avatar transforms once.
    bool isAvatarInitialized_ = false;

    // The list of bones to be updated as part of the cylinder IK chain.
    std::vector<int> cylinderChain_;

    // The default position/orientation of the cylinder IK chain bones, in local space,
    // relative to the parent bone.
    std::vector<Matrix> cylinderChainBindPose_;

    // The list of bone transformation offsets from the cylinder chain's bind pose.
    std::vector<Matrix> cylinderChainTransforms_;

    // The collection of parent indices for each bone in cylinderChainBindPose.
    std::vector<int> cylinderChainParentBones_;

    // Stores the entire list of world transforms for the cylinder chain.
    std::vector<Matrix> cylinderWorldTransforms_;

    // The list of local transforms for the cylinder chain.
    std::vector<Matrix> cylinderLocalTransforms_;

    // The world transform of the cylinder chain's root.
    Matrix cylinderRootWorldTransform_ = Matrix::getIdentityProperty();

    // The index into cylinderChain for the currently updating bone.
    int cylinderChainIndex_ = 1;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;

    // --- Initialization ---

    // Load the cylinder model and configure its BasicEffect. NOXNA: see CylinderModel.hpp
    // for why this reads cylinder_verts.bin/cylinder_idx.bin directly instead of calling
    // Content.Load<Model>("cylinder") (a real CNA ModelTypeReader bug, not an asset gap).
    void LoadCylinderModel() {
        cylinderModel_.emplace();
        cylinderModel_->Load(getContentProperty(), getGraphicsDeviceProperty());
    }

    // Load the avatar and initialize the avatar IK chain.
    void LoadAvatar() {
        // Create a new avatar renderer. Neither constructor argument is actually read
        // by the real (or CNA's faithfully-reproduced) implementation -- see
        // AvatarRenderer.hpp's class remarks -- so a local temporary description is
        // fine here; nothing retains the pointer past the constructor call.
        AvatarDescription description = AvatarDescription::CreateRandom();
        avatarRenderer_ = std::make_unique<AvatarRenderer>(&description, true);

        // Create the avatar IK chain.
        avatarBoneChain_.clear();
        avatarBoneChain_.push_back(static_cast<int>(AvatarBone::FingerMiddle3Left));
        avatarBoneChain_.push_back(static_cast<int>(AvatarBone::WristLeft));
        avatarBoneChain_.push_back(static_cast<int>(AvatarBone::ElbowLeft));
        avatarBoneChain_.push_back(static_cast<int>(AvatarBone::ShoulderLeft));

        // Initialize the avatar transform lists to the identity.
        int boneCount = AvatarRenderer::BoneCount;
        avatarBoneTransforms_.assign(boneCount, Matrix::getIdentityProperty());
        avatarWorldTransforms_ = avatarBoneTransforms_;
        avatarLocalTransforms_ = avatarBoneTransforms_;

        // Rotate the right arm down so it's idle at the avatar's hip.
        avatarBoneTransforms_[static_cast<int>(AvatarBone::ShoulderRight)] =
            Matrix::CreateRotationZ(MathHelper::ToRadians(80.0f));

        // Position the avatar.
        avatarRenderer_->setWorldProperty(Matrix::CreateTranslation(1.0f, 0.0f, 0.0f));
    }

    // Create and initialize the cylinder IK chain.
    void InitializeCylinderChain() {
        // Initialize chain bind pose.
        Matrix T = Matrix::CreateTranslation(0.0f, 0.1f, 0.0f);
        cylinderChainBindPose_.assign(CylinderCount, T);

        // Initialize the chain transform lists to the identity.
        cylinderChainTransforms_.assign(CylinderCount, Matrix::getIdentityProperty());
        cylinderWorldTransforms_ = cylinderChainTransforms_;
        cylinderLocalTransforms_ = cylinderChainTransforms_;

        // Initialize the parent index list. For the cylinder chain, the parent bone is
        // the one just before it in the list: {-1, 0, 1, 2, ..., CylinderCount - 2}.
        cylinderChainParentBones_.clear();
        cylinderChainParentBones_.reserve(CylinderCount);
        for (int i = 0; i < CylinderCount; ++i) {
            cylinderChainParentBones_.push_back(i - 1);
        }

        // Initialize the IK chain: {CylinderCount - 1, CylinderCount - 2, ..., 1, 0}.
        // cylinderChain[0] is the end effector of the IK chain.
        cylinderChain_.clear();
        cylinderChain_.reserve(CylinderCount);
        for (int i = CylinderCount - 1; i >= 0; --i) {
            cylinderChain_.push_back(i);
        }

        // Initialize the World and Local transform lists.
        UpdateTransforms(cylinderWorldTransforms_, cylinderLocalTransforms_,
                          cylinderRootWorldTransform_, cylinderChainBindPose_,
                          cylinderChainTransforms_, cylinderChainParentBones_);
    }

    // --- Update ---

    // Update the view and projection matrices.
    void UpdateCamera() {
        view_ = Matrix::CreateRotationY(MathHelper::ToRadians(cameraRotY_)) *
                Matrix::CreateRotationX(MathHelper::ToRadians(cameraRotX_)) *
                Matrix::CreateLookAt(Vector3(0.0f, 0.0f, -cameraRadius_), Vector3::Zero,
                                     Vector3::Up);

        float aspectRatio = getGraphicsDeviceProperty().getViewportProperty().getAspectRatioProperty();
        projection_ = Matrix::CreatePerspectiveFieldOfView(MathHelper::ToRadians(45.0f),
                                                            aspectRatio, 1.0f, 1000.0f);
    }

    // --- Inverse Kinematics ---

    // Loop over the Avatar IK chain and update each bone. The goal is to bring the end
    // effector closer to the cat.
    void UpdateAvatarIK() {
        if (avatarRenderer_->getStateProperty() != AvatarRendererState::Ready) {
            isAvatarInitialized_ = false;
        } else {
            // NOXNA note: this branch mirrors the C# original exactly, but per
            // AvatarRenderer.hpp's class remarks, State() always forces itself to
            // Unavailable on every read -- faithfully reproducing real XNA/FNA's
            // off-Xbox behavior -- so it can never actually execute here. Kept
            // (and kept compiling) for fidelity with the original; see missing.md.
            if (!isAvatarInitialized_) {
                UpdateAvatarWorldTransforms();
                isAvatarInitialized_ = true;
            }

            // Make the avatar look at the cat.
            AvatarLookAt(cat_->Position);

            // Update avatar chain.
            while (avatarChainIndex_ < static_cast<int>(avatarBoneChain_.size())) {
                // Update the current IK bone.
                UpdateBone(avatarBoneTransforms_, avatarBoneChain_[avatarChainIndex_],
                           avatarBoneChain_[0], cat_->Position, avatarWorldTransforms_,
                           avatarLocalTransforms_);

                // Update the local and world transforms for the IK chain now that
                // bones have moved.
                UpdateAvatarWorldTransforms();

                // Move to the next bone.
                ++avatarChainIndex_;

                // If the user wants to view each bone update one at a time, exit here.
                if (singleStep_) break;
            }

            // Reset the IK chain index to one after the end effector.
            if (avatarChainIndex_ >= static_cast<int>(avatarBoneChain_.size())) {
                avatarChainIndex_ = 1;
            }
        }
    }

    // NOXNA helper: avatarRenderer.BindPose/ParentBones are C#-style
    // ReadOnlyCollection<T> in real XNA; UpdateTransforms below takes std::vector<T>
    // (this port's natural analogue of IList<T>, matching the cylinder chain's own
    // usage), so this copies them out first. Unreachable in practice -- see
    // UpdateAvatarIK's comment above -- but kept compiling for fidelity.
    void UpdateAvatarWorldTransforms() {
        auto bindPoseCollection = avatarRenderer_->getBindPoseProperty();
        auto parentBonesCollection = avatarRenderer_->getParentBonesProperty();
        std::vector<Matrix> bindPose(bindPoseCollection.begin(), bindPoseCollection.end());
        std::vector<int> parentBones(parentBonesCollection.begin(), parentBonesCollection.end());
        UpdateTransforms(avatarWorldTransforms_, avatarLocalTransforms_,
                          avatarRenderer_->getWorldProperty(), bindPose,
                          avatarBoneTransforms_, parentBones);
    }

    // Makes the avatar look at a position in world space.
    void AvatarLookAt(Vector3 position) {
        int headIndex = static_cast<int>(AvatarBone::Head);
        Vector3 target = position - avatarWorldTransforms_[headIndex].getTranslationProperty();
        target.X = -target.X; // Flip the X axis.

        Matrix lookAt = Matrix::CreateLookAt(Vector3::Zero, target, Vector3::Up);

        avatarBoneTransforms_[headIndex] = lookAt;
    }

    // Loop over the cylinder IK chain and update each bone to bring the end effector
    // closer to the cat.
    void UpdateCylinderChainIK() {
        while (cylinderChainIndex_ < static_cast<int>(cylinderChain_.size())) {
            // Update the current IK bone.
            UpdateBone(cylinderChainTransforms_, cylinderChain_[cylinderChainIndex_],
                       cylinderChain_[0], cat_->Position, cylinderWorldTransforms_,
                       cylinderLocalTransforms_);

            // Update the local and world transforms for the IK chain now that bones
            // have moved.
            UpdateTransforms(cylinderWorldTransforms_, cylinderLocalTransforms_,
                              cylinderRootWorldTransform_, cylinderChainBindPose_,
                              cylinderChainTransforms_, cylinderChainParentBones_);

            // Move to the next bone.
            ++cylinderChainIndex_;

            // If the user wants to view each bone update one at a time, exit here.
            if (singleStep_) break;
        }

        // Reset the IK chain index to one after the end effector.
        if (cylinderChainIndex_ >= static_cast<int>(cylinderChain_.size())) {
            cylinderChainIndex_ = 1;
        }
    }

    // This is the primary function for updating inverse kinematics: the Cyclic
    // Coordinate Descent algorithm. Given a goal position, an end effector (often the
    // end bone in a chain), and a current bone, rotate the current bone so that it
    // brings the end effector closer to the goal position. Repeated for every bone in
    // the chain.
    static void UpdateBone(std::vector<Matrix>& transforms, int curBone, int endEffector,
                           Vector3 goal, std::vector<Matrix>& worldTransforms,
                           std::vector<Matrix>& localTransforms) {
        // Get the world transform of the current bone.
        Matrix curBoneWorld = worldTransforms[curBone];

        // Transform the goal into the coordinate system of the current bone.
        Vector3 goalInBoneSpace = Vector3::Transform(goal, Matrix::Invert(curBoneWorld));

        // Transform the end effector into the coordinate system of the current bone.
        Matrix endEffectorWorld = worldTransforms[endEffector];
        Vector3 endEffectorInBoneSpace =
            (endEffectorWorld * Matrix::Invert(curBoneWorld)).getTranslationProperty();

        // Unit vectors in the local coordinate space of the current bone.
        endEffectorInBoneSpace.Normalize();
        goalInBoneSpace.Normalize();

        // Build the rotation matrix that rotates the end effector onto the goal and
        // apply that rotation to the current bone.

        // Compute axis of rotation: the cross product of the two vectors.
        Vector3 axis = Vector3::Cross(endEffectorInBoneSpace, goalInBoneSpace);

        // Orient the axis by the local coordinate transform of the current bone.
        axis = Vector3::TransformNormal(axis, localTransforms[curBone]);
        axis.Normalize();

        // Compute the angle we will be rotating by: the angle between the vectors.
        float dot = Vector3::Dot(goalInBoneSpace, endEffectorInBoneSpace);

        // Clamp to -1 and 1 to avoid any possible floating point precision errors.
        dot = MathHelper::Clamp(dot, -1.0f, 1.0f);

        // Compute the angle.
        float angle = std::acos(dot);
        angle = MathHelper::WrapAngle(angle);

        // Create the rotation matrix.
        Matrix rotation = Matrix::CreateFromAxisAngle(axis, angle);

        // Rotate the current bone by the new rotation matrix.
        transforms[curBone] = transforms[curBone] * rotation;
    }

    // Updates the list of world transforms for a bone hierarchy. The hierarchy must be
    // sorted by bone depth, with the parent bone at the head of the list.
    static void UpdateTransforms(std::vector<Matrix>& worldTransforms,
                                 std::vector<Matrix>& localTransforms,
                                 Matrix rootWorldTransform,
                                 const std::vector<Matrix>& bindPose,
                                 const std::vector<Matrix>& animationTransforms,
                                 const std::vector<int>& parentBones) {
        // Set the parent bone transform.
        localTransforms[0] = animationTransforms[0] * bindPose[0];
        worldTransforms[0] = localTransforms[0] * rootWorldTransform;

        // Loop over all of the bones. Since the bone hierarchy is sorted by depth, the
        // parent is transformed before any child.
        for (std::size_t curBone = 1; curBone < worldTransforms.size(); ++curBone) {
            // Calculate the local transform of the bone.
            Matrix local = animationTransforms[curBone] * bindPose[curBone];

            // Find the transform of this bone's parent. If this is the first bone,
            // use the world matrix used on the avatar.
            Matrix parentMatrix = worldTransforms[static_cast<std::size_t>(parentBones[curBone])];

            // Calculate this bone's world space position.
            localTransforms[curBone] = local;
            worldTransforms[curBone] = local * parentMatrix;
        }
    }

    // --- Draw ---

    // Draws the cylinder chain.
    void DrawCylinderChain() {
        Matrix modelTransform = Matrix::CreateScale(0.04f, 0.025f, 0.04f);
        DrawBones(modelTransform, cylinderChainIndex_, cylinderChain_, cylinderWorldTransforms_);
    }

    // Draws the Avatar and its IK chain.
    void DrawAvatar() {
        avatarRenderer_->setViewProperty(view_);
        avatarRenderer_->setProjectionProperty(projection_);
        avatarRenderer_->Draw(avatarBoneTransforms_, AvatarExpression());

        // Draw the avatar bone chain.
        Matrix S = Matrix::CreateScale(0.04f, 0.01f, 0.04f);
        Matrix R = Matrix::CreateRotationZ(MathHelper::ToRadians(90.0f));
        Matrix modelTransform = S * R;

        // NOXNA note: this is never true in practice -- see UpdateAvatarIK's comment --
        // but kept for fidelity with the original.
        if (avatarRenderer_->getStateProperty() == AvatarRendererState::Ready) {
            DrawBones(modelTransform, avatarChainIndex_, avatarBoneChain_, avatarWorldTransforms_);
        }
    }

    // Draws the bones of a bone chain using a given model transform and scale. NOXNA: draws
    // via CylinderModel::DrawInstance() (see CylinderModel.hpp) instead of iterating
    // Model::Meshes/Effects directly, since this sample's cylinder model is loaded as a
    // CylinderModel, not a Model -- see the field comment on cylinderModel_.
    void DrawBones(Matrix modelTransform, int boneToColor, const std::vector<int>& boneChain,
                   const std::vector<Matrix>& worldTransforms) {
        // Compute the colors we will use to render the bone chain.
        Vector3 red = Color::Red.ToVector3();
        Vector3 black = Color::Black.ToVector3();
        Vector3 lightGray = Color::LightGray.ToVector3();

        for (int curBone : boneChain) {
            // Change the color of the bone we are updating if single step is enabled.
            Vector3 diffuseColor = lightGray;
            if (singleStep_) {
                if (boneChain[boneToColor] == curBone) {
                    diffuseColor = red;
                } else if (curBone == boneChain[0]) {
                    diffuseColor = black;
                }
            }

            cylinderModel_->DrawInstance(modelTransform * worldTransforms[curBone], view_,
                                         projection_, diffuseColor);
        }
    }

    // Draws the HUD text and, on top of it, the F1 help overlay -- both in the same
    // SpriteBatch Begin()/End() block.
    void DrawHUD() {
        std::string pausedText = runSimulation_ ? "Simulation: Running" : "Simulation: Paused";
        pausedText += "\nPress 'P' to toggle";

        std::string singleStepText = singleStep_ ? "Single Step is: ON" : "Single Step is: OFF";
        singleStepText += "\nPress 'Enter' to toggle";

        std::string stepThrough;
        if (singleStep_ || !runSimulation_) {
            stepThrough = "\nPress 'Space' to step once";
        }

        std::string controlsText = "-Controls-\n";
        controlsText += "Move camera: Arrow Keys\n";
        controlsText += "Zoom camera: Z/X Key\n";
        controlsText += "Move cat: W,A,S,D Keys\n";
        controlsText += "Zoom cat: Q/E Key\n";
        controlsText += "Reset: R Key";

        spriteBatch_->Begin();
        spriteBatch_->DrawString(*font_, pausedText, Vector2(100.0f, 80.0f), Color::White);
        spriteBatch_->DrawString(*font_, singleStepText, Vector2(100.0f, 120.0f), Color::White);
        spriteBatch_->DrawString(*font_, stepThrough, Vector2(100.0f, 160.0f), Color::White);
        spriteBatch_->DrawString(*font_, controlsText, Vector2(100.0f, 300.0f), Color::White);

        if (helpTimer_ > 0.0f && helpTexture_.has_value()) {
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            float sx = (float)((vp.getWidthProperty() - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }

        spriteBatch_->End();
    }

    // --- Handle Input ---

    // Handle controller and keyboard input to move the cat, move the camera, and allow
    // the user to exit the sample.
    void HandleInput(GameTime& gameTime) {
        float time = (float)gameTime.getElapsedGameTimeProperty().getTotalMillisecondsProperty();

        // Save current input states as the previous states for the next update.
        prevGamePadState_ = currentGamePadState_;
        prevKeyboardState_ = currentKeyboardState_;

        // Get the new input states.
        currentGamePadState_ = GamePad::GetState(PlayerIndex::One);
        currentKeyboardState_ = Keyboard::GetState();

        // Allow the game to exit.
        if (IsDown(Buttons::Back) || IsDown(Keys::Escape)) {
            Exit();
        }

        // Allow the user to reset the camera and cat.
        if (IsTriggered(Buttons::LeftStick) || IsTriggered(Buttons::RightStick) ||
            IsTriggered(Keys::R)) {
            Reset();
        }

        // Allow the user to start and stop the IK simulation.
        if (IsTriggered(Buttons::A) || IsTriggered(Keys::P)) {
            runSimulation_ = !runSimulation_;
        }

        // If the user presses B, pause and step through the simulation.
        if (IsTriggered(Buttons::B) || IsTriggered(Keys::Space)) {
            runSimulation_ = false;
        }

        // Allow the user to toggle single step mode.
        if (IsTriggered(Buttons::Start) || IsTriggered(Keys::Enter)) {
            singleStep_ = !singleStep_;
        }

        // Handle input that will control the cat.
        float moveSpeed = 0.1f;
        Vector2 movement = currentGamePadState_.getThumbSticksProperty().getLeftProperty() * moveSpeed;
        if (IsDown(Keys::W)) movement.Y += moveSpeed;
        if (IsDown(Keys::S)) movement.Y -= moveSpeed;
        if (IsDown(Keys::A)) movement.X -= moveSpeed;
        if (IsDown(Keys::D)) movement.X += moveSpeed;

        // Allow the cat to be moved toward and away from the center.
        float zoom = 0.0f;
        if (IsDown(Buttons::Y) || IsDown(Keys::Q)) zoom = 0.008f;
        if (IsDown(Buttons::X) || IsDown(Keys::E)) zoom = -0.008f;

        // Update the radius and rotation angles of the cat.
        movement.X = -movement.X;
        cat_->Position += Vector3(movement, zoom * time);

        // Handle input that will control the camera.
        moveSpeed = 0.1f;
        movement = currentGamePadState_.getThumbSticksProperty().getRightProperty() * moveSpeed;
        if (IsDown(Keys::Up)) movement.Y += moveSpeed;
        if (IsDown(Keys::Down)) movement.Y -= moveSpeed;
        if (IsDown(Keys::Left)) movement.X -= moveSpeed;
        if (IsDown(Keys::Right)) movement.X += moveSpeed;

        // Allow the camera to be zoomed in and out.
        zoom = 0.0f;
        zoom += currentGamePadState_.getTriggersProperty().getRightProperty() * 0.01f;
        zoom -= currentGamePadState_.getTriggersProperty().getLeftProperty() * 0.01f;
        if (IsDown(Keys::Z)) zoom = 0.007f;
        if (IsDown(Keys::X)) zoom = -0.007f;

        // Update the rotation angles and radius of the camera.
        cameraRotX_ += movement.Y * time;
        cameraRotY_ += movement.X * time;
        cameraRadius_ += zoom * time;
    }

    // Resets everything.
    void Reset() {
        cat_->Position = Vector3(-1.0f, 0.25f, -2.0f);
        cameraRotX_ = 0.0f;
        cameraRotY_ = 0.0f;
        cameraRadius_ = 5.0f;
        runSimulation_ = true;
        singleStep_ = false;

        InitializeCylinderChain();
        LoadAvatar();
    }

    // Returns whether a button was just pressed.
    bool IsTriggered(Buttons button) const {
        return currentGamePadState_.IsButtonDown(button) && !prevGamePadState_.IsButtonDown(button);
    }

    // Returns whether a key was just pressed.
    bool IsTriggered(Keys key) const {
        return currentKeyboardState_.IsKeyDown(key) && !prevKeyboardState_.IsKeyDown(key);
    }

    // Returns whether a button is down.
    bool IsDown(Buttons button) const {
        return currentGamePadState_.IsButtonDown(button);
    }

    // Returns whether a key is down.
    bool IsDown(Keys key) const {
        return currentKeyboardState_.IsKeyDown(key);
    }
};

} // namespace InverseKinematicsSample
