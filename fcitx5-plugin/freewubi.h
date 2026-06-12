#pragma once

#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>

#include <string>

#include "engine_logic.h"
#include "freewubi_config.h"
#include "pinyin_dict.h"
#include "wubi_dict.h"

class FreeWubiState : public fcitx::InputContextProperty {
 public:
  EngineState engine;
};

class FreeWubiEngine : public fcitx::InputMethodEngineV2 {
 public:
  FreeWubiEngine(fcitx::Instance* instance);
  ~FreeWubiEngine() override;

  void keyEvent(const fcitx::InputMethodEntry& entry, fcitx::KeyEvent& keyEvent) override;

  void reset(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) override;

  const fcitx::Configuration* getConfig() const override;
  void setConfig(const fcitx::RawConfig& config) override;
  void reloadConfig() override;

  std::string subModeLabelImpl(const fcitx::InputMethodEntry& entry, fcitx::InputContext& ic) override;

  std::string subModeIconImpl(const fcitx::InputMethodEntry& entry, fcitx::InputContext& ic) override;

  fcitx::Instance* instance() { return instance_; }

 private:
  void loadDictionary();
  void loadCommonDictionary();
  void loadFrequency();
  void loadPinyin();
  void syncConfigToEngine();

  fcitx::Instance* instance_;
  fcitx::FactoryFor<FreeWubiState> stateFactory_{[](fcitx::InputContext&) { return new FreeWubiState; }};

  WubiDict dict_;         // full dictionary
  WubiDict dict_common_;  // common-only dictionary (no rare characters)
  PinyinDict pinyin_;
  EngineLogic logic_;
  FreeWubiConfig config_;
};

class FreeWubiEngineFactory : public fcitx::AddonFactory {
  fcitx::AddonInstance* create(fcitx::AddonManager* manager) override {
    return new FreeWubiEngine(manager->instance());
  }
};
