#pragma once

#include <any>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/DisplayOrientation.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/MouseState.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "System/Random.hpp"
#include "System/TimeSpan.hpp"

#include "Controls/Button.hpp"
#include "Controls/Container.hpp"
#include "Controls/Image.hpp"
#include "Controls/Label.hpp"
#include "Controls/MultilineTextControl.hpp"
#include "Controls/PhoneScreen.hpp"
#include "Controls/ProgressBar.hpp"
#include "Transitions/Transition.hpp"

namespace DynamicMenu {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::DisplayOrientation;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::GraphicsDeviceManager;
using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::ButtonState;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Mouse;
using Microsoft::Xna::Framework::Input::MouseState;
using Microsoft::Xna::Framework::Input::Touch::GestureSample;
using Microsoft::Xna::Framework::Input::Touch::GestureType;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;
using Controls::Button;
using Controls::Container;
using Controls::Control;
using Controls::Image;
using Controls::Label;
using Controls::MultilineTextControl;
using Controls::PhoneScreen;
using Controls::ProgressBar;
using Transitions::Transition;

// Shows how to build a dynamic, data-driven-style user interface with the
// DynamicMenu control library: a 3-page menu demonstrating transitions
// (Page 1, laid out in code) and controls built from what were originally
// XML page definitions (Page 2/3 -- see BuildMenuPage2Content()/
// BuildMenuPage3Content(), and missing.md for why they're hand-built here
// instead of loaded from XML at runtime). Port of the XNA 4.0 "Dynamic Menu"
// Windows Phone sample.
class DynamicMenuGame : public Game {
public:
    DynamicMenuGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");

        // Frame rate is 30 fps by default for Windows Phone.
        setTargetElapsedTimeProperty(System::TimeSpan::FromTicks(333333));

        graphics_.setSupportedOrientationsProperty(
            DisplayOrientation::Portrait | DisplayOrientation::LandscapeLeft | DisplayOrientation::LandscapeRight);
        graphics_.setPreferredBackBufferWidthProperty(PortraitWidth);
        graphics_.setPreferredBackBufferHeightProperty(PortraitHeight);

        TouchPanel::setEnabledGesturesProperty(GestureType::Tap);

        // Portrait is the default given the back buffer width/height.
        phoneScreen_.SetCurrentOrientation(DisplayOrientation::Portrait);
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "DynamicMenuGame";
        return name;
    }

protected:
    void Initialize() override {
        TouchPanel::setDisplayWidthProperty(PortraitWidth);
        TouchPanel::setDisplayHeightProperty(PortraitHeight);
        TouchPanel::setDisplayOrientationProperty(DisplayOrientation::Portrait);

        // Build the top container (page-select buttons).
        AddPageButton(phoneScreen_.Container1(), 0, "Page 1", Page::Page1);
        AddPageButton(phoneScreen_.Container1(), 135, "Page 2", Page::Page2);
        AddPageButton(phoneScreen_.Container1(), 270, "Page 3", Page::Page3);

        dynamicControlsContainer_ = std::make_shared<Container>();
        // Fills the container
        dynamicControlsContainer_->Left = 0;
        dynamicControlsContainer_->Top = 0;
        dynamicControlsContainer_->Width = 400;
        dynamicControlsContainer_->Height = 400;

        // Set up Page 1's buttons
        hueChangeButton_ = std::make_shared<Button>();
        hueChangeButton_->Left = 100;
        hueChangeButton_->Top = 10;
        hueChangeButton_->Width = 200;
        hueChangeButton_->Height = 80;
        hueChangeButton_->Hue = Color::Green;
        hueChangeButton_->Text = "Change Hue";
        hueChangeButton_->TextColor = Color::White;
        hueChangeButton_->FontName = "Fonts/ControlFont";
        hueChangeButton_->BackTextureName = "Textures/button";
        hueChangeButton_->PressedTextureName = "Textures/buttonpressed";
        hueChangeButton_->Tapped = [this](Button& b) { HueChangeButtonTapped(b); };
        dynamicControlsContainer_->AddControl(hueChangeButton_);

        textChangeButton_ = std::make_shared<Button>();
        textChangeButton_->Left = 100;
        textChangeButton_->Top = 100;
        textChangeButton_->Width = 200;
        textChangeButton_->Height = 80;
        textChangeButton_->Hue = Color::Red;
        textChangeButton_->Text = "Index: " + std::to_string(textButtonIndex_);
        textChangeButton_->TextColor = Color::White;
        textChangeButton_->FontName = "Fonts/ControlFont";
        textChangeButton_->BackTextureName = "Textures/button";
        textChangeButton_->PressedTextureName = "Textures/buttonpressed";
        textChangeButton_->Tapped = [this](Button& b) { TextChangeButtonTapped(b); };
        dynamicControlsContainer_->AddControl(textChangeButton_);

        bouncingButton_ = std::make_shared<Button>();
        bouncingButton_->Left = 100;
        bouncingButton_->Top = 190;
        bouncingButton_->Width = 200;
        bouncingButton_->Height = 80;
        bouncingButton_->Hue = Color::Blue;
        bouncingButton_->Text = "Bounce";
        bouncingButton_->TextColor = Color::White;
        bouncingButton_->FontName = "Fonts/ControlFont";
        bouncingButton_->BackTextureName = "Textures/button";
        bouncingButton_->PressedTextureName = "Textures/buttonpressed";
        bouncingButton_->Tapped = [this](Button& b) { BouncingButtonTapped(b); };
        dynamicControlsContainer_->AddControl(bouncingButton_);

        getBigButton_ = std::make_shared<Button>();
        getBigButton_->Left = 100;
        getBigButton_->Top = 280;
        getBigButton_->Width = 200;
        getBigButton_->Height = 80;
        getBigButton_->Hue = Color::Purple;
        getBigButton_->Text = "Get big";
        getBigButton_->TextColor = Color::White;
        getBigButton_->FontName = "Fonts/ControlFont";
        getBigButton_->BackTextureName = "Textures/button";
        getBigButton_->PressedTextureName = "Textures/buttonpressed";
        getBigButton_->Tapped = [this](Button& b) { GetBigButtonTapped(b); };
        dynamicControlsContainer_->AddControl(getBigButton_);

        phoneScreen_.Container2().AddControl(dynamicControlsContainer_);
        phoneScreen_.Container2().BackTextureName = "Textures/checkerboard";

        // Set up Page 2 and Page 3's container
        loadedControlsContainer_ = std::make_shared<Container>();
        // Fills the container
        loadedControlsContainer_->Left = 0;
        loadedControlsContainer_->Top = 0;
        loadedControlsContainer_->Width = 400;
        loadedControlsContainer_->Height = 400;

        phoneScreen_.Container2().AddControl(loadedControlsContainer_);

        phoneScreen_.Initialize();

        Game::Initialize();

        // Start by showing page 1
        ShowPage(Page::Page1);
    }

    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

        phoneScreen_.LoadContent(getGraphicsDeviceProperty(), getContentProperty());

        // F1 help overlay (CNA addition beyond the XNA original).
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        // Allows the game to exit
        if (GamePad::GetState(PlayerIndex::One).IsButtonDown(Buttons::Back) ||
            Keyboard::GetState().IsKeyDown(Keys::Escape)) {
            Exit();
        }

        // CNA addition (see missing.md): there's no physical rotation sensor on
        // this desktop, so O toggles between the sample's portrait and
        // landscape layouts -- the two layouts PhoneScreen otherwise only
        // switches between via Window.OrientationChanged on a real device.
        bool curO = Keyboard::GetState().IsKeyDown(Keys::O);
        if (curO && !prevO_) ToggleOrientation();
        prevO_ = curO;

        gestureList_.clear();
        while (TouchPanel::getIsGestureAvailableProperty()) {
            gestureList_.push_back(TouchPanel::ReadGesture());
        }

        // CNA addition (see missing.md): the original never reads Mouse, only
        // TouchPanel gestures. Since CNA doesn't synthesize touch/gesture
        // events from mouse input, a left click is turned into a synthetic Tap
        // gesture here so every button in the tree gets mouse support for
        // free, with no changes to the ported Controls library itself.
        MouseState mouse = Mouse::GetState();
        bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        if (mouseDown && !prevMouseDown_) {
            Vector2 mousePos((float)mouse.getXProperty(), (float)mouse.getYProperty());
            gestureList_.emplace_back(GestureType::Tap, gameTime.getTotalGameTimeProperty(),
                                       mousePos, Vector2::Zero, Vector2::Zero, Vector2::Zero);
        }
        prevMouseDown_ = mouseDown;

        if (bouncingButtonActive_) {
            bouncingButtonLeft_ += (int)(bouncingButtonChange_ * gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());

            if (bouncingButtonLeft_ + bouncingButton_->Width > 400 && bouncingButtonChange_ > 0) {
                bouncingButtonLeft_ = 400 - bouncingButton_->Width;
                bouncingButtonChange_ *= -1;
            } else if (bouncingButtonChange_ < 0 && bouncingButtonLeft_ < bouncingButtonStartLeft_) {
                bouncingButtonLeft_ = bouncingButtonStartLeft_;
                bouncingButtonActive_ = false;
            }

            bouncingButton_->Left = bouncingButtonLeft_;
        }

        phoneScreen_.Update(gameTime, gestureList_);

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color::CornflowerBlue);

        spriteBatch_->Begin();
        phoneScreen_.Draw(gameTime, *spriteBatch_);
        spriteBatch_->End();

        // F1 help overlay (CNA addition beyond the XNA original), drawn in its
        // own screen-space batch -- see NEXT.md section 5 on why a second
        // Begin/End per frame is safe on the default EasyGL backend.
        if (helpTimer_ > 0.0f) {
            spriteBatch_->Begin();
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = (float)((vp.getWidthProperty() - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
            spriteBatch_->End();
        }

        Game::Draw(gameTime);
    }

private:
    enum class Page { Page1, Page2, Page3 };

    static constexpr int PortraitWidth = 480;
    static constexpr int PortraitHeight = 800;

    // Adds a button to change to a page of controls.
    void AddPageButton(Container& container, int left, const std::string& text, Page page) {
        auto button = std::make_shared<Button>();
        button->Left = left;
        button->Top = 10;
        button->Width = 130;
        button->Text = text;
        button->Height = 80;
        button->FontName = "Fonts/ControlFont";
        button->BackTextureName = "Textures/button";
        button->PressedTextureName = "Textures/buttonpressed";
        button->Tag = page;
        button->Tapped = [this](Button& b) { PageButtonTapped(b); };

        container.AddControl(button);
        pageButtons_.push_back(button);
    }

    // Shows the specified page of controls.
    void ShowPage(Page page) {
        // Change the color of the buttons to represent the current choice
        for (auto& pageButton : pageButtons_) {
            Page buttonPage = std::any_cast<Page>(pageButton->Tag);

            if (buttonPage == page) {
                pageButton->TextColor = Color::Black;
                pageButton->Hue = Color::Yellow;
            } else {
                pageButton->TextColor = Color::LightGray;
                pageButton->Hue = Color::DarkGray;
            }
        }

        // Hide both containers
        dynamicControlsContainer_->Visible = false;
        loadedControlsContainer_->Visible = false;

        switch (page) {
            case Page::Page1:
                dynamicControlsContainer_->Visible = true;
                break;
            case Page::Page2:
                loadedControlsContainer_->Visible = true;
                LoadControls(BuildMenuPage2Content());
                break;
            case Page::Page3: {
                loadedControlsContainer_->Visible = true;
                std::shared_ptr<Container> container = LoadControls(BuildMenuPage3Content());
                SetupPage3(*container);
                break;
            }
        }
    }

    // Loads the controls for one of the dynamically loaded pages.
    //
    // The original loads these from an XML asset (Content.Load<Container>) via
    // the XNA content pipeline's IntermediateSerializer. CNA has no content
    // pipeline, so BuildMenuPage2Content()/BuildMenuPage3Content() below build
    // the identical control tree in C++ instead -- see missing.md. The
    // Initialize()/LoadContent() call sequence below matches the original
    // exactly.
    std::shared_ptr<Container> LoadControls(std::shared_ptr<Container> loadedContainer) {
        loadedControlsContainer_->ClearControls();

        loadedContainer->Initialize();
        loadedContainer->LoadContent(getGraphicsDeviceProperty(), getContentProperty());

        loadedControlsContainer_->AddControl(loadedContainer);

        return loadedContainer;
    }

    // Sets up the actions for the dynamically loaded controls for page 3.
    void SetupPage3(Container& container) {
        auto advanceButton = std::dynamic_pointer_cast<Button>(container.FindControlByName("AdvanceButton"));
        if (!advanceButton) {
            throw std::runtime_error("Failed to find the control named AdvanceButton in BuildMenuPage3Content()");
        }

        auto progressBar = std::dynamic_pointer_cast<ProgressBar>(container.FindControlByName("ProgressBar"));
        if (!progressBar) {
            throw std::runtime_error("Failed to find the control named ProgressBar in BuildMenuPage3Content()");
        }

        advanceButton->Tag = progressBar.get();
        advanceButton->Tapped = [this](Button& b) { AdvanceButtonTapped(b); };
    }

    // Builds the same control tree the original loads from Menus\MenuPage2.xml.
    static std::shared_ptr<Container> BuildMenuPage2Content() {
        auto container = std::make_shared<Container>();
        container->Left = 0;
        container->Top = 0;
        container->Width = 300;
        container->Height = 300;

        auto image = std::make_shared<Image>();
        image->Left = 20;
        image->Top = 20;
        image->Width = 188;
        image->Height = 94;
        image->BackTextureName = "Textures/UFO";
        container->AddControl(image);

        auto text = std::make_shared<MultilineTextControl>();
        text->Left = 20;
        text->Top = 140;
        text->Width = 360;
        text->Height = 240;
        text->BackTextureName = "Textures/textbox";
        text->Text = "This is a text box with a whole lot of text.  It uses a MultilineTextControl.";
        text->FontName = "Fonts/ControlFont";
        container->AddControl(text);

        return container;
    }

    // Builds the same control tree the original loads from Menus\MenuPage3.xml.
    static std::shared_ptr<Container> BuildMenuPage3Content() {
        auto container = std::make_shared<Container>();
        container->Left = 0;
        container->Top = 0;
        container->Width = 300;
        container->Height = 300;

        auto label = std::make_shared<Label>();
        label->Left = 20;
        label->Top = 20;
        label->Width = 200;
        label->Height = 60;
        label->BackTextureName = "Textures/textbox";
        label->Text = "Progress Bar";
        label->FontName = "Fonts/ControlFont";
        container->AddControl(label);

        auto progressBar = std::make_shared<ProgressBar>();
        progressBar->Left = 20;
        progressBar->Top = 100;
        progressBar->Width = 360;
        progressBar->Height = 60;
        progressBar->Name = "ProgressBar";
        progressBar->BackTextureName = "Textures/textbox";
        progressBar->LeftTextureName = "Textures/progressleft";
        progressBar->RightTextureName = "Textures/progressright";
        progressBar->BorderWidth = 5;
        container->AddControl(progressBar);

        auto advanceButton = std::make_shared<Button>();
        advanceButton->Left = 100;
        advanceButton->Top = 180;
        advanceButton->Width = 200;
        advanceButton->Height = 80;
        advanceButton->Name = "AdvanceButton";
        advanceButton->BackTextureName = "Textures/button";
        advanceButton->Text = "Advance";
        advanceButton->FontName = "Fonts/ControlFont";
        advanceButton->PressedTextureName = "Textures/buttonpressed";
        container->AddControl(advanceButton);

        return container;
    }

    // CNA addition (see missing.md): swaps the back buffer between portrait
    // and landscape and re-lays-out PhoneScreen accordingly, standing in for
    // the original's Window.OrientationChanged handler (there's no physical
    // rotation sensor on this desktop to raise it).
    void ToggleOrientation() {
        bool toLandscape = phoneScreen_.CurrentOrientation() == DisplayOrientation::Portrait;

        graphics_.setPreferredBackBufferWidthProperty(toLandscape ? PortraitHeight : PortraitWidth);
        graphics_.setPreferredBackBufferHeightProperty(toLandscape ? PortraitWidth : PortraitHeight);
        graphics_.ApplyChanges();

        DisplayOrientation newOrientation = toLandscape ? DisplayOrientation::LandscapeLeft : DisplayOrientation::Portrait;
        TouchPanel::setDisplayWidthProperty(toLandscape ? PortraitHeight : PortraitWidth);
        TouchPanel::setDisplayHeightProperty(toLandscape ? PortraitWidth : PortraitHeight);
        TouchPanel::setDisplayOrientationProperty(newOrientation);
        phoneScreen_.SetCurrentOrientation(newOrientation);
    }

    // Handler for a page button. Changes the page of controls being shown.
    void PageButtonTapped(Button& button) {
        ShowPage(std::any_cast<Page>(button.Tag));
    }

    // Handler for Page 1's hue change button. Applies a random color to the
    // background of the button.
    void HueChangeButtonTapped(Button& button) {
        // Choose a random color, 0.0-1.0 for each component
        float r = (float)random_.NextDouble();
        float g = (float)random_.NextDouble();
        float b = (float)random_.NextDouble();
        // Make the alpha value a random value between 0.5 and 1.0
        float a = (float)(random_.NextDouble() * 0.5 + 0.5);

        Color newColor = Color(r, g, b) * a;
        // Font color complements the hue
        Color fontColor(1.0f - r, 1.0f - g, 1.0f - b);

        // Apply a transition which changes the hue over 2 seconds
        Transition transition(std::nullopt, std::nullopt, std::nullopt, std::nullopt, button.Hue, newColor);
        transition.TransitionLength = 2.0f;

        button.ApplyTransition(transition);

        // Set the text color immediately
        button.TextColor = fontColor;
    }

    // Handler for Page 1's text change button.
    void TextChangeButtonTapped(Button& button) {
        ++textButtonIndex_;
        button.Text = "Index: " + std::to_string(textButtonIndex_);
    }

    // Handler for Page 1's bouncing button. Starts the process to animate the
    // position of the button.
    void BouncingButtonTapped(Button& button) {
        // don't respond when we're already bouncing
        if (bouncingButtonActive_) return;

        bouncingButtonActive_ = true;
        if (bouncingButtonChange_ < 0) {
            bouncingButtonChange_ *= -1;
        }
        bouncingButtonStartLeft_ = button.Left;
        bouncingButtonLeft_ = button.Left;
    }

    // Handler for Page 1's getBig button. Applies a transition to make the
    // control larger.
    void GetBigButtonTapped(Button& button) {
        Transition transition(std::nullopt, Point(0, 240), std::nullopt, Point(400, 160), std::nullopt, std::nullopt);
        transition.TransitionComplete = [this](Transition& t) { GetBigTransitionComplete(t); };

        button.ApplyTransition(transition);
    }

    // Handler for the completion of Page 1's getBig button transition. Returns
    // the control to its normal size.
    void GetBigTransitionComplete(Transition& oldTransition) {
        auto* button = static_cast<Button*>(oldTransition.Control);

        Transition newTransition(std::nullopt, Point(100, 280), std::nullopt, Point(200, 80), std::nullopt, std::nullopt);
        button->ApplyTransition(newTransition);
    }

    // Handler for Page 3's Advance button. Increments the position of the
    // progress bar.
    void AdvanceButtonTapped(Button& button) {
        auto* progressBar = std::any_cast<ProgressBar*>(button.Tag);

        int curProgress = progressBar->Position + 10;
        // The position starts at the beginning again
        if (curProgress > progressBar->MaxValue) {
            curProgress = 0;
        }
        progressBar->Position = curProgress;
    }

    GraphicsDeviceManager graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;
    PhoneScreen phoneScreen_;

    std::shared_ptr<Container> dynamicControlsContainer_;
    std::shared_ptr<Container> loadedControlsContainer_;

    std::vector<std::shared_ptr<Button>> pageButtons_;

    // Page 1 controls
    std::shared_ptr<Button> hueChangeButton_;
    std::shared_ptr<Button> textChangeButton_;
    std::shared_ptr<Button> bouncingButton_;
    std::shared_ptr<Button> getBigButton_;

    // Page 1 bouncing button parameters
    bool bouncingButtonActive_ = false;
    int bouncingButtonChange_ = 200;
    int bouncingButtonLeft_ = 0;
    int bouncingButtonStartLeft_ = 0;

    int textButtonIndex_ = 1;

    System::Random random_;

    // Used for passing to the menu controls
    std::vector<GestureSample> gestureList_;

    bool prevO_ = false;
    bool prevMouseDown_ = false;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;
};

} // namespace DynamicMenu
