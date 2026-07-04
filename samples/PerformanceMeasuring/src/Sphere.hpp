#pragma once

// Sphere.hpp — C++ port of Sphere.cs (XNA 4.0 PerformanceMeasuring sample).

#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"

#include "Primitives/SpherePrimitive.hpp"

namespace PerformanceMeasuring {

using Microsoft::Xna::Framework::BoundingSphere;
using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Matrix;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Graphics::GraphicsDevice;

class Sphere {
public:
    Vector3 Position;
    Vector3 Velocity;
    Color SphereColor = Color::White;

    Sphere(GraphicsDevice& graphics, float radius) : radius_(radius), primitive_(graphics, radius * 2.0f, 10) {}

    [[nodiscard]] float getRadius() const { return radius_; }

    [[nodiscard]] BoundingSphere getBounds() const { return BoundingSphere(Position, radius_); }

    void Update(const GameTime& gameTime) {
        Position += Velocity * (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
    }

    void Draw(const Matrix& view, const Matrix& projection) {
        primitive_.Draw(Matrix::CreateTranslation(Position), view, projection, SphereColor);
    }

private:
    float radius_;
    SpherePrimitive primitive_;
};

} // namespace PerformanceMeasuring
