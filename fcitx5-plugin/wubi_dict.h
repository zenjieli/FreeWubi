#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct PromptCandidate {
  std::string text;   // commit text
  std::string label;  // display label (text + remaining code hint)
};

class WubiDict {
 public:
  bool loadFromFile(const std::string& path);
  bool loadFrequency(const std::string& path);
  std::vector<std::string> lookup(const std::string& code) const;
  std::vector<PromptCandidate> promptCandidates(const std::string& partialCode) const;
  bool hasCode(const std::string& code) const;
  // Returns first wubi code for a given text, or empty string
  std::string codeForText(const std::string& text) const;
  size_t numCodes() const { return dict_.size(); }

 private:
  int globalFreq(const std::string& text) const;

  std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> dict_;
  std::unordered_map<std::string, int> freq_;
  std::unordered_map<std::string, std::string> reverse_;  // text → first code
};
