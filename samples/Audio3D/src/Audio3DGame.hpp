#pragma once

#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/GameComponent.hpp>
#include <Microsoft/Xna/Framework/GameComponentCollection.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/AlphaTestEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/VertexPositionTexture.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/CompareFunction.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/GamePad.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/GamePadState.hpp>
#include <Microsoft/Xna/Framework/PlayerIndex.hpp>
#include <Microsoft/Xna/Framework/Input/Buttons.hpp>
#include <Microsoft/Xna/Framework/Audio/AudioListener.hpp>
#include <Microsoft/Xna/Framework/Audio/AudioEmitter.hpp>
#include <Microsoft/Xna/Framework/Audio/SoundEffect.hpp>
#include <Microsoft/Xna/Framework/Audio/SoundEffectInstance.hpp>
#include <Microsoft/Xna/Framework/Audio/SoundState.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>
#include <Microsoft/Xna/Framework/MathHelper.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp>
#include <System/Random.hpp>
#include <System/TimeSpan.hpp>

#include <optional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;
using namespace Microsoft::Xna::Framework::Audio;

namespace Audio3D {

// ── IAudioEmitter ──────────────────────────────────────────────────────────

class IAudioEmitter {
public:
    virtual ~IAudioEmitter() = default;
    virtual Vector3 getPosition() const = 0;
    virtual Vector3 getForward() const = 0;
    virtual Vector3 getUp() const = 0;
    virtual Vector3 getVelocity() const = 0;
};

// ── AudioManager ────────────────────────────────────────────────────────────

class AudioManager : public Microsoft::Xna::Framework::GameComponent {
public:
    explicit AudioManager(Game& game) : GameComponent(game) {}

    AudioListener& getListenerRef() { return listener_; }

    void Initialize() override {
        SoundEffect::setDistanceScaleProperty(2000.0f);
        SoundEffect::setDopplerScaleProperty(0.1f);

        static const char* soundNames[] = {
            "CatSound0", "CatSound1", "CatSound2", "DogSound"
        };
        for (const char* name : soundNames) {
            auto sf = getGameProperty().getContentProperty().Load<SoundEffect>(name);
            soundEffects_.emplace(name, std::move(sf));
        }

        GameComponent::Initialize();
    }

    void Update(GameTime& gameTime) override {
        int index = 0;
        while (index < (int)activeSounds_.size()) {
            auto& s = activeSounds_[index];
            if (s.instance->getStateProperty() == SoundState::Stopped) {
                s.instance.reset();
                activeSounds_.erase(activeSounds_.begin() + index);
            } else {
                Apply3D(s);
                index++;
            }
        }
        GameComponent::Update(gameTime);
    }

    SoundEffectInstance* Play3DSound(const std::string& soundName, bool isLooped, IAudioEmitter* emitter) {
        ActiveSound s;
        auto inst = soundEffects_.at(soundName).CreateInstance();
        s.instance = std::make_unique<SoundEffectInstance>(std::move(inst));
        s.instance->setIsLoopedProperty(isLooped);
        s.emitter = emitter;
        Apply3D(s);
        s.instance->Play();
        activeSounds_.push_back(std::move(s));
        return activeSounds_.back().instance.get();
    }

private:
    AudioListener listener_;
    AudioEmitter  emitter_;

    std::unordered_map<std::string, SoundEffect> soundEffects_;

    struct ActiveSound {
        std::unique_ptr<SoundEffectInstance> instance;
        IAudioEmitter* emitter = nullptr;
    };
    std::vector<ActiveSound> activeSounds_;

    void Apply3D(ActiveSound& s) {
        emitter_.setPositionProperty(s.emitter->getPosition());
        emitter_.setForwardProperty(s.emitter->getForward());
        emitter_.setUpProperty(s.emitter->getUp());
        emitter_.setVelocityProperty(s.emitter->getVelocity());
        s.instance->Apply3D(listener_, emitter_);
    }
};

// ── QuadDrawer ──────────────────────────────────────────────────────────────

class QuadDrawer {
public:
    explicit QuadDrawer(GraphicsDevice& device) : device_(device), effect_(device) {
        effect_.setAlphaFunctionProperty(CompareFunction::Greater);
        effect_.setReferenceAlphaProperty(128);

        vertices_[0].Position = Vector3( 1,  1, 0);
        vertices_[1].Position = Vector3(-1,  1, 0);
        vertices_[2].Position = Vector3( 1, -1, 0);
        vertices_[3].Position = Vector3(-1, -1, 0);
    }

    void DrawQuad(Texture2D& texture, float textureRepeats,
                  const Matrix& world, const Matrix& view, const Matrix& projection) {
        effect_.setTextureProperty(&texture);
        effect_.setWorldProperty(world);
        effect_.setViewProperty(view);
        effect_.setProjectionProperty(projection);

        vertices_[0].TextureCoordinate = Vector2(0, 0);
        vertices_[1].TextureCoordinate = Vector2(textureRepeats, 0);
        vertices_[2].TextureCoordinate = Vector2(0, textureRepeats);
        vertices_[3].TextureCoordinate = Vector2(textureRepeats, textureRepeats);

        effect_.getCurrentTechniqueProperty()->getPassesProperty()[0].Apply();
        device_.DrawUserPrimitives(PrimitiveType::TriangleStrip, vertices_, 0, 2);
    }

private:
    GraphicsDevice&      device_;
    AlphaTestEffect      effect_;
    VertexPositionTexture vertices_[4];
};

// ── SpriteEntity ────────────────────────────────────────────────────────────

class SpriteEntity : public IAudioEmitter {
public:
    Vector3 Position;
    Vector3 Forward;
    Vector3 Up;
    Vector3 Velocity;
    Texture2D* Texture = nullptr;

    Vector3 getPosition() const override { return Position; }
    Vector3 getForward()  const override { return Forward; }
    Vector3 getUp()       const override { return Up; }
    Vector3 getVelocity() const override { return Velocity; }

    virtual void Update(GameTime& gameTime, AudioManager& audioManager) = 0;

    void Draw(QuadDrawer& quadDrawer, const Vector3& cameraPosition,
              const Matrix& view, const Matrix& projection) {
        Matrix world = Matrix::CreateTranslation(0, 1, 0) *
                       Matrix::CreateScale(800.0f) *
                       Matrix::CreateConstrainedBillboard(Position, cameraPosition,
                                                          Up, std::nullopt, std::nullopt);
        quadDrawer.DrawQuad(*Texture, 1.0f, world, view, projection);
    }
};

// ── Cat ─────────────────────────────────────────────────────────────────────

class Cat : public SpriteEntity {
public:
    Cat() {
        Forward = Vector3::Forward;
        Up      = Vector3::Up;
    }

    void Update(GameTime& gameTime, AudioManager& audioManager) override {
        double time = gameTime.getTotalGameTimeProperty().getTotalSecondsProperty();

        float dx = (float)-std::cos(time);
        float dz = (float)-std::sin(time);

        Vector3 newPosition = Vector3(dx, 0.0f, dz) * 6000.0f;

        Velocity = newPosition - Position;
        Position = newPosition;

        if (Velocity == Vector3::Zero)
            Forward = Vector3::Forward;
        else
            Forward = Vector3::Normalize(Velocity);

        Up = Vector3::Up;

        timeDelay_ = timeDelay_ - gameTime.getElapsedGameTimeProperty();
        if (timeDelay_ < System::TimeSpan::Zero) {
            std::string soundName = "CatSound" + std::to_string(random_.Next(3));
            audioManager.Play3DSound(soundName, false, this);
            timeDelay_ = timeDelay_ + System::TimeSpan::FromSeconds(1.25);
        }
    }

private:
    System::TimeSpan timeDelay_ = System::TimeSpan::Zero;
    System::Random   random_;
};

// ── Dog ─────────────────────────────────────────────────────────────────────

class Dog : public SpriteEntity {
public:
    Dog() {
        Forward = Vector3::Forward;
        Up      = Vector3::Up;
    }

    void Update(GameTime& gameTime, AudioManager& audioManager) override {
        Position = Vector3(0.0f, 0.0f, -4000.0f);
        Forward  = Vector3::Forward;
        Up       = Vector3::Up;
        Velocity = Vector3::Zero;

        timeDelay_ = timeDelay_ - gameTime.getElapsedGameTimeProperty();
        if (timeDelay_ < System::TimeSpan::Zero) {
            if (activeSound_ == nullptr) {
                activeSound_ = audioManager.Play3DSound("DogSound", true, this);
                timeDelay_ = timeDelay_ + System::TimeSpan::FromSeconds(6.0);
            } else {
                activeSound_->Stop(false);
                activeSound_ = nullptr;
                timeDelay_ = timeDelay_ + System::TimeSpan::FromSeconds(4.0);
            }
        }
    }

private:
    System::TimeSpan    timeDelay_   = System::TimeSpan::Zero;
    SoundEffectInstance* activeSound_ = nullptr;
};

// ── Audio3DGame ──────────────────────────────────────────────────────────────

class Audio3DGame : public Game {
public:
    Audio3DGame() {
        getContentProperty().setRootDirectoryProperty("Content");
        graphics_      = std::make_unique<GraphicsDeviceManager>(this);
        audioManager_  = std::make_unique<AudioManager>(*this);
        getComponentsProperty().Add(audioManager_.get());
        cat_ = std::make_unique<Cat>();
        dog_ = std::make_unique<Dog>();
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "Audio3DGame";
        return name;
    }

protected:
    void LoadContent() override {
        catTexture_.emplace(getContentProperty().Load<Texture2D>("CatTexture"));
        dogTexture_.emplace(getContentProperty().Load<Texture2D>("DogTexture"));
        checkerTexture_.emplace(getContentProperty().Load<Texture2D>("checker"));

        cat_->Texture = &*catTexture_;
        dog_->Texture = &*dogTexture_;

        quadDrawer_ = std::make_unique<QuadDrawer>(getGraphicsDeviceProperty());

        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        // F1 help overlay
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        currentKeyboardState_ = Keyboard::GetState();
        currentGamePadState_  = GamePad::GetState(PlayerIndex::One);

        if (currentKeyboardState_.IsKeyDown(Keys::Escape) ||
            currentGamePadState_.IsButtonDown(Buttons::Back)) {
            Exit();
            return;
        }

        UpdateCamera();

        AudioListener& listener = audioManager_->getListenerRef();
        listener.setPositionProperty(cameraPosition_);
        listener.setForwardProperty(cameraForward_);
        listener.setUpProperty(cameraUp_);
        listener.setVelocityProperty(cameraVelocity_);

        cat_->Update(gameTime, *audioManager_);
        dog_->Update(gameTime, *audioManager_);

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        auto& device = getGraphicsDeviceProperty();
        device.Clear(Color(100, 149, 237, 255)); // CornflowerBlue
        device.setBlendStateProperty(BlendState::AlphaBlend);

        auto& vp = device.getViewportProperty();
        float aspect = (float)vp.getWidthProperty() / (float)vp.getHeightProperty();

        Matrix view = Matrix::CreateLookAt(cameraPosition_,
                                           cameraPosition_ + cameraForward_,
                                           cameraUp_);
        Matrix projection = Matrix::CreatePerspectiveFieldOfView(1.0f, aspect, 1.0f, 100000.0f);

        // Ground plane
        Matrix groundTransform = Matrix::CreateScale(20000.0f) *
                                 Matrix::CreateRotationX(MathHelper::PiOver2);
        quadDrawer_->DrawQuad(*checkerTexture_, 32.0f, groundTransform, view, projection);

        cat_->Draw(*quadDrawer_, cameraPosition_, view, projection);
        dog_->Draw(*quadDrawer_, cameraPosition_, view, projection);

        // F1 help overlay (uses SpriteBatch internally via base Game::Draw or direct)
        DrawHelpOverlay();

        Game::Draw(gameTime);
    }

private:
    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::unique_ptr<AudioManager>          audioManager_;
    std::unique_ptr<Cat>                   cat_;
    std::unique_ptr<Dog>                   dog_;
    std::unique_ptr<QuadDrawer>            quadDrawer_;

    std::optional<Texture2D> catTexture_;
    std::optional<Texture2D> dogTexture_;
    std::optional<Texture2D> checkerTexture_;
    std::optional<Texture2D> helpTexture_;

    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

    std::unique_ptr<SpriteBatch> spriteBatch_;

    Vector3 cameraPosition_ = Vector3(0.0f, 512.0f, 0.0f);
    Vector3 cameraForward_  = Vector3::Forward;
    Vector3 cameraUp_       = Vector3::Up;
    Vector3 cameraVelocity_ = Vector3::Zero;

    KeyboardState currentKeyboardState_;
    GamePadState  currentGamePadState_;

    void UpdateCamera() {
        const float turnSpeed         = 0.05f;
        const float accelerationSpeed = 4.0f;
        const float frictionAmount    = 0.98f;

        float turn = -currentGamePadState_.getThumbSticksProperty().getLeftProperty().X * turnSpeed;
        if (currentKeyboardState_.IsKeyDown(Keys::Left))  turn += turnSpeed;
        if (currentKeyboardState_.IsKeyDown(Keys::Right)) turn -= turnSpeed;

        cameraForward_ = Vector3::TransformNormal(cameraForward_,
                                                   Matrix::CreateRotationY(turn));

        float accel = currentGamePadState_.getThumbSticksProperty().getLeftProperty().Y * accelerationSpeed;
        if (currentKeyboardState_.IsKeyDown(Keys::Up))   accel += accelerationSpeed;
        if (currentKeyboardState_.IsKeyDown(Keys::Down)) accel -= accelerationSpeed;

        cameraVelocity_ = cameraVelocity_ + cameraForward_ * accel;
        cameraPosition_ = cameraPosition_ + cameraVelocity_;
        cameraVelocity_ = cameraVelocity_ * frictionAmount;
    }

    void DrawHelpOverlay() {
        if (helpTimer_ <= 0.0f || !helpTexture_.has_value()) return;

        if (!spriteBatch_)
            spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

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

} // namespace Audio3D
