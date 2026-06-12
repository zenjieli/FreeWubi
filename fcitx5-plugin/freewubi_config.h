#pragma once

#include <fcitx-config/configuration.h>
#include <fcitx-config/enum.h>
#include <fcitx-config/option.h>

enum class TriggerKey { kBracketLeft, kBracketRight, kSlash, kSemicolon };
FCITX_CONFIG_ENUM_NAME(TriggerKey, "[", "]", "/", ";");

FCITX_CONFIGURATION(CustomPhraseEntry, fcitx::Option<std::string> phrase{this, "Phrase", "Phrase", ""};);

FCITX_CONFIGURATION(
    FreeWubiConfig,
    fcitx::Option<int, fcitx::IntConstrain> pageSize{this, "PageSize", "Page Size", 5, fcitx::IntConstrain(1, 10)};
    fcitx::Option<std::vector<CustomPhraseEntry>, fcitx::NoConstrain<std::vector<CustomPhraseEntry>>,
                  fcitx::DefaultMarshaller<std::vector<CustomPhraseEntry>>, fcitx::ListDisplayOptionAnnotation>
        customPhrases{
            this, "CustomPhrases", "Custom Phrases", {}, {}, {}, fcitx::ListDisplayOptionAnnotation("Phrase")};

    fcitx::Option<TriggerKey, fcitx::NoConstrain<TriggerKey>, fcitx::DefaultMarshaller<TriggerKey>,
                  fcitx::ToolTipAnnotation>
        tempEnglishKey{this,
                       "TempEnglishKey",
                       "Temp English Mode",
                       TriggerKey::kBracketLeft,
                       {},
                       {},
                       fcitx::ToolTipAnnotation("Key to enter temporary English input mode. "
                                                "Type English text and press Space or Enter to commit.")};
    fcitx::Option<TriggerKey, fcitx::NoConstrain<TriggerKey>, fcitx::DefaultMarshaller<TriggerKey>,
                  fcitx::ToolTipAnnotation>
        secondTempEnglishKey{this,
                             "SecondTempEnglishKey",
                             "2nd Temp English Mode",
                             TriggerKey::kSlash,
                             {},
                             {},
                             fcitx::ToolTipAnnotation("Key to enter a second temporary English input mode. "
                                                      "The trigger key is included in the committed text.")};
    fcitx::Option<TriggerKey, fcitx::NoConstrain<TriggerKey>, fcitx::DefaultMarshaller<TriggerKey>,
                  fcitx::ToolTipAnnotation>
        tempPinyinKey{this,
                      "TempPinyinKey",
                      "Temp Pinyin Mode",
                      TriggerKey::kBracketRight,
                      {},
                      {},
                      fcitx::ToolTipAnnotation("Key to enter temporary pinyin input mode. "
                                               "Type pinyin and select a candidate to commit.")};);
