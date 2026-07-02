#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "System/String.hpp"

#include "Strings.hpp"

namespace LocalizationSample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::GraphicsDeviceManager;
using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;

// Sample showing how to localize an XNA Framework game into different
// languages. Port of the XNA 4.0 "Localization" sample.
//
// CNA addition: sharp-runtime's System::Globalization::CultureInfo::CurrentCulture
// is a stub that always returns the invariant culture (there is no OS locale
// detection), so the original's "shows whatever your Windows locale is" demo
// can't run automatically. Instead, SPACE cycles through the five languages
// the sample ships strings for, so the LoadLocalizedAsset<T> fallback
// algorithm (try full culture name, then language-only, then default) is
// still exercised end-to-end for the Flag texture. See missing.md.
class LocalizationGame : public Game {
public:
    LocalizationGame() : graphics_(this) {
        graphics_.setPreferredBackBufferWidthProperty(800);
        graphics_.setPreferredBackBufferHeightProperty(480);
        getContentProperty().setRootDirectoryProperty("Content");
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "LocalizationGame";
        return name;
    }

protected:
    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        font_.emplace(getContentProperty().Load<SpriteFont>("Fonts/Font"));
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        LoadCurrentFlag();
    }

    void Update(GameTime& gameTime) override {
        float elapsed = static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput();

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        const LocalizedText& text = GetLocalizedTexts()[cultureIndex_];

        std::string string1 = text.welcome;
        std::string string2 = System::String::Format(text.currentLocale, text.englishName);
        std::string string3 = text.howToChange;

        getGraphicsDeviceProperty().Clear(Color::CornflowerBlue);

        spriteBatch_->Begin();

        spriteBatch_->DrawString(*font_, string1, Vector2(100.0f, 100.0f), Color::White);
        spriteBatch_->DrawString(*font_, string2, Vector2(100.0f, 130.0f), Color::White);
        spriteBatch_->DrawString(*font_, string3, Vector2(100.0f, 160.0f), Color::White);

        spriteBatch_->Draw(currentFlag_, Vector2(100.0f, 250.0f), Color::White);

        spriteBatch_->DrawString(*font_, "[SPACE] change language", Vector2(100.0f, 400.0f), Color::White);

        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = static_cast<float>((vp.getWidthProperty() - hw) / 2);
            float sy = static_cast<float>((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }

        spriteBatch_->End();

        Game::Draw(gameTime);
    }

private:
    // Loads a .xnb-style asset that can have multiple localized versions.
    // If a default asset is named "Foo", a specialized French version can be
    // "Foo.fr" and a Japanese one "Foo.ja"; specializing by country as well as
    // language works too ("Foo.en-US" vs "Foo.en-GB"). Looks first for the
    // most specialized (language + country) version, then language-only,
    // then falls back to the plain default name.
    // Note: caught as std::runtime_error rather than ContentLoadException --
    // ContentManager's generic Load<T> throws ContentLoadException for a
    // missing asset, but the Texture2D specialization's underlying image
    // reader currently raises a plain std::runtime_error instead (see
    // missing.md). Catching the common base covers both.
    template <typename T>
    T LoadLocalizedAsset(const std::string& assetName, const LocalizedText& text) {
        for (const std::string& cultureName : {text.cultureCode, text.twoLetterLang}) {
            try {
                return getContentProperty().Load<T>(assetName + "." + cultureName);
            } catch (const std::runtime_error&) {
            }
        }

        return getContentProperty().Load<T>(assetName);
    }

    void LoadCurrentFlag() {
        currentFlag_ = LoadLocalizedAsset<Texture2D>("Images/Flag", GetLocalizedTexts()[cultureIndex_]);
    }

    void HandleInput() {
        KeyboardState kb = Keyboard::GetState();

        if (kb.IsKeyDown(Keys::Escape) || GamePad::GetState(PlayerIndex::One).IsButtonDown(Buttons::Back)) {
            Exit();
        }

        bool curSpace = kb.IsKeyDown(Keys::Space);
        if (curSpace && !prevSpace_) {
            cultureIndex_ = (cultureIndex_ + 1) % GetLocalizedTexts().size();
            LoadCurrentFlag();
        }
        prevSpace_ = curSpace;
    }

    GraphicsDeviceManager graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont> font_;
    Texture2D currentFlag_;

    std::size_t cultureIndex_ = 0;
    bool prevSpace_ = false;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;
};

} // namespace LocalizationSample
