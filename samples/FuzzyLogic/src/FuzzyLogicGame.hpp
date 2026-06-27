#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "System/Random.hpp"

namespace FuzzyLogic {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

// ─────────────────────────────────────────────────────────────────────────────
// Forward declarations
// ─────────────────────────────────────────────────────────────────────────────
class Behavior;
class Tank;
class MouseEntity;

// ─────────────────────────────────────────────────────────────────────────────
// Entity  (abstract base — Behavior stored as raw pointer to avoid incomplete-
//          type issues; ownership managed via SetCurrentBehavior / destructor)
// ─────────────────────────────────────────────────────────────────────────────
class Entity {
    Texture2D  texture_;
    Vector2    position_;
    float      orientation_  = 0.0f;
    float      currentSpeed_ = 0.0f;
    Behavior*  currentBehavior_ = nullptr;
    bool       isHighlighted_   = false;
    Rectangle  levelBoundary_;

public:
    virtual ~Entity();                          // defined after Behavior

    virtual float MaxSpeed()  const = 0;
    virtual float TurnSpeed() const = 0;

    Vector2           Position()           const { return position_; }
    void              SetPosition(Vector2 v)     { position_ = v; }
    float             Orientation()        const { return orientation_; }
    void              SetOrientation(float o)    { orientation_ = o; }
    float             CurrentSpeed()       const { return currentSpeed_; }
    void              SetCurrentSpeed(float s)   { currentSpeed_ = s; }
    bool              IsHighlighted()      const { return isHighlighted_; }
    void              SetIsHighlighted(bool h)   { isHighlighted_ = h; }
    const Rectangle&  LevelBoundary()      const { return levelBoundary_; }
    Behavior*         GetCurrentBehavior() const { return currentBehavior_; }

    // Takes ownership of b; deletes old behavior first.
    void SetCurrentBehavior(Behavior* b);       // defined after Behavior

    Entity(Rectangle lb, Texture2D tex) : levelBoundary_(lb), texture_(tex) {}

    void Draw(SpriteBatch& sb, const GameTime& gt);
    void Update(GameTime& gt);

protected:
    virtual void ChooseBehavior(GameTime& gt) = 0;

private:
    Vector2 ClampToLevelBoundary(Vector2 v) {
        v.X = MathHelper::Clamp(v.X, (float)levelBoundary_.X,
                                (float)(levelBoundary_.X + levelBoundary_.Width));
        v.Y = MathHelper::Clamp(v.Y, (float)levelBoundary_.Y,
                                (float)(levelBoundary_.Y + levelBoundary_.Height));
        return v;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Behavior  (abstract base)
// ─────────────────────────────────────────────────────────────────────────────
class Behavior {
    Entity* entity_;
public:
    explicit Behavior(Entity* entity) : entity_(entity) {}
    virtual ~Behavior() = default;
    virtual void Update() = 0;

    Entity* GetEntity() const { return entity_; }

    void TurnToFace(Vector2 facePosition, float turnSpeed) {
        float x = facePosition.X - entity_->Position().X;
        float y = facePosition.Y - entity_->Position().Y;
        float desiredAngle = std::atan2(y, x);
        float diff = WrapAngle(desiredAngle - entity_->Orientation());
        diff = MathHelper::Clamp(diff, -turnSpeed, turnSpeed);
        entity_->SetOrientation(WrapAngle(entity_->Orientation() + diff));
    }

    static float WrapAngle(float r) {
        while (r < -MathHelper::Pi)  r += MathHelper::TwoPi;
        while (r >  MathHelper::Pi)  r -= MathHelper::TwoPi;
        return r;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Entity method bodies (Behavior now complete)
// ─────────────────────────────────────────────────────────────────────────────
inline Entity::~Entity() { delete currentBehavior_; }

inline void Entity::SetCurrentBehavior(Behavior* b) {
    delete currentBehavior_;
    currentBehavior_ = b;
}

inline void Entity::Draw(SpriteBatch& sb, const GameTime& gt) {
    Color tint(255, 255, 255, 255);
    if (isHighlighted_) {
        float t = std::sin(10.0f * (float)gt.getTotalGameTimeProperty().getTotalSecondsProperty());
        t = 0.5f + 0.5f * t;
        int gv = (int)(255.0f * t);
        int bv = (int)(255.0f * t);
        tint = Color(255, gv, bv, 255);
    }
    Vector2 center((float)(texture_.getWidthProperty()  / 2),
                   (float)(texture_.getHeightProperty() / 2));
    sb.Draw(texture_, position_, std::nullopt, tint,
            orientation_, center, 1.0f, SpriteEffects::None, 0.0f);
}

inline void Entity::Update(GameTime& gt) {
    ChooseBehavior(gt);
    if (currentBehavior_) currentBehavior_->Update();
    Vector2 heading(std::cos(orientation_), std::sin(orientation_));
    position_ = ClampToLevelBoundary(position_ + heading * currentSpeed_);
}

// ─────────────────────────────────────────────────────────────────────────────
// ChaseBehavior
// ─────────────────────────────────────────────────────────────────────────────
class ChaseBehavior : public Behavior {
    Entity* chase_;
public:
    ChaseBehavior(Entity* entity, Entity* chase) : Behavior(entity), chase_(chase) {}
    void Update() override {
        TurnToFace(chase_->Position(), GetEntity()->TurnSpeed());
        GetEntity()->SetCurrentSpeed(GetEntity()->MaxSpeed());
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// EvadeBehavior
// ─────────────────────────────────────────────────────────────────────────────
class EvadeBehavior : public Behavior {
    Entity* evade_;
public:
    EvadeBehavior(Entity* entity, Entity* evade) : Behavior(entity), evade_(evade) {}
    void Update() override {
        Vector2 seekPos = GetEntity()->Position() * 2.0f - evade_->Position();
        TurnToFace(seekPos, GetEntity()->TurnSpeed());
        GetEntity()->SetCurrentSpeed(GetEntity()->MaxSpeed());
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// WanderBehavior
// ─────────────────────────────────────────────────────────────────────────────
class WanderBehavior : public Behavior {
    Vector2 wanderDir_;
    inline static System::Random random_;
public:
    explicit WanderBehavior(Entity* entity) : Behavior(entity) {
        wanderDir_.X = std::cos(entity->Orientation());
        wanderDir_.Y = std::sin(entity->Orientation());
    }
    void Update() override {
        wanderDir_.X = wanderDir_.X + MathHelper::Lerp(-0.25f, 0.25f, (float)random_.NextDouble());
        wanderDir_.Y = wanderDir_.Y + MathHelper::Lerp(-0.25f, 0.25f, (float)random_.NextDouble());
        if (wanderDir_.LengthSquared() > 0.0f) wanderDir_.Normalize();

        TurnToFace(GetEntity()->Position() + wanderDir_, 0.15f * GetEntity()->TurnSpeed());

        const Rectangle& lb = GetEntity()->LevelBoundary();
        Vector2 screenCenter((float)(lb.Width / 2), (float)(lb.Height / 2));
        float distFromCenter = Vector2::Distance(screenCenter, GetEntity()->Position());
        float maxDist = (float)std::min(lb.Width / 2, lb.Height / 2);
        float normalized = (maxDist > 0.0f) ? distFromCenter / maxDist : 0.0f;
        float turnToCenter = 0.3f * normalized * normalized * GetEntity()->TurnSpeed();
        TurnToFace(screenCenter, turnToCenter);

        GetEntity()->SetCurrentSpeed(0.25f * GetEntity()->MaxSpeed());
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Tank  (ChooseBehavior/ChooseMouse bodies defined after MouseEntity)
// ─────────────────────────────────────────────────────────────────────────────
class Tank : public Entity {
public:
    static constexpr float CaughtDistance  = 30.0f;
    static constexpr float MaxDistance     = 175.0f;
    static constexpr float MinAngle        = 0.0f;
    static constexpr float MaxAngle        = MathHelper::PiOver2;
    static constexpr float MaxTime         = 4.0f;  // seconds

    float FuzzyDistanceWeight() const { return fuzzyDistanceWeight_; }
    void SetFuzzyDistanceWeight(float v) { fuzzyDistanceWeight_ = MathHelper::Clamp(v, 0.0f, 1.0f); }
    float FuzzyAngleWeight()    const { return fuzzyAngleWeight_; }
    void SetFuzzyAngleWeight(float v)    { fuzzyAngleWeight_    = MathHelper::Clamp(v, 0.0f, 1.0f); }
    float FuzzyTimeWeight()     const { return fuzzyTimeWeight_; }
    void SetFuzzyTimeWeight(float v)     { fuzzyTimeWeight_     = MathHelper::Clamp(v, 0.0f, 1.0f); }

    float MaxSpeed()  const override { return 2.0f; }
    float TurnSpeed() const override { return 0.075f; }

    Tank(Rectangle lb, Texture2D tex, std::vector<MouseEntity*>& mice)
        : Entity(lb, tex), mice_(mice) {
        SetPosition(Vector2((float)(lb.Width / 2), (float)(lb.Height / 2)));
    }

protected:
    void ChooseBehavior(GameTime& gt) override;

private:
    MouseEntity* ChooseMouse();
    float CalculateFuzzyDistance(float distance);
    float CalculateFuzzyAngle(int i);
    float CalculateFuzzyTime(int i);

    std::vector<MouseEntity*>& mice_;
    MouseEntity* currentlyChasingMouse_ = nullptr;
    float timeChasingThisMouse_ = 0.0f;

    float fuzzyDistanceWeight_ = 0.5f;
    float fuzzyAngleWeight_    = 0.5f;
    float fuzzyTimeWeight_     = 0.5f;
};

// ─────────────────────────────────────────────────────────────────────────────
// MouseEntity  (renamed from C# Mouse to avoid conflict with Input::Mouse)
// ─────────────────────────────────────────────────────────────────────────────
class MouseEntity : public Entity {
public:
    static constexpr float MouseEvadeDistance = 125.0f;
    static constexpr float MouseHysteresis    = 45.0f;

    float MaxSpeed()  const override { return 4.25f; }
    float TurnSpeed() const override { return 0.2f; }

    MouseEntity(Rectangle lb, Texture2D tex, Tank* tank)
        : Entity(lb, tex), tank_(tank) {
        SetPosition(Vector2(
            (float)random_.Next(lb.X, lb.X + lb.Width),
            (float)random_.Next(lb.Y, lb.Y + lb.Height)));
        SetCurrentBehavior(new WanderBehavior(this));
    }

protected:
    void ChooseBehavior(GameTime& gt) override;

private:
    Tank* tank_;
    inline static System::Random random_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Tank method bodies (MouseEntity now complete)
// ─────────────────────────────────────────────────────────────────────────────
inline void Tank::ChooseBehavior(GameTime& gt) {
    MouseEntity* next = ChooseMouse();
    if (next == nullptr) {
        SetIsHighlighted(false);
        if (currentlyChasingMouse_) {
            currentlyChasingMouse_->SetIsHighlighted(false);
            currentlyChasingMouse_ = nullptr;
        }
        if (dynamic_cast<WanderBehavior*>(GetCurrentBehavior()) == nullptr)
            SetCurrentBehavior(new WanderBehavior(this));
    } else if (next != currentlyChasingMouse_) {
        SetIsHighlighted(true);
        timeChasingThisMouse_ = 0.0f;
        if (currentlyChasingMouse_)
            currentlyChasingMouse_->SetIsHighlighted(false);
        currentlyChasingMouse_ = next;
        next->SetIsHighlighted(true);
        SetCurrentBehavior(new ChaseBehavior(this, currentlyChasingMouse_));
    } else {
        timeChasingThisMouse_ += (float)gt.getElapsedGameTimeProperty().getTotalSecondsProperty();
    }
}

inline MouseEntity* Tank::ChooseMouse() {
    MouseEntity* best     = nullptr;
    float        bestVal  = 0.0f;
    for (int i = 0; i < (int)mice_.size(); ++i) {
        float dist = Vector2::Distance(Position(), mice_[i]->Position());
        if (dist > MaxDistance) continue;
        float fuzzy = 0.0f;
        fuzzy += CalculateFuzzyDistance(dist)  * fuzzyDistanceWeight_;
        fuzzy += CalculateFuzzyAngle(i)        * fuzzyAngleWeight_;
        fuzzy += CalculateFuzzyTime(i)         * fuzzyTimeWeight_;
        if (fuzzy > bestVal) { best = mice_[i]; bestVal = fuzzy; }
    }
    return best;
}

inline float Tank::CalculateFuzzyDistance(float distance) {
    return 1.0f - (distance / MaxDistance);
}

inline float Tank::CalculateFuzzyAngle(int i) {
    Vector2 toMouse = mice_[i]->Position() - Position();
    float angle     = std::atan2(toMouse.Y, toMouse.X);
    float diff      = std::abs(Behavior::WrapAngle(Orientation() - angle));
    return MathHelper::Clamp(1.0f - diff / MaxAngle, 0.0f, 1.0f);
}

inline float Tank::CalculateFuzzyTime(int i) {
    float time = (mice_[i] == currentlyChasingMouse_) ? timeChasingThisMouse_ : 0.0f;
    return MathHelper::Clamp(time / MaxTime, 0.0f, 1.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// MouseEntity::ChooseBehavior (Tank now complete)
// ─────────────────────────────────────────────────────────────────────────────
inline void MouseEntity::ChooseBehavior(GameTime& /*gt*/) {
    float dist = Vector2::Distance(Position(), tank_->Position());
    if (dynamic_cast<WanderBehavior*>(GetCurrentBehavior()) == nullptr
        && dist > MouseEvadeDistance + MouseHysteresis) {
        SetCurrentBehavior(new WanderBehavior(this));
    } else if (dynamic_cast<EvadeBehavior*>(GetCurrentBehavior()) == nullptr
        && dist < MouseEvadeDistance - MouseHysteresis) {
        SetCurrentBehavior(new EvadeBehavior(this, tank_));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// FuzzyLogicGame
// ─────────────────────────────────────────────────────────────────────────────
class FuzzyLogicGame : public Microsoft::Xna::Framework::Game {
    static constexpr int NumberOfMice = 15;

    GraphicsDeviceManager           graphics_;
    std::unique_ptr<SpriteBatch>    spriteBatch_;

    std::optional<Texture2D> tankTexture_;
    std::optional<Texture2D> mouseTexture_;
    std::optional<Texture2D> onePixelWhite_;

    Rectangle levelBoundary_{0, 0, 0, 0};
    std::unique_ptr<Tank>     tank_;
    std::vector<MouseEntity*> mice_;

    int currentlySelectedWeight_ = 0;

    Rectangle barDistance_{105, 45, 85, 40};
    Rectangle barAngle_   {105, 125, 85, 40};
    Rectangle barTime_    {105, 205, 85, 40};

    KeyboardState lastKeyboardState_;
    GamePadState  lastGamePadState_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

public:
    const std::string& GetTypeName() const override {
        static const std::string name = "FuzzyLogicGame";
        return name;
    }

    FuzzyLogicGame() : graphics_(this) {
        graphics_.setPreferredBackBufferWidthProperty(800);
        graphics_.setPreferredBackBufferHeightProperty(480);
        getContentProperty().setRootDirectoryProperty("Content");
    }

    ~FuzzyLogicGame() {
        for (auto* m : mice_) delete m;
    }

protected:
    void Initialize() override {
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        int w = vp.getWidthProperty(), h = vp.getHeightProperty();
        levelBoundary_ = Rectangle(20, 20, w - 40, h - 40);
        Game::Initialize();
    }

    void LoadContent() override {
        spriteBatch_    = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        tankTexture_.emplace(getContentProperty().Load<Texture2D>("tank"));
        mouseTexture_.emplace(getContentProperty().Load<Texture2D>("mouse"));
        onePixelWhite_.emplace(getContentProperty().Load<Texture2D>("OnePixelWhite"));
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        tank_ = std::make_unique<Tank>(levelBoundary_, *tankTexture_, mice_);
        while ((int)mice_.size() < NumberOfMice)
            mice_.push_back(new MouseEntity(levelBoundary_, *mouseTexture_, tank_.get()));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput();

        tank_->Update(gameTime);

        int i = 0;
        while (i < (int)mice_.size()) {
            mice_[i]->Update(gameTime);
            if (Vector2::Distance(tank_->Position(), mice_[i]->Position()) < Tank::CaughtDistance) {
                delete mice_[i];
                mice_.erase(mice_.begin() + i);
            } else { ++i; }
        }

        while ((int)mice_.size() < NumberOfMice)
            mice_.push_back(new MouseEntity(levelBoundary_, *mouseTexture_, tank_.get()));

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(169, 169, 169, 255));

        spriteBatch_->Begin();

        for (auto* m : mice_) m->Draw(*spriteBatch_, gameTime);
        tank_->Draw(*spriteBatch_, gameTime);

        DrawBar(barDistance_, tank_->FuzzyDistanceWeight(), gameTime, currentlySelectedWeight_ == 0);
        DrawBar(barAngle_,    tank_->FuzzyAngleWeight(),    gameTime, currentlySelectedWeight_ == 1);
        DrawBar(barTime_,     tank_->FuzzyTimeWeight(),     gameTime, currentlySelectedWeight_ == 2);

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
    void DrawBar(Rectangle bar, float widthNorm, const GameTime& gameTime, bool highlighted) {
        Color tint(255, 255, 255, 255);
        if (highlighted) {
            float t = std::sin(10.0f * (float)gameTime.getTotalGameTimeProperty().getTotalSecondsProperty());
            t = 0.5f + 0.5f * t;
            int gv = (int)(255.0f * t);
            int bv = (int)(255.0f * t);
            tint = Color(255, gv, bv, 255);
        }
        bar.Width = (int)(bar.Width * widthNorm);
        spriteBatch_->Draw(*onePixelWhite_, bar, tint);
        // DrawString omitted — CNA has no SpriteFont support yet
    }

    void HandleInput() {
        KeyboardState kb  = Keyboard::GetState();
        GamePadState  pad = GamePad::GetState(PlayerIndex::One);

        if (kb.IsKeyDown(Keys::Escape) || pad.IsButtonDown(Buttons::Back))
            Exit();

        // Up/Down select which weight to modify (edge-triggered)
        if (!kb.IsKeyDown(Keys::Up) && lastKeyboardState_.IsKeyDown(Keys::Up)) {
            if (--currentlySelectedWeight_ < 0) currentlySelectedWeight_ = 2;
        }
        if (!kb.IsKeyDown(Keys::Down) && lastKeyboardState_.IsKeyDown(Keys::Down)) {
            currentlySelectedWeight_ = (currentlySelectedWeight_ + 1) % 3;
        }

        float changeAmount = pad.getThumbSticksProperty().getLeftProperty().X;
        if (kb.IsKeyDown(Keys::Right) || pad.IsButtonDown(Buttons::DPadRight)) changeAmount =  1.0f;
        if (kb.IsKeyDown(Keys::Left)  || pad.IsButtonDown(Buttons::DPadLeft))  changeAmount = -1.0f;
        changeAmount *= 0.025f;

        switch (currentlySelectedWeight_) {
            case 0: tank_->SetFuzzyDistanceWeight(tank_->FuzzyDistanceWeight() + changeAmount); break;
            case 1: tank_->SetFuzzyAngleWeight   (tank_->FuzzyAngleWeight()    + changeAmount); break;
            case 2: tank_->SetFuzzyTimeWeight    (tank_->FuzzyTimeWeight()     + changeAmount); break;
        }

        lastKeyboardState_ = kb;
        lastGamePadState_  = pad;
    }
};

} // namespace FuzzyLogic
