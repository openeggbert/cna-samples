#pragma once

// Ported from XnaGraphicsDemo.AlphaDemo (AlphaDemo.cs). Demo shows how to use
// AlphaTestEffect: renders a single tank into a 400x400 RenderTarget2D, then stamps 25
// billboarded copies of that rendertarget ("imposter sprites") around a 3D scene using
// AlphaTestEffect + DrawUserIndexedPrimitives, faking a much larger scene of tanks
// cheaply. Drag-to-rotate the camera.

#include <cstdint>
#include <vector>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/AlphaTestEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/RenderTarget2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionTexture.hpp"

#include "DemoGame.hpp"
#include "GridModel.hpp"
#include "MenuComponent.hpp"
#include "MenuEntry.hpp"
#include "TankModel.hpp"

namespace ReachGraphicsDemoSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class AlphaDemo : public MenuComponent {
public:
    explicit AlphaDemo(DemoGame& game) : MenuComponent(game) {
        auto backEntry = std::make_unique<MenuEntry>();
        backEntry->SetText("back");
        backEntry->Clicked = [&game]() { game.SetActiveMenu(0); };
        Entries.push_back(std::move(backEntry));
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "AlphaDemo";
        return name;
    }

    // Resets the menu state.
    void Reset() override {
        cameraRotation_ = 0.85f;

        MenuComponent::Reset();
    }

    // Loads content for this demo.
    void LoadContent() override {
        tank_.Load(GetGame().getContentProperty(), getGraphicsDeviceProperty());
        grid_.Load(GetGame().getContentProperty(), getGraphicsDeviceProperty());

        renderTarget_ = std::make_unique<RenderTarget2D>(getGraphicsDeviceProperty(), 400, 400, false,
                                                          SurfaceFormat::Color, DepthFormat::Depth24);

        alphaTestEffect_ = std::make_unique<AlphaTestEffect>(getGraphicsDeviceProperty());
        alphaTestEffect_->setAlphaFunctionProperty(CompareFunction::Greater);
        alphaTestEffect_->setReferenceAlphaProperty(128);
    }

    // Animates the tank model.
    void Update(GameTime& gameTime) override {
        tank_.Animate(gameTime);

        MenuComponent::Update(gameTime);
    }

    // Draws the AlphaTestEffect demo.
    void Draw(const GameTime& gameTime) override {
        // Compute camera matrices.
        float time = static_cast<float>(gameTime.getTotalGameTimeProperty().getTotalSecondsProperty());

        Matrix tankRotation = Matrix::CreateRotationY(time * 0.15f);
        Matrix sceneRotation = Matrix::CreateRotationY(cameraRotation_);

        Vector3 cameraPosition(1250.0f, 250.0f, 0.0f);
        Vector3 cameraTarget(0.0f, -100.0f, 0.0f);

        Matrix view = Matrix::CreateLookAt(cameraPosition, cameraTarget, Vector3::Up);

        Matrix projection = Matrix::CreatePerspectiveFieldOfView(
            MathHelper::PiOver4, getGraphicsDeviceProperty().getViewportProperty().getAspectRatioProperty(), 10.0f,
            10000.0f);

        // Draw a single copy of the tank model into a rendertarget.
        DrawTankIntoRenderTarget(tankRotation, sceneRotation);

        // Draw the scene background.
        DrawTitle("alpha test effect", Color(192, 192, 192, 255), Color(156, 156, 156, 255));

        getGraphicsDeviceProperty().setBlendStateProperty(BlendState::Opaque);
        getGraphicsDeviceProperty().setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::None);
        getGraphicsDeviceProperty().getSamplerStatesProperty()[0] = SamplerState::LinearWrap;

        grid_.Draw(Matrix::CreateTranslation(0.0f, -8.0f, 0.0f) * sceneRotation, view, projection);

        // Draw many copies of the imposter sprite, faking the illusion of a more
        // complex 3D scene with many tanks.
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::Default);
        getGraphicsDeviceProperty().getSamplerStatesProperty()[0] = SamplerState::LinearClamp;

        DrawImposterSprites(tankRotation, sceneRotation, cameraPosition, cameraTarget, view, projection);

        MenuComponent::Draw(gameTime);
    }

protected:
    // Dragging on the menu background rotates the camera.
    void OnDrag(Vector2 delta) override { cameraRotation_ += delta.X / 400.0f; }

private:
    // Draws the 3D tank model into a rendertarget.
    void DrawTankIntoRenderTarget(const Matrix& tankRotation, const Matrix& sceneRotation) {
        Matrix view = Matrix::CreateLookAt(Vector3(1250.0f, 650.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3::Up);

        Matrix projection = Matrix::CreatePerspectiveFieldOfView(MathHelper::PiOver4, 1.0f, 10.0f, 10000.0f);

        std::vector<RenderTargetBinding> previousRenderTargets = getGraphicsDeviceProperty().GetRenderTargets();

        getGraphicsDeviceProperty().SetRenderTarget(renderTarget_.get());

        getGraphicsDeviceProperty().setBlendStateProperty(BlendState::Opaque);
        getGraphicsDeviceProperty().setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::Default);
        getGraphicsDeviceProperty().getSamplerStatesProperty()[0] = SamplerState::LinearWrap;

        getGraphicsDeviceProperty().Clear(Color(0, 0, 0, 0));

        tank_.Draw(tankRotation * sceneRotation * Matrix::CreateScale(0.9f), view, projection, LightingMode::OneVertexLight,
                   true);

        if (previousRenderTargets.empty()) {
            getGraphicsDeviceProperty().SetRenderTarget(nullptr);
        } else {
            getGraphicsDeviceProperty().SetRenderTargets(previousRenderTargets);
        }
    }

    // Draws many copies of the rendertarget as 2D billboard sprites, positioned within
    // the 3D scene.
    void DrawImposterSprites(const Matrix& tankRotation, const Matrix& sceneRotation, const Vector3& cameraPosition,
                              const Vector3& cameraTarget, const Matrix& view, const Matrix& projection) {
        constexpr int start = -2;
        constexpr int end = 3;
        constexpr int count = (end - start) * (end - start);

        constexpr float size = 0.2f;
        constexpr float spacing = 240.0f;

        constexpr float width = 120.0f;
        constexpr float height1 = 135.0f;
        constexpr float height2 = -135.0f;

        // Create billboard vertices.
        std::vector<VertexPositionTexture> vertices(count * 4);
        int i = 0;

        for (int x = start; x < end; x++) {
            for (int z = start; z < end; z++) {
                Matrix scale = Matrix::CreateScale(size);
                Matrix translation = Matrix::CreateTranslation(static_cast<float>(x) * spacing, 0.0f,
                                                                 static_cast<float>(z) * spacing);
                Matrix world = tankRotation * scale * translation * sceneRotation;

                Matrix billboard = Matrix::CreateConstrainedBillboard(world.getTranslationProperty(), cameraPosition,
                                                                       Vector3::Up, cameraTarget - cameraPosition,
                                                                       std::optional<Vector3>());

                vertices[i].Position = Vector3::Transform(Vector3(width, height1, 0.0f), billboard);
                vertices[i++].TextureCoordinate = Vector2(0.0f, 0.0f);

                vertices[i].Position = Vector3::Transform(Vector3(-width, height1, 0.0f), billboard);
                vertices[i++].TextureCoordinate = Vector2(1.0f, 0.0f);

                vertices[i].Position = Vector3::Transform(Vector3(-width, height2, 0.0f), billboard);
                vertices[i++].TextureCoordinate = Vector2(1.0f, 1.0f);

                vertices[i].Position = Vector3::Transform(Vector3(width, height2, 0.0f), billboard);
                vertices[i++].TextureCoordinate = Vector2(0.0f, 1.0f);
            }
        }

        // Create billboard indices.
        std::vector<std::uint16_t> indices(count * 6);
        std::uint16_t currentVertex = 0;
        i = 0;

        while (i < static_cast<int>(indices.size())) {
            indices[static_cast<std::size_t>(i++)] = currentVertex;
            indices[static_cast<std::size_t>(i++)] = static_cast<std::uint16_t>(currentVertex + 1);
            indices[static_cast<std::size_t>(i++)] = static_cast<std::uint16_t>(currentVertex + 2);

            indices[static_cast<std::size_t>(i++)] = currentVertex;
            indices[static_cast<std::size_t>(i++)] = static_cast<std::uint16_t>(currentVertex + 2);
            indices[static_cast<std::size_t>(i++)] = static_cast<std::uint16_t>(currentVertex + 3);

            currentVertex += 4;
        }

        // Draw the billboard sprites.
        alphaTestEffect_->setWorldProperty(Matrix::getIdentityProperty());
        alphaTestEffect_->setViewProperty(view);
        alphaTestEffect_->setProjectionProperty(projection);
        alphaTestEffect_->setTextureProperty(renderTarget_.get());

        alphaTestEffect_->getCurrentTechniqueProperty()->getPassesProperty()[0].Apply();

        getGraphicsDeviceProperty().DrawUserIndexedPrimitives(PrimitiveType::TriangleList, vertices.data(), 0,
                                                               count * 4, indices.data(), 0, count * 2);
    }

    GridModel grid_;
    TankModel tank_;
    std::unique_ptr<RenderTarget2D> renderTarget_;
    std::unique_ptr<AlphaTestEffect> alphaTestEffect_;

    float cameraRotation_ = 0.0f;
};

} // namespace ReachGraphicsDemoSample
