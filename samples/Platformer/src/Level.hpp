#pragma once
#include <cmath>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "Enemy.hpp"
#include "Gem.hpp"
#include "Player.hpp"
#include "RectangleExtensions.hpp"
#include "Tile.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "System/IServiceProvider.hpp"
#include "System/IO/Stream.hpp"
#include "System/Random.hpp"
#include "System/TimeSpan.hpp"

namespace Platformer {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Audio;
using namespace Microsoft::Xna::Framework::Content;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

class Level {
    std::vector<std::vector<Tile>> tiles_; // tiles_[x][y]
    std::vector<Texture2D> layers_;
    static constexpr int EntityLayer = 2;

    std::unique_ptr<Player> player_;
    std::vector<Gem> gems_;
    std::vector<Enemy> enemies_;

    Vector2 start_;
    Point exit_;
    static constexpr int InvalidPos = -1;

    System::Random random_{354668};
    int score_ = 0;
    bool reachedExit_ = false;
    System::TimeSpan timeRemaining_;
    static constexpr int PointsPerSecond = 5;

    ContentManager content_;
    std::optional<SoundEffect> exitReachedSound_;

public:
    Level(System::IServiceProvider* serviceProvider, System::IO::Stream& fileStream, int levelIndex,
          Graphics::GraphicsDevice& graphicsDevice)
        : content_(serviceProvider, "Content"),
          exit_(InvalidPos, InvalidPos)
    {
        content_.setGraphicsDevice(graphicsDevice);
        timeRemaining_ = System::TimeSpan::FromMinutes(2.0);

        LoadTiles(fileStream);

        layers_.reserve(3);
        for (int i = 0; i < 3; ++i) {
            layers_.push_back(content_.Load<Texture2D>(
                std::string("Backgrounds/Layer") + std::to_string(i) + "_" + std::to_string(levelIndex)));
        }

        exitReachedSound_.emplace(content_.Load<SoundEffect>("Sounds/ExitReached"));
    }

    ContentManager& getContentProperty() { return content_; }

    Player* getPlayerProperty() { return player_.get(); }

    int getScoreProperty() const { return score_; }
    bool getReachedExitProperty() const { return reachedExit_; }
    System::TimeSpan getTimeRemainingProperty() const { return timeRemaining_; }

    int getWidthProperty() const { return (int)tiles_.size(); }
    int getHeightProperty() const { return tiles_.empty() ? 0 : (int)tiles_[0].size(); }

    TileCollision GetCollision(int x, int y) const {
        if (x < 0 || x >= getWidthProperty()) return TileCollision::Impassable;
        if (y < 0 || y >= getHeightProperty()) return TileCollision::Passable;
        return tiles_[x][y].Collision;
    }

    Rectangle GetBounds(int x, int y) const {
        return Rectangle(x * Tile::Width, y * Tile::Height, Tile::Width, Tile::Height);
    }

    void Update(GameTime& gameTime,
                const KeyboardState& keyboardState,
                const GamePadState& gamePadState) {
        if (!player_->getIsAliveProperty() || timeRemaining_ == System::TimeSpan::Zero) {
            player_->ApplyPhysics(gameTime);
        } else if (reachedExit_) {
            int seconds = (int)std::round(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty() * 100.0);
            int maxSec  = (int)std::ceil(timeRemaining_.getTotalSecondsProperty());
            seconds = std::min(seconds, maxSec);
            timeRemaining_ = timeRemaining_ - System::TimeSpan::FromSeconds((double)seconds);
            score_ += seconds * PointsPerSecond;
        } else {
            timeRemaining_ = timeRemaining_ - gameTime.getElapsedGameTimeProperty();
            player_->Update(gameTime, keyboardState, gamePadState);
            UpdateGems(gameTime);

            if (player_->getBoundingRectangleProperty().getTopProperty() >= getHeightProperty() * Tile::Height)
                OnPlayerKilled(nullptr);

            UpdateEnemies(gameTime);

            if (player_->getIsAliveProperty() &&
                player_->getIsOnGroundProperty() &&
                player_->getBoundingRectangleProperty().Contains(exit_)) {
                OnExitReached();
            }
        }

        if (timeRemaining_ < System::TimeSpan::Zero)
            timeRemaining_ = System::TimeSpan::Zero;
    }

    void StartNewLife() { player_->Reset(start_); }

    void Draw(GameTime& gameTime, SpriteBatch& spriteBatch) {
        for (int i = 0; i <= EntityLayer; ++i)
            spriteBatch.Draw(layers_[i], Vector2::Zero, Color(255, 255, 255, 255));

        DrawTiles(spriteBatch);

        for (auto& gem : gems_)
            gem.Draw(gameTime, spriteBatch);

        player_->Draw(gameTime, spriteBatch);

        for (auto& enemy : enemies_)
            enemy.Draw(gameTime, spriteBatch);

        for (int i = EntityLayer + 1; i < (int)layers_.size(); ++i)
            spriteBatch.Draw(layers_[i], Vector2::Zero, Color(255, 255, 255, 255));
    }

private:
    void LoadTiles(System::IO::Stream& fileStream) {
        std::string content;
        uint8_t buf[4096];
        int32_t n;
        while ((n = fileStream.Read(reinterpret_cast<SharpRuntime::bytecs*>(buf), 0, 4096)) > 0)
            content.append(reinterpret_cast<char*>(buf), n);

        std::istringstream ss(content);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(ss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) lines.push_back(line);
        }

        if (lines.empty()) throw std::runtime_error("Level file is empty.");

        int width  = (int)lines[0].size();
        int height = (int)lines.size();

        tiles_.resize(width, std::vector<Tile>(height));

        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                tiles_[x][y] = LoadTile(lines[y][x], x, y);

        if (!player_)  throw std::runtime_error("A level must have a starting point.");
        if (exit_.X == InvalidPos) throw std::runtime_error("A level must have an exit.");
    }

    Tile LoadTile(char tileType, int x, int y) {
        switch (tileType) {
        case '.': return Tile(std::nullopt, TileCollision::Passable);
        case 'X': return LoadExitTile(x, y);
        case 'G': return LoadGemTile(x, y);
        case '-': return LoadNamedTile("Platform", TileCollision::Platform);
        case 'A': return LoadEnemyTile(x, y, "MonsterA");
        case 'B': return LoadEnemyTile(x, y, "MonsterB");
        case 'C': return LoadEnemyTile(x, y, "MonsterC");
        case 'D': return LoadEnemyTile(x, y, "MonsterD");
        case '~': return LoadVarietyTile("BlockB", 2, TileCollision::Platform);
        case ':': return LoadVarietyTile("BlockB", 2, TileCollision::Passable);
        case '1': return LoadStartTile(x, y);
        case '#': return LoadVarietyTile("BlockA", 7, TileCollision::Impassable);
        default:
            throw std::runtime_error(
                std::string("Unsupported tile type '") + tileType + "' at " +
                std::to_string(x) + "," + std::to_string(y));
        }
    }

    Tile LoadNamedTile(const std::string& name, TileCollision collision) {
        return Tile(content_.Load<Texture2D>("Tiles/" + name), collision);
    }

    Tile LoadVarietyTile(const std::string& baseName, int variationCount, TileCollision collision) {
        return LoadNamedTile(baseName + std::to_string(random_.Next(variationCount)), collision);
    }

    Tile LoadStartTile(int x, int y) {
        if (player_) throw std::runtime_error("A level may only have one starting point.");
        start_ = RectangleExtensions::GetBottomCenter(GetBounds(x, y));
        player_ = std::make_unique<Player>(this, start_);
        return Tile(std::nullopt, TileCollision::Passable);
    }

    Tile LoadExitTile(int x, int y) {
        if (exit_.X != InvalidPos) throw std::runtime_error("A level may only have one exit.");
        exit_ = GetBounds(x, y).getCenterProperty();
        return LoadNamedTile("Exit", TileCollision::Passable);
    }

    Tile LoadEnemyTile(int x, int y, const std::string& spriteSet) {
        enemies_.emplace_back(this, RectangleExtensions::GetBottomCenter(GetBounds(x, y)), spriteSet);
        return Tile(std::nullopt, TileCollision::Passable);
    }

    Tile LoadGemTile(int x, int y) {
        Point center = GetBounds(x, y).getCenterProperty();
        gems_.emplace_back(this, Vector2((float)center.X, (float)center.Y));
        return Tile(std::nullopt, TileCollision::Passable);
    }

    void UpdateGems(GameTime& gameTime) {
        for (int i = 0; i < (int)gems_.size(); ++i) {
            gems_[i].Update(gameTime);
            if (gems_[i].getBoundingCircleProperty().Intersects(player_->getBoundingRectangleProperty())) {
                score_ += Gem::PointValue;
                gems_[i].OnCollected(*player_);
                gems_.erase(gems_.begin() + i);
                --i;
            }
        }
    }

    void UpdateEnemies(GameTime& gameTime) {
        for (auto& enemy : enemies_) {
            enemy.Update(gameTime);
            if (enemy.getBoundingRectangleProperty().Intersects(player_->getBoundingRectangleProperty()))
                OnPlayerKilled(&enemy);
        }
    }

    void OnPlayerKilled(Enemy* killedBy) { player_->OnKilled(killedBy); }

    void OnExitReached() {
        player_->OnReachedExit();
        exitReachedSound_->Play();
        reachedExit_ = true;
    }

    void DrawTiles(SpriteBatch& spriteBatch) {
        for (int y = 0; y < getHeightProperty(); ++y) {
            for (int x = 0; x < getWidthProperty(); ++x) {
                if (tiles_[x][y].Texture) {
                    Vector2 position = Vector2((float)x, (float)y) * Tile::Size;
                    spriteBatch.Draw(*tiles_[x][y].Texture, position, Color(255, 255, 255, 255));
                }
            }
        }
    }
};

// ─── Gem method bodies ────────────────────────────────────────────────────────

inline Gem::Gem(Level* level, Microsoft::Xna::Framework::Vector2 position)
    : level_(level), basePosition_(position) {
    LoadContent();
}

inline void Gem::LoadContent() {
    texture_.emplace(level_->getContentProperty().Load<Texture2D>("Sprites/Gem"));
    origin_ = Vector2(texture_->getWidthProperty() / 2.0f, texture_->getHeightProperty() / 2.0f);
    collectedSound_.emplace(level_->getContentProperty().Load<SoundEffect>("Sounds/GemCollected"));
}

inline void Gem::Update(Microsoft::Xna::Framework::GameTime& gameTime) {
    constexpr float BounceHeight = 0.18f;
    constexpr float BounceRate   = 3.0f;
    constexpr float BounceSync   = -0.75f;
    double t = gameTime.getTotalGameTimeProperty().getTotalSecondsProperty() * BounceRate
               + getPositionProperty().X * BounceSync;
    bounce_ = (float)std::sin(t) * BounceHeight * (float)texture_->getHeightProperty();
}

inline void Gem::OnCollected(Player& /*collectedBy*/) {
    collectedSound_->Play();
}

inline void Gem::Draw(Microsoft::Xna::Framework::GameTime& /*gameTime*/,
                      Microsoft::Xna::Framework::Graphics::SpriteBatch& spriteBatch) {
    spriteBatch.Draw(*texture_, getPositionProperty(), std::nullopt,
                     Color_, 0.0f, origin_, 1.0f,
                     Microsoft::Xna::Framework::Graphics::SpriteEffects::None, 0.0f);
}

// ─── Enemy method bodies ──────────────────────────────────────────────────────

inline Enemy::Enemy(Level* level, Microsoft::Xna::Framework::Vector2 position, const std::string& spriteSet)
    : level_(level), position_(position) {
    LoadContent(spriteSet);
}

inline void Enemy::LoadContent(const std::string& spriteSet) {
    std::string path = "Sprites/" + spriteSet + "/";
    runAnimation_.emplace(level_->getContentProperty().Load<Texture2D>(path + "Run"),  0.1f,  true);
    idleAnimation_.emplace(level_->getContentProperty().Load<Texture2D>(path + "Idle"), 0.15f, true);
    sprite_.PlayAnimation(&*idleAnimation_);

    int width  = (int)(idleAnimation_->getFrameWidthProperty() * 0.35f);
    int left   = (idleAnimation_->getFrameWidthProperty() - width) / 2;
    int height = (int)(idleAnimation_->getFrameWidthProperty() * 0.7f);
    int top    = idleAnimation_->getFrameHeightProperty() - height;
    localBounds_ = Microsoft::Xna::Framework::Rectangle(left, top, width, height);
}

inline void Enemy::Update(Microsoft::Xna::Framework::GameTime& gameTime) {
    float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

    float posX = position_.X + localBounds_.Width / 2 * (int)direction_;
    int tileX = (int)std::floor(posX / Tile::Width) - (int)direction_;
    int tileY = (int)std::floor(position_.Y / Tile::Height);

    if (waitTime_ > 0.0f) {
        waitTime_ = std::max(0.0f, waitTime_ - elapsed);
        if (waitTime_ <= 0.0f)
            direction_ = (FaceDirection)(-(int)direction_);
    } else {
        if (level_->GetCollision(tileX + (int)direction_, tileY - 1) == TileCollision::Impassable ||
            level_->GetCollision(tileX + (int)direction_, tileY)     == TileCollision::Passable) {
            waitTime_ = MaxWaitTime;
        } else {
            position_ = position_ + Vector2((float)((int)direction_) * MoveSpeed * elapsed, 0.0f);
        }
    }
}

inline void Enemy::Draw(Microsoft::Xna::Framework::GameTime& gameTime,
                         Microsoft::Xna::Framework::Graphics::SpriteBatch& spriteBatch) {
    Player* player = level_->getPlayerProperty();
    if (!player->getIsAliveProperty() ||
        level_->getReachedExitProperty() ||
        level_->getTimeRemainingProperty() == System::TimeSpan::Zero ||
        waitTime_ > 0.0f) {
        sprite_.PlayAnimation(&*idleAnimation_);
    } else {
        sprite_.PlayAnimation(&*runAnimation_);
    }

    SpriteEffects flip = (int)direction_ > 0 ? SpriteEffects::FlipHorizontally : SpriteEffects::None;
    sprite_.Draw(gameTime, spriteBatch, position_, flip);
}

// ─── Player method bodies ─────────────────────────────────────────────────────

inline Player::Player(Level* level, Microsoft::Xna::Framework::Vector2 position)
    : level_(level) {
    LoadContent();
    Reset(position);
}

inline void Player::LoadContent() {
    auto& cm = level_->getContentProperty();

    idleAnimation_.emplace(    cm.Load<Texture2D>("Sprites/Player/Idle"),      0.1f,  true);
    runAnimation_.emplace(     cm.Load<Texture2D>("Sprites/Player/Run"),        0.1f,  true);
    jumpAnimation_.emplace(    cm.Load<Texture2D>("Sprites/Player/Jump"),       0.1f,  false);
    celebrateAnimation_.emplace(cm.Load<Texture2D>("Sprites/Player/Celebrate"), 0.1f, false);
    dieAnimation_.emplace(     cm.Load<Texture2D>("Sprites/Player/Die"),        0.1f,  false);

    int width  = (int)(idleAnimation_->getFrameWidthProperty() * 0.4f);
    int left   = (idleAnimation_->getFrameWidthProperty() - width) / 2;
    int height = (int)(idleAnimation_->getFrameWidthProperty() * 0.8f);
    int top    = idleAnimation_->getFrameHeightProperty() - height;
    localBounds_ = Rectangle(left, top, width, height);

    killedSound_.emplace(cm.Load<SoundEffect>("Sounds/PlayerKilled"));
    jumpSound_.emplace(  cm.Load<SoundEffect>("Sounds/PlayerJump"));
    fallSound_.emplace(  cm.Load<SoundEffect>("Sounds/PlayerFall"));
}

inline void Player::Reset(Microsoft::Xna::Framework::Vector2 position) {
    position_ = position;
    velocity_ = Vector2::Zero;
    isAlive_  = true;
    sprite_.PlayAnimation(&*idleAnimation_);
}

inline void Player::Update(Microsoft::Xna::Framework::GameTime& gameTime,
                            const Microsoft::Xna::Framework::Input::KeyboardState& keyboardState,
                            const Microsoft::Xna::Framework::Input::GamePadState& gamePadState) {
    GetInput(keyboardState, gamePadState);
    ApplyPhysics(gameTime);

    if (isAlive_ && isOnGround_) {
        if (std::abs(velocity_.X) - 0.02f > 0)
            sprite_.PlayAnimation(&*runAnimation_);
        else
            sprite_.PlayAnimation(&*idleAnimation_);
    }

    movement_  = 0.0f;
    isJumping_ = false;
}

inline void Player::GetInput(const Microsoft::Xna::Framework::Input::KeyboardState& keyboardState,
                              const Microsoft::Xna::Framework::Input::GamePadState& gamePadState) {
    using namespace Microsoft::Xna::Framework::Input;

    movement_ = gamePadState.getThumbSticksProperty().getLeftProperty().X * MoveStickScale;
    if (std::abs(movement_) < 0.5f) movement_ = 0.0f;

    if (gamePadState.IsButtonDown(Buttons::DPadLeft) ||
        keyboardState.IsKeyDown(Keys::Left) ||
        keyboardState.IsKeyDown(Keys::A)) {
        movement_ = -1.0f;
    } else if (gamePadState.IsButtonDown(Buttons::DPadRight) ||
               keyboardState.IsKeyDown(Keys::Right) ||
               keyboardState.IsKeyDown(Keys::D)) {
        movement_ = 1.0f;
    }

    isJumping_ = gamePadState.IsButtonDown(Buttons::A) ||
                 keyboardState.IsKeyDown(Keys::Space)  ||
                 keyboardState.IsKeyDown(Keys::Up)     ||
                 keyboardState.IsKeyDown(Keys::W);
}

inline void Player::ApplyPhysics(Microsoft::Xna::Framework::GameTime& gameTime) {
    float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

    Vector2 previousPosition = position_;

    velocity_.X = velocity_.X + movement_ * MoveAcceleration * elapsed;
    velocity_.Y = MathHelper::Clamp(velocity_.Y + GravityAcceleration * elapsed, -MaxFallSpeed, MaxFallSpeed);
    velocity_.Y = DoJump(velocity_.Y, gameTime);

    if (isOnGround_)
        velocity_.X *= GroundDragFactor;
    else
        velocity_.X *= AirDragFactor;

    velocity_.X = MathHelper::Clamp(velocity_.X, -MaxMoveSpeed, MaxMoveSpeed);

    position_ = position_ + velocity_ * elapsed;
    position_ = Vector2((float)std::round(position_.X), (float)std::round(position_.Y));

    HandleCollisions();

    if (position_.X == previousPosition.X) velocity_.X = 0.0f;
    if (position_.Y == previousPosition.Y) velocity_.Y = 0.0f;
}

inline float Player::DoJump(float velocityY, Microsoft::Xna::Framework::GameTime& gameTime) {
    if (isJumping_) {
        if ((!wasJumping_ && isOnGround_) || jumpTime_ > 0.0f) {
            if (jumpTime_ == 0.0f)
                jumpSound_->Play();
            jumpTime_ += (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
            sprite_.PlayAnimation(&*jumpAnimation_);
        }
        if (jumpTime_ > 0.0f && jumpTime_ <= MaxJumpTime) {
            velocityY = JumpLaunchVelocity * (1.0f - (float)std::pow(jumpTime_ / MaxJumpTime, JumpControlPower));
        } else {
            jumpTime_ = 0.0f;
        }
    } else {
        jumpTime_ = 0.0f;
    }
    wasJumping_ = isJumping_;
    return velocityY;
}

inline void Player::HandleCollisions() {
    Rectangle bounds = getBoundingRectangleProperty();
    int leftTile   = (int)std::floor((float)bounds.getLeftProperty()   / Tile::Width);
    int rightTile  = (int)std::ceil( (float)bounds.getRightProperty()  / Tile::Width) - 1;
    int topTile    = (int)std::floor((float)bounds.getTopProperty()    / Tile::Height);
    int bottomTile = (int)std::ceil( (float)bounds.getBottomProperty() / Tile::Height) - 1;

    isOnGround_ = false;

    for (int y = topTile; y <= bottomTile; ++y) {
        for (int x = leftTile; x <= rightTile; ++x) {
            TileCollision collision = level_->GetCollision(x, y);
            if (collision == TileCollision::Passable) continue;

            Rectangle tileBounds = level_->GetBounds(x, y);
            Vector2 depth = RectangleExtensions::GetIntersectionDepth(bounds, tileBounds);
            if (depth == Vector2::Zero) continue;

            float absDepthX = std::abs(depth.X);
            float absDepthY = std::abs(depth.Y);

            if (absDepthY < absDepthX || collision == TileCollision::Platform) {
                if (previousBottom_ <= (float)tileBounds.getTopProperty())
                    isOnGround_ = true;

                if (collision == TileCollision::Impassable || isOnGround_) {
                    position_ = Vector2(position_.X, position_.Y + depth.Y);
                    bounds = getBoundingRectangleProperty();
                }
            } else if (collision == TileCollision::Impassable) {
                position_ = Vector2(position_.X + depth.X, position_.Y);
                bounds = getBoundingRectangleProperty();
            }
        }
    }

    previousBottom_ = (float)bounds.getBottomProperty();
}

inline void Player::OnKilled(Enemy* killedBy) {
    isAlive_ = false;
    if (killedBy)
        killedSound_->Play();
    else
        fallSound_->Play();
    sprite_.PlayAnimation(&*dieAnimation_);
}

inline void Player::OnReachedExit() {
    sprite_.PlayAnimation(&*celebrateAnimation_);
}

inline void Player::Draw(Microsoft::Xna::Framework::GameTime& gameTime,
                          Microsoft::Xna::Framework::Graphics::SpriteBatch& spriteBatch) {
    using namespace Microsoft::Xna::Framework::Graphics;
    if (velocity_.X > 0.0f)
        flip_ = SpriteEffects::FlipHorizontally;
    else if (velocity_.X < 0.0f)
        flip_ = SpriteEffects::None;

    sprite_.Draw(gameTime, spriteBatch, position_, flip_);
}

} // namespace Platformer
