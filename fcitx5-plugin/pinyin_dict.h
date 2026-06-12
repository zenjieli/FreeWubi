#pragma once

#include <string>
#include <vector>

struct PinyinEntry {
  std::string text;      // the character/phrase
  std::string stripped;  // pinyin without tones (letters only)
  int freq;
};

class PinyinDict {
 public:
  bool loadFromFile(const std::string& path);
  // Returns candidates whose stripped pinyin starts with `prefix`,
  // sorted by frequency (lower = more common).
  std::vector<PinyinEntry> lookup(const std::string& prefix) const;
  size_t numEntries() const { return entries_.size(); }

 private:
  // Sorted by stripped pinyin for binary search
  std::vector<PinyinEntry> entries_;
};
