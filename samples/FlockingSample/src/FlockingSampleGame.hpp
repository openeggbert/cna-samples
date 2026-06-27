#pragma once
#include <memory>
#include <string>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "AIParameters.hpp"
#include "Animals/Cat.hpp"
#include "Flock.hpp"
#include "InputState.hpp"

namespace Flocking {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

class FlockingSampleGame : public Microsoft::Xna::Framework::Game {
    static constexpr float detectionDefault              = 70.0f;
    static constexpr float separationDefault             = 50.0f;
    static constexpr float moveInOldDirInfluenceDefault  = 1.0f;
    static constexpr float moveInFlockDirInfluenceDefault= 1.0f;
    static constexpr float moveInRandomDirInfluenceDefault=0.05f;
    static constexpr float maxTurnRadiansDefault         = 6.0f;
    static constexpr float perMemberWeightDefault        = 1.0f;
    static constexpr float perDangerWeightDefault        = 50.0f;

    static constexpr float sliderMin = 0.0f;
    static constexpr float sliderMax = 100.0f;

    GraphicsDeviceManager graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;

    InputState inputState_;
    AIParameters flockParams_;
    bool aiParamUpdate_ = false;

    std::unique_ptr<Flock> flock_;
    std::unique_ptr<Cat>   cat_;

    int selectionNum_ = 0;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

public:
    const std::string& GetTypeName() const override {
        static const std::string name = "FlockingSampleGame";
        return name;
    }

    FlockingSampleGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");
        ResetAIParams();
    }

protected:
    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

        Texture2D catTex   = getContentProperty().Load<Texture2D>("cat");
        Texture2D birdTex  = getContentProperty().Load<Texture2D>("mouse");

        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        int w = vp.getWidthProperty();
        int h = vp.getHeightProperty();

        flock_ = std::make_unique<Flock>(birdTex, w, h, flockParams_);
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;
        inputState_.Update();

        if (inputState_.Exit()) { Exit(); return; }

        if (inputState_.SelectUp())   { selectionNum_--; if (selectionNum_ < 0) selectionNum_ = 1; }
        if (inputState_.SelectDown()) { selectionNum_ = (selectionNum_ + 1) % 2; }

        if (cat_) cat_->SetInputDirection(inputState_.MoveCatX(), inputState_.MoveCatY());

        if (inputState_.ToggleCat()) {
            if (cat_) {
                cat_.reset();
            } else {
                auto& vp = getGraphicsDeviceProperty().getViewportProperty();
                Texture2D catTex = getContentProperty().Load<Texture2D>("cat");
                cat_ = std::make_unique<Cat>(catTex,
                    vp.getWidthProperty(), vp.getHeightProperty());
            }
        }

        if (inputState_.ResetDistances()) { ResetAIParams(); aiParamUpdate_ = true; }

        if (inputState_.ResetFlock()) {
            if (flock_) flock_->ResetFlock();
            aiParamUpdate_ = true;
        }

        float drag = inputState_.SliderMove();
        if (drag != 0.0f) {
            switch (selectionNum_) {
                case 0: flockParams_.DetectionDistance  += drag; break;
                case 1: flockParams_.SeparationDistance += drag; break;
            }
            flockParams_.DetectionDistance  = MathHelper::Clamp(flockParams_.DetectionDistance,  sliderMin, sliderMax);
            flockParams_.SeparationDistance = MathHelper::Clamp(flockParams_.SeparationDistance, sliderMin, sliderMax);
            aiParamUpdate_ = true;
        }

        if (aiParamUpdate_) {
            if (flock_) flock_->SetFlockParams(flockParams_);
            aiParamUpdate_ = false;
        }

        if (cat_)   cat_->Update(gameTime);
        if (flock_) flock_->Update(gameTime, cat_.get());

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255));

        spriteBatch_->Begin();
        if (flock_) flock_->Draw(*spriteBatch_, gameTime);
        if (cat_)   cat_->Draw(*spriteBatch_, gameTime);
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
    void ResetAIParams() {
        flockParams_.DetectionDistance          = detectionDefault;
        flockParams_.SeparationDistance         = separationDefault;
        flockParams_.MoveInOldDirectionInfluence= moveInOldDirInfluenceDefault;
        flockParams_.MoveInFlockDirectionInfluence= moveInFlockDirInfluenceDefault;
        flockParams_.MoveInRandomDirectionInfluence= moveInRandomDirInfluenceDefault;
        flockParams_.MaxTurnRadians             = maxTurnRadiansDefault;
        flockParams_.PerMemberWeight            = perMemberWeightDefault;
        flockParams_.PerDangerWeight            = perDangerWeightDefault;
    }
};

} // namespace Flocking
