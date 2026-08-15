#include "engine_logic.h"

#include <algorithm>
#include <cstdlib>

const std::unordered_map<std::string, std::string> EngineLogic::kPunctMap = {
    {",", "\xef\xbc\x8c"},              // ，
    {".", "\xe3\x80\x82"},              // 。
    {"!", "\xef\xbc\x81"},              // ！
    {"?", "\xef\xbc\x9f"},              // ？
    {";", "\xef\xbc\x9b"},              // ；
    {":", "\xef\xbc\x9a"},              // ：
    {"^", "\xe2\x80\xa6\xe2\x80\xa6"},  // ……
    {"<", "\xe3\x80\x8a"},              // 《
    {">", "\xe3\x80\x8b"},              // 》
    {"_", "\xe2\x80\x94\xe2\x80\x94"},  // ——
    {"\\", "\xe3\x80\x81"},             // 、
};

EngineLogic::EngineLogic(WubiDict& dict, WubiDict& dict_common, PinyinDict& pinyin)
    : dict_(dict), dict_common_(dict_common), pinyin_(pinyin) {}

bool EngineLogic::processKey(const KeyInput& input, EngineState* state, IEngineOutput* out) {
  bool prevCharWasDigit = state->lastCharWasDigit;
  state->lastCharWasDigit = false;

  // Right Ctrl toggles English/Chinese mode
  if (input.sym == keys::Control_R) {
    state->englishMode = !state->englishMode;
    state->lastCommit.clear();
    if (state->englishMode &&
        (!state->code.empty() || state->literalMode || state->tempPinyinMode || state->slashMode)) {
      state->code.clear();
      state->literalMode = false;
      state->literalModeAuto = false;
      state->literalBuffer.clear();
      state->tempPinyinMode = false;
      state->pinyinCode.clear();
      state->slashMode = false;
      state->slashBuffer.clear();
      out->clearPanel();
    }
    out->updateStatus();
    return true;
  }

  // In English mode: pass everything through
  if (state->englishMode) return false;

  // Backtick mid-composition: toggle rare character mode
  if (input.sym == keys::Grave && input.isUnmodified() && !state->code.empty()) {
    state->rareMode = !state->rareMode;
    state->pageOffset = 0;
    updateUI(state, out);
    return true;
  }

  // --- Temp pinyin mode ---
  if (state->tempPinyinMode) {
    // Escape: cancel and exit
    if (input.sym == keys::Escape && input.isUnmodified()) {
      exitTempPinyin(state, out);
      return true;
    }

    // Backspace: delete last typed letter
    if (input.sym == keys::BackSpace && input.isUnmodified()) {
      if (!state->pinyinCode.empty()) {
        state->pinyinCode.pop_back();
        state->pageOffset = 0;
        updatePinyinUI(state, out);
      } else {
        exitTempPinyin(state, out);
      }
      return true;
    }

    // Space: commit first candidate and exit
    if (input.sym == keys::Space && input.isUnmodified()) {
      auto candidates = pinyin_.lookup(state->pinyinCode);
      if (!candidates.empty()) {
        out->commit(candidates[0].text);
      }
      exitTempPinyin(state, out);
      return true;
    }

    // Enter: commit raw pinyin code as literal text
    if (input.sym == keys::Return && input.isUnmodified()) {
      out->commit(state->pinyinCode);
      exitTempPinyin(state, out);
      return true;
    }

    // Number 1-9,0: select candidate and exit
    {
      int selectIdx = -1;
      if (input.sym >= keys::_1 && input.sym <= keys::_9) {
        selectIdx = input.sym - keys::_1;
      } else if (input.sym == keys::_0) {
        selectIdx = 9;
      }
      if (selectIdx >= 0 && selectIdx < pageSize_) {
        int idx = state->pageOffset * pageSize_ + selectIdx;
        auto candidates = pinyin_.lookup(state->pinyinCode);
        std::vector<PinyinEntry> exact;
        for (auto& e : candidates) {
          if (e.stripped == state->pinyinCode) exact.push_back(e);
        }
        if (idx < static_cast<int>(exact.size())) {
          out->commit(exact[idx].text);
        }
        exitTempPinyin(state, out);
        return true;
      }
    }

    // -: previous page
    if (input.sym == keys::Minus && input.isUnmodified()) {
      if (state->pageOffset > 0) {
        --state->pageOffset;
        updatePinyinUI(state, out);
      }
      return true;
    }

    // =: next page
    if (input.sym == keys::Equal && input.isUnmodified()) {
      auto candidates = pinyin_.lookup(state->pinyinCode);
      int exactCount = 0;
      for (auto& e : candidates) {
        if (e.stripped == state->pinyinCode) ++exactCount;
      }
      int maxPage = (exactCount - 1) / pageSize_;
      if (state->pageOffset < maxPage) {
        ++state->pageOffset;
        updatePinyinUI(state, out);
      }
      return true;
    }

    // Letter key: append to pinyin code
    if (input.isUnmodified() && input.lowercase()) {
      state->pinyinCode += input.lowercase();
      state->pageOffset = 0;
      updatePinyinUI(state, out);
      return true;
    }

    // Any other key: exit temp pinyin mode and let it through
    exitTempPinyin(state, out);
    return false;
  }

  // --- Literal text mode ---
  if (state->literalMode) {
    // Escape: cancel
    if (input.sym == keys::Escape && input.isUnmodified()) {
      state->literalMode = false;
      state->literalModeAuto = false;
      state->literalBuffer.clear();
      out->clearPanel();
      return true;
    }

    // Backspace: delete last character
    if (input.sym == keys::BackSpace && input.isUnmodified()) {
      if (!state->literalBuffer.empty()) {
        state->literalBuffer.pop_back();
      }
      if (state->literalBuffer.empty()) {
        state->literalMode = false;
        state->literalModeAuto = false;
        out->clearPanel();
      } else {
        updateLiteralUI(state, out);
      }
      return true;
    }

    // Space
    if (input.sym == keys::Space && input.isUnmodified()) {
      if (state->literalModeAuto) {
        // Auto mode: commit buffer, no trailing space
        out->commit(state->literalBuffer);
        state->lastCommit.clear();
        state->literalMode = false;
        state->literalModeAuto = false;
        state->literalBuffer.clear();
        out->clearPanel();
      } else if (state->literalBuffer == std::string(1, static_cast<char>(tempEnglishKey_))) {
        // [ mode with just trigger char: commit it
        out->commit(std::string(1, static_cast<char>(tempEnglishKey_)));
        state->lastCommit.clear();
        state->literalMode = false;
        state->literalBuffer.clear();
        out->clearPanel();
      } else if (state->literalBuffer.size() < static_cast<size_t>(kMaxLiteralLen)) {
        // [ mode: space is a character
        state->literalBuffer += ' ';
        updateLiteralUI(state, out);
      }
      return true;
    }

    // Enter: commit buffer
    if (input.sym == keys::Return && input.isUnmodified()) {
      if (state->literalModeAuto) {
        out->commit(state->literalBuffer);
      } else if (state->literalBuffer == std::string(1, static_cast<char>(tempEnglishKey_))) {
        out->commit(std::string(1, static_cast<char>(tempEnglishKey_)));
      } else {
        out->commit(state->literalBuffer.substr(1));
      }
      state->lastCommit.clear();
      state->literalMode = false;
      state->literalModeAuto = false;
      state->literalBuffer.clear();
      out->clearPanel();
      return true;
    }

    // Printable characters: accumulate in buffer
    // (no Ctrl/Alt/Super — allow unmodified or Shift only)
    if (!input.hasAny(Modifiers::kCtrl | Modifiers::kAlt | Modifiers::kSuper) && input.sym >= keys::Exclam &&
        input.sym <= keys::Asciitilde) {
      if (state->literalBuffer.size() < static_cast<size_t>(kMaxLiteralLen)) {
        state->literalBuffer += input.asciiChar();
        updateLiteralUI(state, out);
      }
      return true;
    }

    // Tab or anything else: pass through to application
    return false;
  }

  // --- Slash mode ---
  if (state->slashMode) {
    // Escape: cancel
    if (input.sym == keys::Escape && input.isUnmodified()) {
      state->slashMode = false;
      state->slashBuffer.clear();
      out->clearPanel();
      return true;
    }

    // Backspace: delete last character
    if (input.sym == keys::BackSpace && input.isUnmodified()) {
      if (state->slashBuffer.size() > 1) {
        state->slashBuffer.pop_back();
        updateSlashUI(state, out);
      } else {
        state->slashMode = false;
        state->slashBuffer.clear();
        out->clearPanel();
      }
      return true;
    }

    // Space or Enter: commit entire buffer including /
    if ((input.sym == keys::Space || input.sym == keys::Return) && input.isUnmodified()) {
      out->commit(state->slashBuffer);
      state->lastCommit.clear();
      state->slashMode = false;
      state->slashBuffer.clear();
      out->clearPanel();
      return true;
    }

    // Printable characters: accumulate in buffer
    if (!input.hasAny(Modifiers::kCtrl | Modifiers::kAlt | Modifiers::kSuper) && input.sym >= keys::Exclam &&
        input.sym <= keys::Asciitilde) {
      if (state->slashBuffer.size() < static_cast<size_t>(kMaxLiteralLen)) {
        state->slashBuffer += input.asciiChar();
        updateSlashUI(state, out);
      }
      return true;
    }

    // Anything else: pass through
    return false;
  }

  // Ctrl / Alt / Meta shortcuts always pass through
  if (input.hasAny(Modifiers::kCtrl | Modifiers::kAlt | Modifiers::kSuper)) {
    return false;
  }

  // Capital letter (uppercase sym): enter auto literal mode
  if (input.uppercase()) {
    if (!state->code.empty()) {
      commitTopCandidate(state, out);
    }
    state->literalMode = true;
    state->literalModeAuto = true;
    state->literalBuffer = std::string(1, input.uppercase());
    updateLiteralUI(state, out);
    return true;
  }

  // Other Shift combinations: pass through
  if (input.hasAny(Modifiers::kShift)) {
    return false;
  }

  // [: enter literal text mode
  if (input.sym == tempEnglishKey_ && input.isUnmodified()) {
    if (!state->code.empty()) {
      commitTopCandidate(state, out);
    }
    state->literalMode = true;
    state->literalModeAuto = false;
    state->literalBuffer = std::string(1, static_cast<char>(tempEnglishKey_));
    updateLiteralUI(state, out);
    return true;
  }

  // ]: enter temp pinyin mode
  if (input.sym == tempPinyinKey_ && input.isUnmodified()) {
    if (!state->code.empty()) {
      commitTopCandidate(state, out);
    }
    state->tempPinyinMode = true;
    state->pinyinCode.clear();
    updatePinyinUI(state, out);
    return true;
  }

  // /: enter slash mode (commit whole buffer including /)
  if (input.sym == secondTempEnglishKey_ && input.isUnmodified()) {
    if (!state->code.empty()) {
      commitTopCandidate(state, out);
    }
    state->slashMode = true;
    state->slashBuffer = std::string(1, static_cast<char>(secondTempEnglishKey_));
    updateSlashUI(state, out);
    return true;
  }

  // Escape: cancel composition
  if (input.sym == keys::Escape && input.isUnmodified() && !state->code.empty()) {
    state->code.clear();
    state->rareMode = false;
    state->pageOffset = 0;
    out->clearPanel();
    return true;
  }

  // Backspace: delete last code letter
  if (input.sym == keys::BackSpace && input.isUnmodified() && !state->code.empty()) {
    state->code.pop_back();
    state->pageOffset = 0;
    updateUI(state, out);
    return true;
  }

  // Space: commit first candidate
  if (input.sym == keys::Space && input.isUnmodified() && !state->code.empty()) {
    if (state->code == "z" && !state->lastCommit.empty()) {
      out->commit(state->lastCommit);
    } else {
      auto dictCandidates = activeDict(state).promptCandidates(state->code);
      auto allCandidates = mergeCustomPhrases(state->code, dictCandidates);
      if (!allCandidates.empty()) {
        out->commit(allCandidates[0].text);
        state->lastCommit = allCandidates[0].text;
      }
    }
    state->code.clear();
    state->rareMode = false;
    state->pageOffset = 0;
    out->clearPanel();
    return true;
  }

  // Enter: commit raw code as literal text
  if (input.sym == keys::Return && input.isUnmodified() && !state->code.empty()) {
    out->commit(state->code);
    state->code.clear();
    state->rareMode = false;
    state->pageOffset = 0;
    out->clearPanel();
    return true;
  }

  // -: previous page
  if (input.sym == keys::Minus && input.isUnmodified() && !state->code.empty()) {
    if (state->pageOffset > 0) {
      --state->pageOffset;
      updateUI(state, out);
    }
    return true;
  }

  // =: next page
  if (input.sym == keys::Equal && input.isUnmodified() && !state->code.empty()) {
    auto candidates = activeDict(state).promptCandidates(state->code);
    int maxPage = (static_cast<int>(candidates.size()) - 1) / pageSize_;
    if (state->pageOffset < maxPage) {
      ++state->pageOffset;
      updateUI(state, out);
    }
    return true;
  }

  // Number key 1-9,0: select candidate by index
  {
    int selectIdx = -1;
    if (input.sym >= keys::_1 && input.sym <= keys::_9) {
      selectIdx = input.sym - keys::_1;
    } else if (input.sym == keys::_0) {
      selectIdx = 9;
    }
    if (selectIdx >= 0 && selectIdx < pageSize_ && !state->code.empty()) {
      int idx = state->pageOffset * pageSize_ + selectIdx;
      if (state->code == "z" && !state->lastCommit.empty() && idx == 0) {
        out->commit(state->lastCommit);
        state->code.clear();
        state->rareMode = false;
        state->pageOffset = 0;
        out->clearPanel();
      } else {
        auto dictCandidates = activeDict(state).promptCandidates(state->code);
        auto allCandidates = mergeCustomPhrases(state->code, dictCandidates);
        if (idx < static_cast<int>(allCandidates.size())) {
          out->commit(allCandidates[idx].text);
          state->lastCommit = allCandidates[idx].text;
          state->code.clear();
          state->rareMode = false;
          state->pageOffset = 0;
          out->clearPanel();
        }
      }
      return true;
    }
    // Not consumed as a candidate selector: the digit will pass through as
    // literal text, so remember it for the decimal-point check below.
    if (selectIdx >= 0 && state->code.empty()) {
      state->lastCharWasDigit = true;
    }
  }

  // Period right after a digit: keep it as an ASCII decimal point (e.g. "1.5")
  // instead of the Chinese full stop, even while composing Chinese text.
  if (input.sym == keys::Period && prevCharWasDigit) {
    return false;
  }

  // Punctuation keys
  if (input.sym == keys::Comma || input.sym == keys::Period || input.sym == keys::Exclam ||
      input.sym == keys::Question || input.sym == keys::Semicolon || input.sym == keys::Colon ||
      input.sym == keys::Less || input.sym == keys::Greater || input.sym == keys::Underscore ||
      input.sym == keys::Backslash || input.sym == keys::Asciicircum) {
    std::string ascii(1, input.asciiChar());
    auto punct = chinesePunct(state, ascii);
    if (!punct.empty()) {
      if (!state->code.empty()) {
        commitTopCandidate(state, out, punct);
      } else {
        out->commit(punct);
      }
      return true;
    }
    return false;
  }

  // Quote keys (double and single)
  if (input.sym == keys::Quotedbl || input.sym == keys::Apostrophe) {
    char ch = (input.sym == keys::Quotedbl) ? '"' : '\'';
    std::string ascii(1, ch);
    auto punct = chinesePunct(state, ascii);
    if (!punct.empty()) {
      if (!state->code.empty()) {
        commitTopCandidate(state, out, punct);
      } else {
        out->commit(punct);
      }
      return true;
    }
    return false;
  }

  // z (when idle): show last commit in candidate list
  if (input.sym == keys::z && input.isUnmodified() && state->code.empty()) {
    if (!state->lastCommit.empty()) {
      state->code = "z";
      state->pageOffset = 0;
      updateUI(state, out);
      return true;
    }
    return false;
  }

  // Letter key: append to code
  if (input.isUnmodified() && input.lowercase()) {
    char ch = input.lowercase();

    // At max code length: auto-commit top candidate and restart
    if (static_cast<int>(state->code.size()) >= kMaxCodeLen) {
      commitTopCandidate(state, out);
    }

    state->code += ch;
    state->pageOffset = 0;

    // Auto-commit when max-length code resolves to exactly one candidate
    if (static_cast<int>(state->code.size()) == kMaxCodeLen) {
      auto dictCandidates = activeDict(state).lookup(state->code);
      auto it = customPhrases_.find(state->code);
      int customCount = (it != customPhrases_.end()) ? it->second.size() : 0;
      int totalCount = customCount + dictCandidates.size();
      if (totalCount == 1) {
        if (customCount == 1) {
          out->commit(it->second[0]);
          state->lastCommit = it->second[0];
        } else {
          out->commit(dictCandidates[0]);
          state->lastCommit = dictCandidates[0];
        }
        state->code.clear();
        state->rareMode = false;
        state->pageOffset = 0;
        out->clearPanel();
        return true;
      }
    }

    updateUI(state, out);
    return true;
  }

  // Unhandled key: pass through
  return false;
}

void EngineLogic::updateUI(EngineState* state, IEngineOutput* out) {
  out->clearPanel();

  if (state->code.empty()) {
    out->setPreedit("");
    return;
  }

  std::string preeditText = state->rareMode ? ("`" + state->code) : state->code;
  out->setPreedit(preeditText);

  // Repeat-last mode: show lastCommit as the only candidate
  if (state->code == "z" && !state->lastCommit.empty()) {
    std::vector<CandidateEntry> cands;
    cands.push_back({"1." + state->lastCommit, state->lastCommit});
    out->setCandidates(cands);
  } else {
    auto dictCandidates = activeDict(state).promptCandidates(state->code);
    auto allCandidates = mergeCustomPhrases(state->code, dictCandidates);
    if (!allCandidates.empty()) {
      int start = state->pageOffset * pageSize_;
      int count = std::min(pageSize_, static_cast<int>(allCandidates.size()) - start);
      if (start >= static_cast<int>(allCandidates.size())) {
        start = 0;
        count = std::min(pageSize_, static_cast<int>(allCandidates.size()));
        state->pageOffset = 0;
      }
      std::vector<CandidateEntry> cands;
      for (int i = 0; i < count; i++) {
        cands.push_back({allCandidates[start + i].text, std::to_string(i + 1) + "." + allCandidates[start + i].label});
      }
      out->setCandidates(cands);
    }
  }
}

void EngineLogic::commitTopCandidate(EngineState* state, IEngineOutput* out, const std::string& suffix) {
  // Check custom phrases first (exact match)
  auto it = customPhrases_.find(state->code);
  if (it != customPhrases_.end() && !it->second.empty()) {
    out->commit(it->second[0] + suffix);
    state->lastCommit = it->second[0];
  } else {
    auto candidates = activeDict(state).lookup(state->code);
    if (!candidates.empty()) {
      out->commit(candidates[0] + suffix);
      state->lastCommit = candidates[0];
    } else if (!suffix.empty()) {
      out->commit(suffix);
    }
  }
  state->code.clear();
  state->rareMode = false;
  state->pageOffset = 0;
  out->clearPanel();
}

void EngineLogic::updatePinyinUI(EngineState* state, IEngineOutput* out) {
  out->clearPanel();

  std::string preedit = std::string(1, static_cast<char>(tempPinyinKey_)) + state->pinyinCode;
  out->setPreedit(preedit);

  if (!state->pinyinCode.empty()) {
    auto entries = pinyin_.lookup(state->pinyinCode);
    std::vector<const PinyinEntry*> exact;
    for (auto& e : entries) {
      if (e.stripped == state->pinyinCode) exact.push_back(&e);
    }
    if (!exact.empty()) {
      int start = state->pageOffset * pageSize_;
      int count = std::min(pageSize_, static_cast<int>(exact.size()) - start);
      if (start >= static_cast<int>(exact.size())) {
        start = 0;
        count = std::min(pageSize_, static_cast<int>(exact.size()));
        state->pageOffset = 0;
      }
      std::vector<CandidateEntry> cands;
      for (int i = 0; i < count; i++) {
        auto& entry = *exact[start + i];
        std::string wubi = dict_.codeForText(entry.text);
        std::string label = std::to_string(i + 1) + "." + entry.text;
        if (!wubi.empty()) label += " [" + wubi + "]";
        cands.push_back({entry.text, label});
      }
      out->setCandidates(cands);
    }
  }
}

void EngineLogic::updateLiteralUI(EngineState* state, IEngineOutput* out) {
  out->clearPanel();
  out->setPreedit(state->literalBuffer);
}

void EngineLogic::updateSlashUI(EngineState* state, IEngineOutput* out) {
  out->clearPanel();
  out->setPreedit(state->slashBuffer);
}

void EngineLogic::exitTempPinyin(EngineState* state, IEngineOutput* out) {
  state->tempPinyinMode = false;
  state->pinyinCode.clear();
  state->pageOffset = 0;
  out->clearPanel();
}

std::string EngineLogic::chinesePunct(EngineState* state, const std::string& ascii) {
  auto it = kPunctMap.find(ascii);
  if (it != kPunctMap.end()) {
    return it->second;
  }

  if (ascii == "\"") {
    std::string result = state->quoteOpenDouble ? "\xe2\x80\x9c"   // "
                                                : "\xe2\x80\x9d";  // "
    state->quoteOpenDouble = !state->quoteOpenDouble;
    return result;
  }
  if (ascii == "'") {
    std::string result = state->quoteOpenSingle ? "\xe2\x80\x98"   // '
                                                : "\xe2\x80\x99";  // '
    state->quoteOpenSingle = !state->quoteOpenSingle;
    return result;
  }

  return {};
}

WubiDict& EngineLogic::activeDict(EngineState* state) {
  if (!state->rareMode && dict_common_.numCodes() > 0) return dict_common_;
  return dict_;
}

void EngineLogic::setCustomPhrases(const std::vector<std::pair<std::string, std::string>>& phrases) {
  customPhrases_.clear();
  for (auto& [code, phrase] : phrases) {
    if (!code.empty() && !phrase.empty()) {
      customPhrases_[code].push_back(phrase);
    }
  }
}

std::vector<CandidateEntry> EngineLogic::mergeCustomPhrases(const std::string& code,
                                                            std::vector<PromptCandidate>& candidates) const {
  std::vector<CandidateEntry> result;

  // Prepend custom phrases that match the exact code
  auto it = customPhrases_.find(code);
  if (it != customPhrases_.end()) {
    for (auto& phrase : it->second) {
      result.push_back({phrase, phrase});
    }
  }

  // Append dictionary candidates
  for (auto& c : candidates) {
    result.push_back({c.text, c.label});
  }

  return result;
}

// --- UTF-8 helpers (file-local) ---

// Return the byte length of the UTF-8 character starting at `s[pos]`.
static size_t utf8CharLen(const std::string& s, size_t pos) {
  unsigned char c = static_cast<unsigned char>(s[pos]);
  if (c < 0x80) return 1;
  if (c < 0xE0) return 2;
  if (c < 0xF0) return 3;
  return 4;
}

// Split a UTF-8 string into individual character strings.
static std::vector<std::string> splitUtf8(const std::string& s) {
  std::vector<std::string> chars;
  size_t i = 0;
  while (i < s.size()) {
    size_t len = utf8CharLen(s, i);
    if (i + len > s.size()) break;  // malformed tail
    chars.push_back(s.substr(i, len));
    i += len;
  }
  return chars;
}

// Take up to `n` chars from the beginning of `code`.
static std::string takeFirst(const std::string& code, size_t n) { return code.substr(0, std::min(n, code.size())); }

std::string EngineLogic::computePhraseCode(const std::string& phrase) const {
  auto chars = splitUtf8(phrase);
  if (chars.empty()) return {};

  // Get each character's root code
  std::vector<std::string> codes;
  for (auto& ch : chars) {
    std::string code = dict_.codeForText(ch);
    if (code.empty()) return {};  // unknown character
    codes.push_back(std::move(code));
  }

  size_t n = codes.size();
  std::string result;

  if (n == 1) {
    // Single character: use its code directly
    result = codes[0];
  } else if (n == 2) {
    // First 2 keys of char 1 + first 2 keys of char 2
    result = takeFirst(codes[0], 2) + takeFirst(codes[1], 2);
  } else if (n == 3) {
    // First key of char 1 + first key of char 2 + first 2 keys of char 3
    result = takeFirst(codes[0], 1) + takeFirst(codes[1], 1) + takeFirst(codes[2], 2);
  } else {
    // 4+ chars: first key of char 1, 2, 3, and last
    result = takeFirst(codes[0], 1) + takeFirst(codes[1], 1) + takeFirst(codes[2], 1) + takeFirst(codes[n - 1], 1);
  }

  return result;
}
