#pragma once

// Ported from InverseKinematicsSample.Cat (Cat.cs). A simple billboarded quad -- always
// faces the camera -- textured with cat.tga/cat.png. This is the target the IK chains
// (cylinder chain and, in principle, the avatar's arm) try to reach. See missing.md.

#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/VertexPositionTexture.hpp>
#include <Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>

#include <array>
#include <optional>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace InverseKinematicsSample {

// Entity that always faces the camera.
class Cat {
public:
    // Gets or sets the scale of the entity.
    float Scale = 1.0f;

    // Gets or sets the 3D position of the entity.
    Vector3 Position;

    // Gets or sets the orientation of this entity.
    Vector3 Up = Vector3::Up;

    // Gets or sets the texture used to display this entity. Not owned by Cat -- matches
    // the C# original's plain `Texture2D Texture { get; set; }` reference property; the
    // owning Game keeps the real Texture2D alive.
    Texture2D* Texture = nullptr;

    // Creates a billboard sprite that the IK chains will attempt to reach.
    explicit Cat(GraphicsDevice& device) : graphicsDevice_(device), basicEffect_(device) {
        // Pre-allocate an array of six vertices.
        vertices_[0].Position = Vector3(1.0f, 1.0f, 0.0f);
        vertices_[1].Position = Vector3(-1.0f, 1.0f, 0.0f);
        vertices_[2].Position = Vector3(-1.0f, -1.0f, 0.0f);
        vertices_[3].Position = Vector3(1.0f, 1.0f, 0.0f);
        vertices_[4].Position = Vector3(-1.0f, -1.0f, 0.0f);
        vertices_[5].Position = Vector3(1.0f, -1.0f, 0.0f);
    }

    // Draw the billboard sprite with transparency.
    void Draw(Vector3 cameraPosition, Matrix view, Matrix projection) {
        // Create the world transform for the billboarded cat.
        Matrix world = Matrix::CreateTranslation(0.0f, 1.0f, 0.0f) *
                       Matrix::CreateScale(Scale) *
                       Matrix::CreateConstrainedBillboard(Position, cameraPosition, Up,
                                                           std::nullopt, std::nullopt);

        // Draw the cat.
        DrawQuad(Texture, 1.0f, world, view, projection);
    }

    // Draws a quadrilateral as part of the 3D world.
    void DrawQuad(Texture2D* texture, float textureRepeats, Matrix world, Matrix view,
                  Matrix projection) {
        // Set our effect to use the specified texture and camera matrices.
        basicEffect_.setTextureProperty(texture);
        basicEffect_.setTextureEnabledProperty(true);

        basicEffect_.World = world;
        basicEffect_.View = view;
        basicEffect_.Projection = projection;

        // Update our vertex array to use the specified number of texture repeats.
        vertices_[0].TextureCoordinate = Vector2(0.0f, 0.0f);
        vertices_[1].TextureCoordinate = Vector2(textureRepeats, 0.0f);
        vertices_[2].TextureCoordinate = Vector2(textureRepeats, textureRepeats);
        vertices_[3].TextureCoordinate = Vector2(0.0f, 0.0f);
        vertices_[4].TextureCoordinate = Vector2(textureRepeats, textureRepeats);
        vertices_[5].TextureCoordinate = Vector2(0.0f, textureRepeats);

        // Draw the quad.
        for (auto& pass : basicEffect_.getCurrentTechniqueProperty()->getPassesProperty()) {
            pass.Apply();
        }
        graphicsDevice_.DrawUserPrimitives(PrimitiveType::TriangleList, vertices_.data(), 0, 2);
    }

private:
    // Graphics related things used for drawing.
    GraphicsDevice& graphicsDevice_;
    BasicEffect basicEffect_;
    std::array<VertexPositionTexture, 6> vertices_;
};

} // namespace InverseKinematicsSample
