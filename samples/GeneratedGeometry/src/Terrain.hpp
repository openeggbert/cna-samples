#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionNormalTexture.hpp"

namespace GeneratedGeometrySample
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Content;
    using namespace Microsoft::Xna::Framework::Graphics;

    class Terrain
    {
        static constexpr float terrainScale     = 4.0f;
        static constexpr float terrainBumpiness = 64.0f;
        static constexpr float texCoordScale    = 0.1f;

        std::unique_ptr<VertexBuffer> vertexBuffer_;
        std::unique_ptr<IndexBuffer>  indexBuffer_;
        std::unique_ptr<BasicEffect>  effect_;
        Texture2D                     rockTexture_;
        int                           primitiveCount_ = 0;
        int                           vertexCount_    = 0;

        void Build(GraphicsDevice& device, const Texture2D& heightmap)
        {
            const int w = heightmap.getWidthProperty();
            const int h = heightmap.getHeightProperty();

            std::vector<Color> pixels(w * h, Color(0, 0, 0, 255));
            heightmap.GetData(pixels.data(), w * h);

            std::vector<float> heights(w * h);
            for (int i = 0; i < w * h; i++)
                heights[i] = pixels[i].getRProperty() / 255.0f;

            auto sampleH = [&](int x, int y) -> float {
                x = std::clamp(x, 0, w - 1);
                y = std::clamp(y, 0, h - 1);
                return heights[y * w + x];
            };

            std::vector<VertexPositionNormalTexture> verts(w * h);
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    float ht  = sampleH(x, y);
                    float posX = (x - w * 0.5f) * terrainScale;
                    float posY = (ht - 1.0f) * terrainBumpiness;
                    float posZ = (y - h * 0.5f) * terrainScale;

                    // gradient-based normal: cross(tangentZ, tangentX) gives +Y for flat terrain
                    Vector3 tangentX(2.0f * terrainScale,
                                     (sampleH(x+1,y) - sampleH(x-1,y)) * terrainBumpiness,
                                     0.0f);
                    Vector3 tangentZ(0.0f,
                                     (sampleH(x,y+1) - sampleH(x,y-1)) * terrainBumpiness,
                                     2.0f * terrainScale);
                    Vector3 normal = Vector3::Cross(tangentZ, tangentX);
                    normal.Normalize();

                    verts[y * w + x] = VertexPositionNormalTexture(
                        Vector3(posX, posY, posZ),
                        normal,
                        Vector2(x * texCoordScale, y * texCoordScale));
                }
            }

            std::vector<uint16_t> indices;
            indices.reserve((w - 1) * (h - 1) * 6);
            for (int y = 0; y < h - 1; y++) {
                for (int x = 0; x < w - 1; x++) {
                    auto idx = [&](int xi, int yi) -> uint16_t {
                        return static_cast<uint16_t>(yi * w + xi);
                    };
                    // two CCW triangles per quad (matching XNA TerrainProcessor winding)
                    indices.push_back(idx(x,   y));
                    indices.push_back(idx(x+1, y));
                    indices.push_back(idx(x+1, y+1));

                    indices.push_back(idx(x,   y));
                    indices.push_back(idx(x+1, y+1));
                    indices.push_back(idx(x,   y+1));
                }
            }

            vertexCount_    = static_cast<int>(verts.size());
            primitiveCount_ = static_cast<int>(indices.size()) / 3;

            vertexBuffer_ = std::make_unique<VertexBuffer>(device, vertexCount_);
            vertexBuffer_->SetData(verts.data(), vertexCount_);

            indexBuffer_ = std::make_unique<IndexBuffer>(device, static_cast<int>(indices.size()));
            indexBuffer_->SetData(indices.data(), static_cast<int>(indices.size()));
        }

    public:
        void LoadContent(ContentManager& content, GraphicsDevice& device)
        {
            Texture2D heightmap = content.Load<Texture2D>("terrain.bmp");
            rockTexture_        = content.Load<Texture2D>("rocks.bmp");

            Build(device, heightmap);

            effect_ = std::make_unique<BasicEffect>(device);
            effect_->EnableDefaultLighting();
            effect_->setSpecularColorProperty(Vector3(0.6f, 0.4f, 0.2f));
            effect_->setSpecularPowerProperty(8.0f);
            effect_->setFogEnabledProperty(true);
            effect_->setFogColorProperty(Vector3(0.15f, 0.15f, 0.15f));
            effect_->setFogStartProperty(100.0f);
            effect_->setFogEndProperty(320.0f);
            effect_->setTextureEnabledProperty(true);
            effect_->setTextureProperty(&rockTexture_);
            effect_->setWorldProperty(Matrix::getIdentityProperty());
        }

        void Draw(Matrix view, Matrix projection)
        {
            effect_->setViewProperty(view);
            effect_->setProjectionProperty(projection);

            auto& device = effect_->getGraphicsDeviceInternal();
            device.setDepthStencilStateProperty(DepthStencilState::Default);
            device.setRasterizerStateProperty(RasterizerState::CullNone);
            device.SetVertexBuffer(vertexBuffer_.get());
            device.setIndicesProperty(indexBuffer_.get());

            for (auto& pass : effect_->getCurrentTechniqueProperty()->getPassesProperty()) {
                pass.Apply();
                device.DrawIndexedPrimitives(PrimitiveType::TriangleList,
                    0, 0, vertexCount_, 0, primitiveCount_);
            }

            device.setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
        }
    };

} // namespace GeneratedGeometrySample
