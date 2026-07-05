#pragma once

// DialogueScreen.hpp -- simplified adaptation of GameScreens/DialogueScreen.cs.
// The original renders a wooden dialogue-box texture with wrapped body text
// and Back/Select button icons; this port keeps the same TitleText/
// DialogueText/BackText/SelectText fields and Ok/Cancel semantics but draws
// them as plain strings over a translucent panel instead of the bespoke
// artwork. See missing.md.

#include <functional>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "../AudioManager.hpp"
#include "../Fonts.hpp"
#include "../InputManager.hpp"
#include "../ScreenManager/GameScreen.hpp"
#include "../ScreenManager/ScreenManager.hpp"

namespace RolePlaying {

class DialogueScreen : public GameScreen {
public:
    DialogueScreen() {
        SetIsPopup(true);
        SetTransitionOnTime(System::TimeSpan::FromSeconds(0.3));
        SetTransitionOffTime(System::TimeSpan::FromSeconds(0.3));
    }

    std::string TitleText;
    std::string DialogueText;
    std::string SelectText = "Ok";
    std::string BackText = "Back";

    std::function<void()> OnSelect;
    std::function<void()> OnBack;

    void HandleInput() override {
        if (InputManager::IsActionTriggered(InputManager::Action::Ok)) {
            AudioManager::PlayCue("Continue");
            ExitScreen();
            if (OnSelect) OnSelect();
        } else if (!BackText.empty() && InputManager::IsActionTriggered(InputManager::Action::Back)) {
            ExitScreen();
            if (OnBack) OnBack();
        }
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        auto& spriteBatch = GetScreenManager()->getSpriteBatch();
        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
        int w = viewport.getWidthProperty();
        int h = viewport.getHeightProperty();

        spriteBatch.Begin();
        Microsoft::Xna::Framework::Rectangle panel(w / 2 - 300, h / 2 - 150, 600, 300);
        if (!TitleText.empty())
            spriteBatch.DrawString(Fonts::HeaderFont(), TitleText,
                                   Microsoft::Xna::Framework::Vector2((float)panel.X + 20, (float)panel.Y + 15),
                                   Fonts::CaptionColor);
        auto lines = Fonts::BreakTextIntoList(DialogueText, Fonts::DescriptionFont(), panel.Width - 40);
        float y = (float)panel.Y + 60;
        for (auto& line : lines) {
            spriteBatch.DrawString(Fonts::DescriptionFont(), line,
                                   Microsoft::Xna::Framework::Vector2((float)panel.X + 20, y),
                                   Microsoft::Xna::Framework::Color(255, 255, 255, 255));
            y += 26;
        }
        std::string footer = SelectText;
        if (!BackText.empty()) footer += "   /   " + BackText;
        spriteBatch.DrawString(Fonts::ButtonNamesFont(), footer,
                               Microsoft::Xna::Framework::Vector2((float)panel.X + 20, (float)(panel.Y + panel.Height - 30)),
                               Fonts::HighlightColor);
        spriteBatch.End();
    }
};

} // namespace RolePlaying
