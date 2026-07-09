#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <vector>

#include <Microsoft/Xna/Framework/DrawableGameComponent.hpp>
#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>
#include <Microsoft/Xna/Framework/Vector4.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/Viewport.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteSortMode.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>
#include <Microsoft/Xna/Framework/Graphics/SamplerState.hpp>
#include <Microsoft/Xna/Framework/Graphics/OcclusionQuery.hpp>
#include <Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp>
#include <Microsoft/Xna/Framework/Graphics/EffectTechnique.hpp>
#include <Microsoft/Xna/Framework/Graphics/EffectPassCollection.hpp>
#include <Microsoft/Xna/Framework/Graphics/EffectPass.hpp>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace LensFlareSample {

// Reusable component for drawing a lensflare effect over the top of a 3D scene.
class LensFlareComponent : public DrawableGameComponent {
public:
    // Set by the main game to tell us the position of the camera and sun.
    Matrix View;
    Matrix Projection;
    Vector3 LightDirection = Vector3::Normalize(Vector3(-1.0f, -0.1f, 0.3f));

    explicit LensFlareComponent(Game& game) : DrawableGameComponent(game) {}

    const std::string& GetTypeName() const override {
        static const std::string name = "LensFlareComponent";
        return name;
    }

    void Draw(const GameTime& gameTime) override {
        UpdateOcclusion();
        DrawGlow();
        DrawFlares();
        RestoreRenderStates();
    }

protected:
    void LoadContent() override {
        spriteBatch_.emplace(getGraphicsDeviceProperty());

        glowSprite_ = getGameProperty().getContentProperty().Load<Texture2D>("glow");

        for (Flare& flare : flares_) {
            flare.texture = getGameProperty().getContentProperty().Load<Texture2D>(flare.textureName);
        }

        // Effect for drawing occlusion query polygons.
        basicEffect_.emplace(getGraphicsDeviceProperty());
        basicEffect_->setViewProperty(Matrix::getIdentityProperty());
        basicEffect_->VertexColorEnabled = true;

        // Create vertex data for the occlusion query polygons.
        queryVertices_[0].Position = Vector3(-querySize / 2, -querySize / 2, -1);
        queryVertices_[1].Position = Vector3(querySize / 2, -querySize / 2, -1);
        queryVertices_[2].Position = Vector3(-querySize / 2, querySize / 2, -1);
        queryVertices_[3].Position = Vector3(querySize / 2, querySize / 2, -1);

        occlusionQuery_.emplace(getGraphicsDeviceProperty());
    }

private:
    // How big is the circular glow effect?
    static constexpr float glowSize = 400;

    // How big a rectangle should we examine when issuing our occlusion queries?
    // Increasing this makes the flares fade out more gradually when the sun goes
    // behind scenery, while smaller query areas cause sudden on/off transitions.
    static constexpr float querySize = 100;

    // The lensflare effect is made up from several individual flare graphics, which
    // move across the screen depending on the position of the sun. Keeps track of
    // the position, size, and color for each flare.
    struct Flare {
        float position;
        float scale;
        Color color;
        std::string textureName;
        Texture2D texture;
    };

    // Array describes the position, size, color, and texture for each individual
    // flare graphic. The position value lies on a line between the sun and the
    // center of the screen. Zero places a flare directly over the top of the sun,
    // one is exactly in the middle of the screen, fractional positions lie in
    // between these two points, while negative values or positions greater than
    // one will move the flares outward toward the edge of the screen.
    std::vector<Flare> flares_ = {
        { -0.5f, 0.7f, Color( 50,  25,  50, 255), "flare1", Texture2D() },
        {  0.3f, 0.4f, Color(100, 255, 200, 255), "flare1", Texture2D() },
        {  1.2f, 1.0f, Color(100,  50,  50, 255), "flare1", Texture2D() },
        {  1.5f, 1.5f, Color( 50, 100,  50, 255), "flare1", Texture2D() },

        { -0.3f, 0.7f, Color(200,  50,  50, 255), "flare2", Texture2D() },
        {  0.6f, 0.9f, Color( 50, 100,  50, 255), "flare2", Texture2D() },
        {  0.7f, 0.4f, Color( 50, 200, 200, 255), "flare2", Texture2D() },

        { -0.7f, 0.7f, Color( 50, 100,  25, 255), "flare3", Texture2D() },
        {  0.0f, 0.6f, Color( 25,  25,  25, 255), "flare3", Texture2D() },
        {  2.0f, 1.4f, Color( 25,  50, 100, 255), "flare3", Texture2D() },
    };

    // Computed by UpdateOcclusion, which projects LightDirection into screenspace.
    Vector2 lightPosition_;
    bool lightBehindCamera_ = false;

    // Graphics objects.
    Texture2D glowSprite_;
    std::optional<SpriteBatch> spriteBatch_;
    std::optional<BasicEffect> basicEffect_;
    std::array<VertexPositionColor, 4> queryVertices_;

    // Custom blend state so the occlusion query polygons do not show up on screen.
    BlendState colorWriteDisable_ = [] {
        BlendState bs;
        bs.setColorWriteChannelsProperty(ColorWriteChannels::None);
        return bs;
    }();

    // An occlusion query is used to detect when the sun is hidden behind scenery.
    std::optional<OcclusionQuery> occlusionQuery_;
    bool occlusionQueryActive_ = false;
    float occlusionAlpha_ = 0.0f;

    // Measures how much of the sun is visible, by drawing a small rectangle,
    // centered on the sun, but with the depth set to as far away as possible,
    // and using an occlusion query to measure how many of these very-far-away
    // pixels are not hidden behind the terrain.
    //
    // The problem with occlusion queries is that the graphics card runs in
    // parallel with the CPU. When you issue drawing commands, they are just
    // stored in a buffer, and the graphics card can be as much as a frame delayed
    // in getting around to processing the commands from that buffer. This means
    // that even after we issue our occlusion query, the occlusion results will
    // not be available until later, after the graphics card finishes processing
    // these commands.
    //
    // It would slow our game down too much if we waited for the graphics card,
    // so instead we delay our occlusion processing by one frame. Each time
    // around the game loop, we read back the occlusion results from the previous
    // frame, then issue a new occlusion query ready for the next frame to read
    // its result. This keeps the data flowing smoothly between the CPU and GPU,
    // but also causes our data to be a frame out of date: we are deciding
    // whether or not to draw our lensflare effect based on whether it was
    // visible in the previous frame, as opposed to the current one! Fortunately,
    // the camera tends to move slowly, and the lensflare fades in and out
    // smoothly as it goes behind the scenery, so this out-by-one-frame error
    // is not too noticeable in practice.
    void UpdateOcclusion() {
        // The sun is infinitely distant, so it should not be affected by the
        // position of the camera. Floating point math doesn't support infinitely
        // distant vectors, but we can get the same result by making a copy of our
        // view matrix, then resetting the view translation to zero. Pretending the
        // camera has not moved position gives the same result as if the camera
        // was moving, but the light was infinitely far away. If our flares came
        // from a local object rather than the sun, we would use the original view
        // matrix here.
        Matrix infiniteView = View;
        infiniteView.setTranslationProperty(Vector3::Zero);

        // Project the light position into 2D screen space.
        Viewport viewport = getGraphicsDeviceProperty().getViewportProperty();

        Vector3 projectedPosition = viewport.Project(-LightDirection, Projection,
                                                       infiniteView, Matrix::getIdentityProperty());

        // Don't draw any flares if the light is behind the camera.
        if (projectedPosition.Z < 0 || projectedPosition.Z > 1) {
            lightBehindCamera_ = true;
            return;
        }

        lightPosition_ = Vector2(projectedPosition.X, projectedPosition.Y);
        lightBehindCamera_ = false;

        if (occlusionQueryActive_) {
            // If the previous query has not yet completed, wait until it does.
            if (!occlusionQuery_->getIsCompleteProperty())
                return;

            // Use the occlusion query pixel count to work
            // out what percentage of the sun is visible.
            constexpr float queryArea = querySize * querySize;

            occlusionAlpha_ = std::min(occlusionQuery_->getPixelCountProperty() / queryArea, 1.0f);
        }

        // Set renderstates for drawing the occlusion query geometry. We want depth
        // tests enabled, but depth writes disabled, and we disable color writes
        // to prevent this query polygon actually showing up on the screen.
        getGraphicsDeviceProperty().setBlendStateProperty(colorWriteDisable_);
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::DepthRead);

        // Set up our BasicEffect to center on the current 2D light position.
        basicEffect_->setWorldProperty(Matrix::CreateTranslation(lightPosition_.X, lightPosition_.Y, 0));

        basicEffect_->setProjectionProperty(Matrix::CreateOrthographicOffCenter(
            0, (float)viewport.getWidthProperty(), (float)viewport.getHeightProperty(), 0, 0, 1));

        basicEffect_->getCurrentTechniqueProperty()->getPassesProperty()[0].Apply();

        // Issue the occlusion query.
        occlusionQuery_->Begin();

        getGraphicsDeviceProperty().DrawUserPrimitives(PrimitiveType::TriangleStrip,
                                                        queryVertices_.data(), 0, 2);

        occlusionQuery_->End();

        occlusionQueryActive_ = true;
    }

    // Draws a large circular glow sprite, centered on the sun.
    void DrawGlow() {
        if (lightBehindCamera_ || occlusionAlpha_ <= 0)
            return;

        Color color = Color::White * occlusionAlpha_;
        Vector2 origin = Vector2((float)glowSprite_.getWidthProperty(),
                                  (float)glowSprite_.getHeightProperty()) / 2;
        float scale = glowSize * 2 / (float)glowSprite_.getWidthProperty();

        spriteBatch_->Begin();

        spriteBatch_->Draw(glowSprite_, lightPosition_, std::nullopt, color, 0,
                            origin, scale, SpriteEffects::None, 0);

        spriteBatch_->End();
    }

    // Draws the lensflare sprites, computing the position
    // of each one based on the current angle of the sun.
    void DrawFlares() {
        if (lightBehindCamera_ || occlusionAlpha_ <= 0)
            return;

        Viewport viewport = getGraphicsDeviceProperty().getViewportProperty();

        // Lensflare sprites are positioned at intervals along a line that
        // runs from the 2D light position toward the center of the screen.
        Vector2 screenCenter = Vector2((float)viewport.getWidthProperty(),
                                        (float)viewport.getHeightProperty()) / 2;

        Vector2 flareVector = screenCenter - lightPosition_;

        // Draw the flare sprites using additive blending.
        spriteBatch_->Begin(SpriteSortMode::Deferred, BlendState::Additive);

        for (const Flare& flare : flares_) {
            // Compute the position of this flare sprite.
            Vector2 flarePosition = lightPosition_ + flareVector * flare.position;

            // Set the flare alpha based on the previous occlusion query result.
            Vector4 flareColor = flare.color.ToVector4();

            flareColor.W *= occlusionAlpha_;

            // Center the sprite texture.
            Vector2 flareOrigin = Vector2((float)flare.texture.getWidthProperty(),
                                           (float)flare.texture.getHeightProperty()) / 2;

            // Draw the flare.
            spriteBatch_->Draw(flare.texture, flarePosition, std::nullopt,
                                Color(flareColor), 1, flareOrigin,
                                flare.scale, SpriteEffects::None, 0);
        }

        spriteBatch_->End();
    }

    // Sets renderstates back to their default values after we finish drawing
    // the lensflare, to avoid messing up the 3D terrain rendering.
    void RestoreRenderStates() {
        getGraphicsDeviceProperty().setBlendStateProperty(BlendState::Opaque);
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::Default);
        getGraphicsDeviceProperty().getSamplerStatesProperty()[0] = SamplerState::LinearWrap;
    }
};

} // namespace LensFlareSample
