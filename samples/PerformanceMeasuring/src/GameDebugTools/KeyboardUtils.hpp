#pragma once

// KeyboardUtils.hpp — C++ port of GameDebugTools/KeyboardUtils.cs (XNA 4.0
// PerformanceMeasuring sample). Helper for converting a pressed key into a
// typed character, for the DebugCommandUI text input.

#include <cctype>
#include <optional>
#include <unordered_map>

#include "Microsoft/Xna/Framework/Input/Keys.hpp"

namespace PerformanceMeasuring::GameDebugTools {

using Microsoft::Xna::Framework::Input::Keys;

class KeyboardUtils {
public:
    // Gets a character from key information. Returns true when it gets a character.
    static bool KeyToString(Keys key, bool shiftKeyPressed, char& character) {
        character = ' ';

        if ((Keys::A <= key && key <= Keys::Z) || key == Keys::Space) {
            // Use as-is if it is A-Z, or Space key.
            character = shiftKeyPressed ? (char)key : (char)std::tolower((char)key);
            return true;
        }

        const auto& map = KeyMap();
        auto it = map.find(key);
        if (it != map.end()) {
            const CharPair& pair = it->second;
            if (!shiftKeyPressed) {
                character = pair.normalChar;
                return true;
            }
            if (pair.shiftChar.has_value()) {
                character = pair.shiftChar.value();
                return true;
            }
        }

        return false;
    }

private:
    struct CharPair {
        char normalChar;
        std::optional<char> shiftChar;
    };

    static const std::unordered_map<Keys, CharPair>& KeyMap() {
        static const std::unordered_map<Keys, CharPair> map = [] {
            std::unordered_map<Keys, CharPair> m;
            auto add = [&m](Keys key, const char* pair) {
                CharPair cp{pair[0], pair[1] != '\0' ? std::optional<char>(pair[1]) : std::nullopt};
                m.emplace(key, cp);
            };

            // First row of US keyboard.
            add(Keys::OemTilde, "`~");
            add(Keys::D1, "1!");
            add(Keys::D2, "2@");
            add(Keys::D3, "3#");
            add(Keys::D4, "4$");
            add(Keys::D5, "5%");
            add(Keys::D6, "6^");
            add(Keys::D7, "7&");
            add(Keys::D8, "8*");
            add(Keys::D9, "9(");
            add(Keys::D0, "0)");
            add(Keys::OemMinus, "-_");
            add(Keys::OemPlus, "=+");

            // Second row of US keyboard.
            add(Keys::OemOpenBrackets, "[{");
            add(Keys::OemCloseBrackets, "]}");
            add(Keys::OemPipe, "\\|");

            // Third row of US keyboard.
            add(Keys::OemSemicolon, ";:");
            add(Keys::OemQuotes, "'\"");
            add(Keys::OemComma, ",<");
            add(Keys::OemPeriod, ".>");
            add(Keys::OemQuestion, "/?");

            // Keypad keys of US keyboard.
            add(Keys::NumPad1, "1");
            add(Keys::NumPad2, "2");
            add(Keys::NumPad3, "3");
            add(Keys::NumPad4, "4");
            add(Keys::NumPad5, "5");
            add(Keys::NumPad6, "6");
            add(Keys::NumPad7, "7");
            add(Keys::NumPad8, "8");
            add(Keys::NumPad9, "9");
            add(Keys::NumPad0, "0");
            add(Keys::Add, "+");
            add(Keys::Divide, "/");
            add(Keys::Multiply, "*");
            add(Keys::Subtract, "-");
            add(Keys::Decimal, ".");

            return m;
        }();
        return map;
    }
};

} // namespace PerformanceMeasuring::GameDebugTools
