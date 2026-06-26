#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

namespace SpriteSheetSample
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Content;
    using namespace Microsoft::Xna::Framework::Graphics;

    // Runtime port of SpriteSheetRuntime.SpriteSheet + SpriteSheetPipeline.SpritePacker.
    // Packs individual sprite images into a single atlas texture at LoadContent time.
    class SpriteSheet
    {
        Texture2D                              texture_;
        std::vector<Rectangle>                 spriteRectangles_;
        std::unordered_map<std::string, int>   spriteNames_;

        struct ArrangedSprite {
            int index, x, y, width, height;
        };

        static int GuessOutputWidth(const std::vector<ArrangedSprite>& sprites)
        {
            std::vector<int> widths;
            widths.reserve(sprites.size());
            for (auto& s : sprites) widths.push_back(s.width);
            std::sort(widths.begin(), widths.end());
            int maxWidth    = widths.back();
            int medianWidth = widths[widths.size() / 2];
            int width = static_cast<int>(medianWidth *
                            std::round(std::sqrt(static_cast<double>(sprites.size()))));
            return std::max(width, maxWidth);
        }

        static int FindIntersecting(const std::vector<ArrangedSprite>& sprites,
                                    int index, int x, int y)
        {
            int w = sprites[index].width;
            int h = sprites[index].height;
            for (int i = 0; i < index; i++) {
                if (sprites[i].x >= x + w) continue;
                if (sprites[i].x + sprites[i].width <= x) continue;
                if (sprites[i].y >= y + h) continue;
                if (sprites[i].y + sprites[i].height <= y) continue;
                return i;
            }
            return -1;
        }

        static void PositionSprite(std::vector<ArrangedSprite>& sprites,
                                   int index, int outputWidth)
        {
            int x = 0, y = 0;
            while (true) {
                int hit = FindIntersecting(sprites, index, x, y);
                if (hit < 0) { sprites[index].x = x; sprites[index].y = y; return; }
                x = sprites[hit].x + sprites[hit].width;
                if (x + sprites[index].width > outputWidth) { x = 0; y++; }
            }
        }

    public:
        Texture2D&       getTextureProperty()       { return texture_; }
        const Texture2D& getTextureProperty() const { return texture_; }

        Rectangle SourceRectangle(int index) const
        {
            return spriteRectangles_.at(static_cast<std::size_t>(index));
        }

        Rectangle SourceRectangle(const std::string& name) const
        {
            return SourceRectangle(GetIndex(name));
        }

        int GetIndex(const std::string& name) const
        {
            auto it = spriteNames_.find(name);
            if (it == spriteNames_.end())
                throw std::runtime_error("SpriteSheet does not contain: " + name);
            return it->second;
        }

        // Build the atlas from a list of sprite asset names (no extension needed).
        // The order of names determines the index assigned to each sprite.
        void Build(ContentManager& content, GraphicsDevice& device,
                   const std::vector<std::string>& spriteNames)
        {
            int n = static_cast<int>(spriteNames.size());

            // Load each sprite.
            std::vector<Texture2D> sources;
            sources.reserve(n);
            for (auto& name : spriteNames)
                sources.push_back(content.Load<Texture2D>(name));

            // Read pixel data.
            std::vector<std::vector<Color>> srcPixels(n);
            for (int i = 0; i < n; i++) {
                int count = sources[i].getWidthProperty() * sources[i].getHeightProperty();
                srcPixels[i].assign(count, Color(0, 0, 0, 0));
                sources[i].GetData(srcPixels[i].data(), count);
            }

            // Build arranged sprites with 1 px padding (matching SpritePacker).
            std::vector<ArrangedSprite> arranged(n);
            for (int i = 0; i < n; i++) {
                arranged[i] = { i, 0, 0,
                                sources[i].getWidthProperty()  + 2,
                                sources[i].getHeightProperty() + 2 };
            }

            // Sort by descending size, then place.
            std::sort(arranged.begin(), arranged.end(),
                [](const ArrangedSprite& a, const ArrangedSprite& b) {
                    return (b.height * 1024 + b.width) < (a.height * 1024 + a.width);
                });

            int outputWidth  = GuessOutputWidth(arranged);
            int outputHeight = 0;
            for (int i = 0; i < n; i++) {
                PositionSprite(arranged, i, outputWidth);
                outputHeight = std::max(outputHeight, arranged[i].y + arranged[i].height);
            }

            // Sort back to original index order.
            std::sort(arranged.begin(), arranged.end(),
                [](const ArrangedSprite& a, const ArrangedSprite& b) {
                    return a.index < b.index;
                });

            // Build atlas pixel buffer.
            std::vector<Color> atlas(outputWidth * outputHeight, Color(0, 0, 0, 0));

            auto putPx = [&](int x, int y, Color c) {
                if (x >= 0 && x < outputWidth && y >= 0 && y < outputHeight)
                    atlas[y * outputWidth + x] = c;
            };
            auto getPx = [&](int si, int x, int y) -> Color {
                int w = sources[si].getWidthProperty();
                int h = sources[si].getHeightProperty();
                x = std::clamp(x, 0, w - 1);
                y = std::clamp(y, 0, h - 1);
                return srcPixels[si][y * w + x];
            };

            spriteRectangles_.resize(n);
            for (auto& s : arranged) {
                int si = s.index;
                int ox = s.x, oy = s.y;
                int w  = sources[si].getWidthProperty();
                int h  = sources[si].getHeightProperty();

                // Main body (offset +1 for padding).
                for (int y = 0; y < h; y++)
                    for (int x = 0; x < w; x++)
                        putPx(ox + 1 + x, oy + 1 + y, getPx(si, x, y));

                // Edge strips.
                for (int y = 0; y < h; y++) putPx(ox,         oy + 1 + y, getPx(si, 0,   y));
                for (int y = 0; y < h; y++) putPx(ox + w + 1, oy + 1 + y, getPx(si, w-1, y));
                for (int x = 0; x < w; x++) putPx(ox + 1 + x, oy,         getPx(si, x,   0));
                for (int x = 0; x < w; x++) putPx(ox + 1 + x, oy + h + 1, getPx(si, x,   h-1));

                // Corners.
                putPx(ox,         oy,         getPx(si, 0,   0  ));
                putPx(ox + w + 1, oy,         getPx(si, w-1, 0  ));
                putPx(ox,         oy + h + 1, getPx(si, 0,   h-1));
                putPx(ox + w + 1, oy + h + 1, getPx(si, w-1, h-1));

                // Record final rectangle (the inner region without padding).
                spriteRectangles_[si] = Rectangle(ox + 1, oy + 1, w, h);
            }

            // Register sprite names (strip file extension).
            for (int i = 0; i < n; i++) {
                std::string name = spriteNames[i];
                auto dot = name.rfind('.');
                if (dot != std::string::npos) name = name.substr(0, dot);
                spriteNames_[name] = i;
            }

            // Upload to GPU.
            texture_ = Texture2D(device, outputWidth, outputHeight);
            texture_.SetData(atlas.data(), static_cast<int>(atlas.size()));
        }
    };

} // namespace SpriteSheetSample
