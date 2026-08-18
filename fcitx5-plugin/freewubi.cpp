#include "freewubi.h"

#include <fcitx-config/iniparser.h>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>

#include "fcitx_output.h"

FreeWubiEngine::FreeWubiEngine(fcitx::Instance* instance) : instance_(instance), logic_(dict_, dict_common_, pinyin_) {
  instance_->inputContextManager().registerProperty("freewubiState", &stateFactory_);
  loadDictionary();
  loadCommonDictionary();
  loadFrequency();
  loadPinyin();
  reloadConfig();
}

FreeWubiEngine::~FreeWubiEngine() = default;

void FreeWubiEngine::loadDictionary() {
  std::string path =
      fcitx::StandardPath::global().locate(fcitx::StandardPath::Type::PkgData, "data/wubi86_jidian.dict.yaml");

  if (path.empty()) {
    FCITX_WARN() << "FreeWubi: dictionary not found";
    return;
  }

  if (dict_.loadFromFile(path)) {
    FCITX_INFO() << "FreeWubi: loaded " << dict_.numCodes() << " codes from " << path;
  } else {
    FCITX_WARN() << "FreeWubi: cannot open " << path;
  }
}

void FreeWubiEngine::loadCommonDictionary() {
  std::string path =
      fcitx::StandardPath::global().locate(fcitx::StandardPath::Type::PkgData, "data/wubi86_jidian_common.dict.yaml");

  if (path.empty()) {
    FCITX_WARN() << "FreeWubi: common dictionary not found, "
                    "rare-character filtering disabled";
    return;
  }

  if (dict_common_.loadFromFile(path)) {
    FCITX_INFO() << "FreeWubi: loaded " << dict_common_.numCodes() << " codes from common dict " << path;
  } else {
    FCITX_WARN() << "FreeWubi: cannot open common dict " << path;
  }
}

void FreeWubiEngine::loadFrequency() {
  std::string path = fcitx::StandardPath::global().locate(fcitx::StandardPath::Type::PkgData, "data/frequency.txt");

  if (path.empty()) {
    FCITX_WARN() << "FreeWubi: frequency data not found";
    return;
  }

  if (dict_.loadFrequency(path)) {
    FCITX_INFO() << "FreeWubi: loaded frequency data from " << path;
  } else {
    FCITX_WARN() << "FreeWubi: cannot open frequency " << path;
  }
  dict_common_.loadFrequency(path);  // also needed for prefix-match sorting
}

void FreeWubiEngine::loadPinyin() {
  std::string path = fcitx::StandardPath::global().locate(fcitx::StandardPath::Type::PkgData, "data/pinyin.txt");

  if (path.empty()) {
    FCITX_WARN() << "FreeWubi: pinyin data not found";
    return;
  }

  if (pinyin_.loadFromFile(path)) {
    FCITX_INFO() << "FreeWubi: loaded " << pinyin_.numEntries() << " pinyin entries from " << path;
  } else {
    FCITX_WARN() << "FreeWubi: cannot open pinyin " << path;
  }
}

void FreeWubiEngine::keyEvent(const fcitx::InputMethodEntry& /*entry*/, fcitx::KeyEvent& keyEvent) {
  auto* ic = keyEvent.inputContext();
  if (!ic) return;
  if (keyEvent.isRelease()) return;

  auto* state = ic->propertyFor(&stateFactory_);
  FcitxOutput out(ic, config_.pageSize.value());
  KeyInput input(keyEvent.key().sym(), static_cast<Modifiers>(static_cast<uint32_t>(keyEvent.key().states())));
  if (logic_.processKey(input, &state->engine, &out)) {
    keyEvent.filterAndAccept();
  }
}

std::string FreeWubiEngine::subModeLabelImpl(const fcitx::InputMethodEntry& /*entry*/, fcitx::InputContext& ic) {
  auto* state = ic.propertyFor(&stateFactory_);
  if (state->engine.englishMode) {
    return "En";
  }
  return {};
}

// Must stay functional even though the icon isn't visually shown —
// removing or always-returning-empty breaks the En label display.
std::string FreeWubiEngine::subModeIconImpl(const fcitx::InputMethodEntry& /*entry*/, fcitx::InputContext& ic) {
  auto* state = ic.propertyFor(&stateFactory_);
  if (state->engine.englishMode) {
    return "input-keyboard";
  }
  return {};
}

void FreeWubiEngine::reset(const fcitx::InputMethodEntry& /*entry*/, fcitx::InputContextEvent& event) {
  auto* ic = event.inputContext();
  if (!ic) return;
  auto* state = ic->propertyFor(&stateFactory_);
  bool wasEnglish = state->engine.englishMode;
  state->engine = EngineState{};
  ic->inputPanel().reset();
  ic->updatePreedit();
  ic->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  if (wasEnglish) {
    ic->updateUserInterface(fcitx::UserInterfaceComponent::StatusArea);
  }
}

const fcitx::Configuration* FreeWubiEngine::getConfig() const { return &config_; }

void FreeWubiEngine::syncConfigToEngine() {
  logic_.setPageSize(config_.pageSize.value());
  logic_.setDecimalPointAfterDigit(config_.decimalPointAfterDigit.value());

  // Convert TriggerKey enum to keysym
  auto symForKey = [](TriggerKey key) -> uint32_t {
    switch (key) {
      case TriggerKey::kBracketLeft:
        return keys::BracketLeft;
      case TriggerKey::kBracketRight:
        return keys::BracketRight;
      case TriggerKey::kSlash:
        return keys::Slash;
      case TriggerKey::kSemicolon:
        return keys::Semicolon;
    }
    return 0;
  };

  uint32_t te = symForKey(config_.tempEnglishKey.value());
  uint32_t se = symForKey(config_.secondTempEnglishKey.value());
  uint32_t tp = symForKey(config_.tempPinyinKey.value());

  // Validate: no two trigger keys may be the same.
  // If duplicates are found, reset all three to defaults and persist
  // the correction so the config UI shows the valid values on reload.
  if (te == se || te == tp || se == tp) {
    FCITX_WARN() << "FreeWubi: duplicate trigger keys detected, "
                    "resetting to defaults";
    config_.tempEnglishKey.setValue(TriggerKey::kBracketLeft);
    config_.secondTempEnglishKey.setValue(TriggerKey::kSlash);
    config_.tempPinyinKey.setValue(TriggerKey::kBracketRight);
    te = keys::BracketLeft;
    se = keys::Slash;
    tp = keys::BracketRight;
  }

  logic_.setTempEnglishKey(te);
  logic_.setSecondTempEnglishKey(se);
  logic_.setTempPinyinKey(tp);

  std::vector<std::pair<std::string, std::string>> phrases;
  for (auto& entry : config_.customPhrases.value()) {
    const std::string& phrase = entry.phrase.value();
    if (phrase.empty()) continue;
    std::string code = logic_.computePhraseCode(phrase);
    if (!code.empty()) {
      phrases.emplace_back(std::move(code), phrase);
    }
  }
  logic_.setCustomPhrases(phrases);
}

void FreeWubiEngine::setConfig(const fcitx::RawConfig& config) {
  config_.load(config, true);
  syncConfigToEngine();
  fcitx::safeSaveAsIni(config_, "conf/freewubi.config");
}

void FreeWubiEngine::reloadConfig() {
  fcitx::readAsIni(config_, "conf/freewubi.config");
  syncConfigToEngine();
}

FCITX_ADDON_FACTORY(FreeWubiEngineFactory);
