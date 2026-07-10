#pragma once

// Ported from XnaGraphicsDemo.BasicDemo (BasicDemo.cs). Demo shows how to use
// BasicEffect: a rotating tank (TankModel.hpp) and a textured background grid
// (GridModel.hpp), with menu options to toggle the diffuse texture and cycle through 4
// lighting modes, plus drag-to-zoom.

#include <cmath>
#include <stdexcept>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"

#include "DemoGame.hpp"
#include "GridModel.hpp"
#include "MenuComponent.hpp"
#include "MenuEntry.hpp"
#include "TankModel.hpp"

namespace ReachGraphicsDemoSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

// Custom menu entry subclass for cycling through the different lighting options.
class LightModeMenu : public MenuEntry {
public:
    LightingMode LightMode = LightingMode::ThreeVertexLights;

    void OnClicked() override {
        if (LightMode == LightingMode::ThreePixelLights) {
            LightMode = LightingMode::NoLighting;
        } else {
            LightMode = static_cast<LightingMode>(static_cast<int>(LightMode) + 1);
        }

        MenuEntry::OnClicked();
    }

    std::string GetText() const override {
        switch (LightMode) {
            case LightingMode::NoLighting: return "no lighting";
            case LightingMode::OneVertexLight: return "one vertex light";
            case LightingMode::ThreeVertexLights: return "three vertex lights";
            case LightingMode::ThreePixelLights: return "three pixel lights";
        }
        throw std::logic_error("LightModeMenu: unsupported LightingMode");
    }
    void SetText(const std::string&) override {}
};

class BasicDemo : public MenuComponent {
public:
    explicit BasicDemo(DemoGame& game) : MenuComponent(game) {
        auto textureEntry = std::make_unique<BoolMenuEntry>("texture");
        textureEnable_ = textureEntry.get();
        Entries.push_back(std::move(textureEntry));

        auto lightEntry = std::make_unique<LightModeMenu>();
        lightMode_ = lightEntry.get();
        Entries.push_back(std::move(lightEntry));

        auto backEntry = std::make_unique<MenuEntry>();
        backEntry->SetText("back");
        backEntry->Clicked = [&game]() { game.SetActiveMenu(0); };
        Entries.push_back(std::move(backEntry));
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "BasicDemo";
        return name;
    }

    // Resets the menu state.
    void Reset() override {
        lightMode_->LightMode = LightingMode::ThreeVertexLights;
        textureEnable_->Value = true;
        zoom_ = 1.0f;

        MenuComponent::Reset();
    }

    // Loads content for this demo.
    void LoadContent() override {
        tank_.Load(GetGame().getContentProperty(), getGraphicsDeviceProperty());
        grid_.Load(GetGame().getContentProperty(), getGraphicsDeviceProperty());
    }

    // Updates the tank animation.
    void Update(GameTime& gameTime) override {
        tank_.Animate(gameTime);

        MenuComponent::Update(gameTime);
    }

    // Draws the BasicEffect demo.
    void Draw(const GameTime& gameTime) override {
        float time = static_cast<float>(gameTime.getTotalGameTimeProperty().getTotalSecondsProperty());

        // Compute camera matrices.
        Matrix rotation = Matrix::CreateRotationY(time * 0.1f);

        Matrix projection =
            Matrix::CreatePerspectiveFieldOfView(MathHelper::PiOver4, getGraphicsDeviceProperty().getViewportProperty().getAspectRatioProperty(),
                                                  10.0f, 20000.0f);

        Matrix view = Matrix::CreateLookAt(Vector3(1500.0f, 550.0f, 0.0f) * zoom_ + Vector3(0.0f, 150.0f, 0.0f),
                                            Vector3(0.0f, 150.0f, 0.0f), Vector3::Up);

        // Draw the title.
        DrawTitle("basic effect", Color(192, 192, 192, 255), Color(156, 156, 156, 255));

        // Set render states.
        getGraphicsDeviceProperty().setBlendStateProperty(BlendState::Opaque);
        getGraphicsDeviceProperty().setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::Default);
        getGraphicsDeviceProperty().getSamplerStatesProperty()[0] = SamplerState::LinearWrap;

        // Draw the background grid.
        grid_.Draw(Matrix::CreateScale(1.5f) * rotation, view, projection);

        // Draw the tank model.
        tank_.Draw(rotation, view, projection, lightMode_->LightMode, textureEnable_->Value);

        MenuComponent::Draw(gameTime);
    }

protected:
    // Dragging up and down on the menu background zooms in and out.
    void OnDrag(Vector2 delta) override {
        zoom_ = MathHelper::Clamp(zoom_ * std::exp(delta.Y / 400.0f), 0.4f, 6.0f);
    }

private:
    GridModel grid_;
    TankModel tank_;
    LightModeMenu* lightMode_ = nullptr;
    BoolMenuEntry* textureEnable_ = nullptr;
    float zoom_ = 1.0f;
};

} // namespace ReachGraphicsDemoSample
