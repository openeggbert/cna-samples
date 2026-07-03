#pragma once

#include "Microsoft/Xna/Framework/DisplayOrientation.hpp"

#include "Container.hpp"

namespace DynamicMenu::Controls {

using Microsoft::Xna::Framework::DisplayOrientation;

// Provides an abstraction that allows a dynamic menu to be shown both
// vertically and horizontally by breaking the control space into two
// containers. When held vertically, the two containers are on top of each
// other; when held horizontally, the containers are side by side. Port of
// Controls/PhoneScreen.cs.
class PhoneScreen : public Control {
public:
    [[nodiscard]] Container& Container1() { return container1_; }
    [[nodiscard]] Container& Container2() { return container2_; }

    [[nodiscard]] DisplayOrientation CurrentOrientation() const { return currentOrientation_; }
    void SetCurrentOrientation(DisplayOrientation value) {
        currentOrientation_ = value;
        UpdateOrientation();
    }

    void Initialize() override {
        Control::Initialize();
        container1_.Initialize();
        container2_.Initialize();
        UpdateOrientation();
    }

    void LoadContent(GraphicsDevice& graphics, ContentManager& content) override {
        Control::LoadContent(graphics, content);
        container1_.LoadContent(graphics, content);
        container2_.LoadContent(graphics, content);
    }

    void Update(const GameTime& gameTime, const std::vector<GestureSample>& gestures) override {
        Control::Update(gameTime, gestures);
        if (container1_.Visible) container1_.Update(gameTime, gestures);
        if (container2_.Visible) container2_.Update(gameTime, gestures);
    }

    void Draw(const GameTime& gameTime, SpriteBatch& spriteBatch) override {
        Control::Draw(gameTime, spriteBatch);
        if (container1_.Visible) container1_.Draw(gameTime, spriteBatch);
        if (container2_.Visible) container2_.Draw(gameTime, spriteBatch);
    }

private:
    static constexpr int ContainerWidth = 400;
    static constexpr int ContainerHeight = 400;

    // Assuming orientation of 480 x 800
    static constexpr int VerticalContainer1Left = 40;
    static constexpr int VerticalContainer1Top = 0;
    static constexpr int VerticalContainer2Left = 40;
    static constexpr int VerticalContainer2Top = 400;

    // Assuming orientation of 800 x 480
    static constexpr int HorizontalContainer1Left = 0;
    static constexpr int HorizontalContainer1Top = 40;
    static constexpr int HorizontalContainer2Left = 400;
    static constexpr int HorizontalContainer2Top = 40;

    // Changes the position of the containers according to the orientation of
    // the phone.
    void UpdateOrientation() {
        container1_.Width = ContainerWidth;
        container1_.Height = ContainerHeight;
        container2_.Width = ContainerWidth;
        container2_.Height = ContainerHeight;

        switch (currentOrientation_) {
            case DisplayOrientation::Portrait:
                container1_.Left = VerticalContainer1Left;
                container1_.Top = VerticalContainer1Top;
                container2_.Left = VerticalContainer2Left;
                container2_.Top = VerticalContainer2Top;
                break;
            case DisplayOrientation::LandscapeLeft:
            case DisplayOrientation::LandscapeRight:
                container1_.Left = HorizontalContainer1Left;
                container1_.Top = HorizontalContainer1Top;
                container2_.Left = HorizontalContainer2Left;
                container2_.Top = HorizontalContainer2Top;
                break;
            default:
                break;
        }
    }

    Container container1_;
    Container container2_;
    DisplayOrientation currentOrientation_ = DisplayOrientation::Portrait;
};

} // namespace DynamicMenu::Controls
