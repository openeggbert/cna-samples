#pragma once

// OptionsMenu.hpp -- C++ port of Screens/OptionsMenu.cs (XNA 4.0
// CardsStarterKit sample). Lets the player toggle the card-back deck theme
// (Red/Blue), previewing it with a live AnimatedGameComponent card.

#include <map>
#include <memory>
#include <string>

#include "../../CardsFramework/AnimatedGameComponent.hpp"
#include "../../GameStateManagement/MenuScreen.hpp"
#include "../BlackjackCommon.hpp"
#include "MainMenuScreen.hpp"

namespace Blackjack {

using CardsFramework::AnimatedGameComponent;
using GameStateManagement::MenuEntry;
using GameStateManagement::mul;
using GameStateManagement::MenuScreen;

class OptionsMenu : public MenuScreen {
public:
    OptionsMenu() : MenuScreen("") {}

    void LoadContent() override {
        safeArea_ = GetScreenManager()->SafeArea();

        auto themeGameMenuEntry = std::make_shared<MenuEntry>("Deck");
        auto returnMenuEntry = std::make_shared<MenuEntry>("Return");

        themeGameMenuEntry->Selected = [this](PlayerIndex) { ThemeGameMenuEntrySelected(); };
        returnMenuEntry->Selected = [this](PlayerIndex p) { OnCancel(p); };

        MenuEntries().push_back(themeGameMenuEntry);
        MenuEntries().push_back(returnMenuEntry);

        themes_.emplace("Red", Load<Texture2D>("Images/Cards/CardBack_Red"));
        themes_.emplace("Blue", Load<Texture2D>("Images/Cards/CardBack_Blue"));
        background_ = Load<Texture2D>("Images/UI/table");

        card_ = std::make_shared<AnimatedGameComponent>(GetScreenManager()->getGameProperty(),
                                                        &themes_.at(MainMenuScreen::Theme));
        card_->CurrentPosition =
            Vector2((float)(safeArea_.X + safeArea_.Width / 2), (float)(safeArea_.Y + safeArea_.Height / 2 - 50));
        GetScreenManager()->getGameProperty().getComponentsProperty().Add(card_.get());

        MenuScreen::LoadContent();
    }

    void Draw(const GameTime& gameTime) override {
        GetScreenManager()->getSpriteBatch().Begin();
        GetScreenManager()->getSpriteBatch().Draw(
            background_, GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty().getBoundsProperty(),
            mul(Color::White, TransitionAlpha()));
        GetScreenManager()->getSpriteBatch().End();
        MenuScreen::Draw(gameTime);
    }

private:
    void ThemeGameMenuEntrySelected() {
        MainMenuScreen::Theme = (MainMenuScreen::Theme == "Red") ? "Blue" : "Red";
        card_->CurrentFrame = &themes_.at(MainMenuScreen::Theme);
    }

    void OnCancel(PlayerIndex playerIndex) override {
        (void)playerIndex;
        (void)GetScreenManager()->getGameProperty().getComponentsProperty().Remove(card_.get());
        ExitScreen();
    }

    std::map<std::string, Texture2D> themes_;
    std::shared_ptr<AnimatedGameComponent> card_;
    Texture2D background_;
    Rectangle safeArea_;
};

// ---- MainMenuScreen::ThemeGameMenuEntrySelected (needs OptionsMenu complete) ----

inline void MainMenuScreen::ThemeGameMenuEntrySelected() {
    GetScreenManager()->AddScreen(std::make_shared<OptionsMenu>(), std::nullopt);
}

} // namespace Blackjack
