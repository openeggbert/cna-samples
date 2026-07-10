#pragma once
#include <memory>
#include <string>
#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/Viewport.hpp"

namespace SafeArea {

class SafeAreaOverlay : public Microsoft::Xna::Framework::DrawableGameComponent {
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::SpriteBatch> spriteBatch_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> dummyTexture_;

public:
    explicit SafeAreaOverlay(Microsoft::Xna::Framework::Game& game)
        : DrawableGameComponent(game)
    {
        setDrawOrderProperty(1000);
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "SafeAreaOverlay";
        return name;
    }

protected:
    void LoadContent() override {
        using namespace Microsoft::Xna::Framework;
        using namespace Microsoft::Xna::Framework::Graphics;
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        dummyTexture_ = std::make_unique<Texture2D>(getGraphicsDeviceProperty(), 1, 1);
        Color white = Color::White;
        dummyTexture_->SetData(&white, 1);
    }

    void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) override {
        using namespace Microsoft::Xna::Framework;
        using namespace Microsoft::Xna::Framework::Graphics;

        Viewport viewport = getGraphicsDeviceProperty().getViewportProperty();
        Rectangle safeArea = viewport.getTitleSafeAreaProperty();

        int viewportRight  = viewport.getXProperty() + viewport.getWidthProperty();
        int viewportBottom = viewport.getYProperty() + viewport.getHeightProperty();

        Rectangle leftBorder(viewport.getXProperty(), viewport.getYProperty(),
                              safeArea.getLeftProperty() - viewport.getXProperty(),
                              viewport.getHeightProperty());
        Rectangle rightBorder(safeArea.getRightProperty(), viewport.getYProperty(),
                               viewportRight - safeArea.getRightProperty(),
                               viewport.getHeightProperty());
        Rectangle topBorder(safeArea.getLeftProperty(), viewport.getYProperty(),
                             safeArea.Width,
                             safeArea.getTopProperty() - viewport.getYProperty());
        Rectangle bottomBorder(safeArea.getLeftProperty(), safeArea.getBottomProperty(),
                                safeArea.Width,
                                viewportBottom - safeArea.getBottomProperty());

        Color translucentRed = Color::Red * 0.5f;

        spriteBatch_->Begin();
        spriteBatch_->Draw(*dummyTexture_, leftBorder,   translucentRed);
        spriteBatch_->Draw(*dummyTexture_, rightBorder,  translucentRed);
        spriteBatch_->Draw(*dummyTexture_, topBorder,    translucentRed);
        spriteBatch_->Draw(*dummyTexture_, bottomBorder, translucentRed);
        spriteBatch_->End();
    }
};

} // namespace SafeArea
