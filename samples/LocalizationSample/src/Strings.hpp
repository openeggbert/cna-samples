#pragma once

#include <array>
#include <string>

namespace LocalizationSample {

// C++ counterpart of the XNA 4.0 sample's Strings.resx family (compiled by
// .NET into a generated Strings.Designer.cs resource class). Holds the three
// localized strings for each of the sample's five languages.
struct LocalizedText {
    std::string cultureCode;   // e.g. "da-DK" -- matches CultureInfo.Name in the original
    std::string twoLetterLang; // e.g. "da"    -- matches CultureInfo.TwoLetterISOLanguageName
    std::string englishName;   // e.g. "Danish (Denmark)" -- matches CultureInfo.EnglishName
    std::string welcome;
    std::string currentLocale; // format string with one "{0}" placeholder
    std::string howToChange;
};

inline const std::array<LocalizedText, 5>& GetLocalizedTexts() {
    static const std::array<LocalizedText, 5> texts = {{
        LocalizedText{
            "en-US", "en", "English (United States)",
            "Welcome to the localization sample!",
            "Current culture: {0}",
            "To change this, alter your system settings, then restart the sample",
        },
        LocalizedText{
            "da-DK", "da", "Danish (Denmark)",
            "Velkommen til lokaliserings eksemplet!",
            "Nuværende kultur: {0}",
            "For at ændre denne, skift system instillinger og genstart eksemplet",
        },
        LocalizedText{
            "fr-FR", "fr", "French (France)",
            "Bienvenue dans l'exemple de localisation!",
            "Localisé en: {0}",
            "Pour changer cette valeur modifiez vos paramètres\nsystème puis redémarrez cet exemple",
        },
        LocalizedText{
            "ja-JP", "ja", "Japanese (Japan)",
            "ローカライズサンプルへようこそ！",
            "現在のローケル: {0}",
            "この設定を変更するにはシステム設定を変更し、サンプルを再実行してください。",
        },
        LocalizedText{
            "ko-KR", "ko", "Korean (South Korea)",
            "번역 샘플을 환영합니다!",
            "현재 언어: {0}",
            "변경을 위해서는, 시스템 설정을 바꿔야 합니다, 그리고 샘플을 재시동하십시오",
        },
    }};
    return texts;
}

} // namespace LocalizationSample
