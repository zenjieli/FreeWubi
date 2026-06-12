#pragma once

#include <unordered_map>
#include <utility>
#include <vector>

#include "engine_types.h"
#include "pinyin_dict.h"
#include "wubi_dict.h"

class EngineLogic {
 public:
  EngineLogic(WubiDict& dict, WubiDict& dict_common, PinyinDict& pinyin);

  // Returns true if the key was consumed.
  bool processKey(const KeyInput& input, EngineState* state, IEngineOutput* out);

  void setPageSize(int size) { pageSize_ = size; }
  int pageSize() const { return pageSize_; }

  void setTempEnglishKey(uint32_t sym) { tempEnglishKey_ = sym; }
  void setSecondTempEnglishKey(uint32_t sym) { secondTempEnglishKey_ = sym; }
  void setTempPinyinKey(uint32_t sym) { tempPinyinKey_ = sym; }

  // Custom phrases: pairs of (code, phrase)
  void setCustomPhrases(const std::vector<std::pair<std::string, std::string>>& phrases);

  // Compute wubi phrase code from a phrase string.
  // Returns empty if any character is not in the dictionary.
  std::string computePhraseCode(const std::string& phrase) const;

 private:
  void updateUI(EngineState* state, IEngineOutput* out);
  void updatePinyinUI(EngineState* state, IEngineOutput* out);
  void updateLiteralUI(EngineState* state, IEngineOutput* out);
  void updateSlashUI(EngineState* state, IEngineOutput* out);
  void commitTopCandidate(EngineState* state, IEngineOutput* out, const std::string& suffix = "");
  void exitTempPinyin(EngineState* state, IEngineOutput* out);
  std::string chinesePunct(EngineState* state, const std::string& ascii);
  WubiDict& activeDict(EngineState* state);
  // Prepend custom phrase matches for exact code to candidate list
  std::vector<CandidateEntry> mergeCustomPhrases(const std::string& code,
                                                 std::vector<PromptCandidate>& candidates) const;

  WubiDict& dict_;
  WubiDict& dict_common_;
  PinyinDict& pinyin_;
  int pageSize_ = kDefaultPageSize;
  uint32_t tempEnglishKey_ = keys::BracketLeft;
  uint32_t secondTempEnglishKey_ = keys::Slash;
  uint32_t tempPinyinKey_ = keys::BracketRight;
  // code → list of custom phrases
  std::unordered_map<std::string, std::vector<std::string>> customPhrases_;

  static const std::unordered_map<std::string, std::string> kPunctMap;
};
