#pragma once

// Ported from BoundingVolumeRendering.BoundingSphereRenderer (BoundingSphereRenderer.cs).
// The C# original is a static class holding shared GPU state (a VertexBuffer + BasicEffect
// used by every BoundingSphere.Draw(view, projection) extension-method call). Since PickingGame
// only ever needs one instance of this helper, it's ported as an ordinary instance class held as
// a member of PickingGame instead of C#-style static/global state -- a natural adjustment for the
// C++ port (no static-class/extension-method equivalent), not a behavioral change: Initialize()
// and Draw() do exactly what the original's same-named static methods did.

#include <Microsoft/Xna/Framework/BoundingSphere.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/MathHelper.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp>
#include <Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/BufferUsage.hpp>
#include <Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp>

#include <cmath>
#include <optional>
#include <stdexcept>
#include <vector>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace PickingSample {

class BoundingSphereRenderer {
public:
    // Initializes the graphics objects for rendering BoundingSpheres.
    // sphereResolution: number of line segments per one of the three circles.
    void Initialize(GraphicsDevice& graphicsDevice, int sphereResolution) {
        effect_.emplace(graphicsDevice);
        effect_->setLightingEnabledProperty(false);
        effect_->VertexColorEnabled = true;

        lineCount_ = (sphereResolution + 1) * 3;

        std::vector<VertexPositionColor> verts;
        verts.reserve(static_cast<std::size_t>(lineCount_) * 2);

        float step = MathHelper::TwoPi / (float)sphereResolution;

        // XY plane loop
        for (float a = 0.0f; a < MathHelper::TwoPi; a += step) {
            verts.emplace_back(Vector3(std::cos(a), std::sin(a), 0.0f), Color::Blue);
            verts.emplace_back(Vector3(std::cos(a + step), std::sin(a + step), 0.0f), Color::Blue);
        }

        // XZ plane loop
        for (float a = 0.0f; a < MathHelper::TwoPi; a += step) {
            verts.emplace_back(Vector3(std::cos(a), 0.0f, std::sin(a)), Color::Red);
            verts.emplace_back(Vector3(std::cos(a + step), 0.0f, std::sin(a + step)), Color::Red);
        }

        // YZ plane loop
        for (float a = 0.0f; a < MathHelper::TwoPi; a += step) {
            verts.emplace_back(Vector3(0.0f, std::cos(a), std::sin(a)), Color::Green);
            verts.emplace_back(Vector3(0.0f, std::cos(a + step), std::sin(a + step)), Color::Green);
        }

        vertexBuffer_.emplace(graphicsDevice, VertexPositionColor::getVertexDeclarationStatic(),
                              (int)verts.size(), BufferUsage::WriteOnly);
        vertexBuffer_->SetData(verts.data(), (int)verts.size());
    }

    // Draws a BoundingSphere using the current view/projection matrices.
    void Draw(const BoundingSphere& sphere, const Matrix& view, const Matrix& projection) {
        if (!effect_.has_value()) {
            throw std::runtime_error("You must call Initialize before you can render any spheres.");
        }

        effect_->getGraphicsDeviceInternal().SetVertexBuffer(&*vertexBuffer_);

        effect_->World = Matrix::CreateScale(sphere.Radius) * Matrix::CreateTranslation(sphere.Center);
        effect_->View = view;
        effect_->Projection = projection;

        effect_->getCurrentTechniqueProperty()->getPassesProperty()[0].Apply();
        effect_->getGraphicsDeviceInternal().DrawPrimitives(PrimitiveType::LineList, 0, lineCount_);
    }

private:
    std::optional<VertexBuffer> vertexBuffer_;
    std::optional<BasicEffect> effect_;
    int lineCount_ = 0;
};

} // namespace PickingSample
