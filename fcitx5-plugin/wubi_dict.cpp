#include "wubi_dict.h"

#include <algorithm>
#include <climits>
#include <fstream>
#include <sstream>
#include <tuple>
#include <unordered_set>

bool WubiDict::loadFromFile(const std::string& path) {
  std::ifstream fin(path);
  if (!fin.is_open()) return false;

  bool inBody = false;
  std::string line;
  std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> tmp;

  while (std::getline(fin, line)) {
    if (!inBody) {
      if (line.find("...") == 0) inBody = true;
      continue;
    }
    if (line.empty() || line[0] == '#') continue;

    auto tab1 = line.find('\t');
    if (tab1 == std::string::npos) continue;
    auto tab2 = line.find('\t', tab1 + 1);

    std::string text = line.substr(0, tab1);
    std::string code = line.substr(tab1 + 1, tab2 == std::string::npos ? std::string::npos : tab2 - tab1 - 1);

    int weight = 0;
    if (tab2 != std::string::npos) {
      auto tab3 = line.find('\t', tab2 + 1);
      std::string ws = line.substr(tab2 + 1, tab3 == std::string::npos ? std::string::npos : tab3 - tab2 - 1);
      try {
        weight = std::stoi(ws);
      } catch (...) {
      }
    }

    for (auto& c : code) c = static_cast<char>(std::tolower(c));
    tmp[code].emplace_back(text, weight);
  }

  dict_.clear();
  reverse_.clear();
  for (auto& [code, entries] : tmp) {
    std::stable_sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    auto& vec = dict_[code];
    vec.reserve(entries.size());
    for (auto& e : entries) {
      vec.emplace_back(e.first, e.second);
      if (!reverse_.count(e.first)) reverse_[e.first] = code;
    }
  }

  return true;
}

std::vector<std::string> WubiDict::lookup(const std::string& code) const {
  auto it = dict_.find(code);
  if (it == dict_.end()) return {};
  std::vector<std::string> result;
  result.reserve(it->second.size());
  for (auto& p : it->second) result.push_back(p.first);
  return result;
}

bool WubiDict::hasCode(const std::string& code) const { return dict_.count(code) > 0; }

std::string WubiDict::codeForText(const std::string& text) const {
  auto it = reverse_.find(text);
  return it != reverse_.end() ? it->second : std::string{};
}

bool WubiDict::loadFrequency(const std::string& path) {
  std::ifstream fin(path);
  if (!fin.is_open()) return false;

  std::string line;
  while (std::getline(fin, line)) {
    auto tab = line.find('\t');
    if (tab == std::string::npos) continue;
    std::string text = line.substr(0, tab);
    int rank = std::stoi(line.substr(tab + 1));
    auto it = freq_.find(text);
    if (it == freq_.end() || rank < it->second) freq_[text] = rank;
  }
  return true;
}

int WubiDict::globalFreq(const std::string& text) const {
  auto it = freq_.find(text);
  return it != freq_.end() ? it->second : INT_MAX;
}

std::vector<PromptCandidate> WubiDict::promptCandidates(const std::string& partialCode) const {
  static constexpr size_t kMaxPrefix = 5;
  std::unordered_set<std::string> seen;

  // Exact matches: all entries (no cap — pagination handles display)
  std::vector<PromptCandidate> result;

  auto it = dict_.find(partialCode);
  if (it != dict_.end()) {
    for (auto& entry : it->second) {
      if (seen.insert(entry.first).second) result.push_back({entry.first, entry.first});
    }
  }

  // Prefix matches: up to kMaxPrefix, sorted by frequency then weight
  std::vector<std::tuple<int, int, std::string, std::string>> prefix;
  for (char c = 'a'; c <= 'y'; c++) {
    std::string code = partialCode + c;
    auto it2 = dict_.find(code);
    if (it2 == dict_.end()) continue;
    for (auto& entry : it2->second) {
      if (!seen.insert(entry.first).second) continue;
      prefix.emplace_back(globalFreq(entry.first), -entry.second, entry.first, code);
    }
    if (prefix.size() >= kMaxPrefix + 10) break;
  }

  std::sort(prefix.begin(), prefix.end());

  size_t prefixAdded = 0;
  for (auto& e : prefix) {
    if (prefixAdded >= kMaxPrefix) break;
    auto& text = std::get<2>(e);
    auto& fullCode = std::get<3>(e);
    std::string label = text + fullCode.substr(partialCode.size());
    result.push_back({std::move(text), std::move(label)});
    ++prefixAdded;
  }

  return result;
}
