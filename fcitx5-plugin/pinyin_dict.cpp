#include "pinyin_dict.h"

#include <algorithm>
#include <fstream>

bool PinyinDict::loadFromFile(const std::string &path) {
    std::ifstream fin(path);
    if (!fin.is_open()) return false;

    std::string line;
    while (std::getline(fin, line)) {
        auto tab1 = line.find('\t');
        if (tab1 == std::string::npos) continue;
        auto tab2 = line.find('\t', tab1 + 1);
        if (tab2 == std::string::npos) continue;

        std::string text = line.substr(0, tab1);
        std::string rawPinyin = line.substr(tab1 + 1, tab2 - tab1 - 1);
        int freq = std::stoi(line.substr(tab2 + 1));

        // Strip non-letter characters to get lookup key
        std::string stripped;
        stripped.reserve(rawPinyin.size());
        for (char c : rawPinyin) {
            if (c >= 'a' && c <= 'z') stripped += c;
        }

        entries_.push_back({std::move(text), std::move(stripped), freq});
    }

    std::sort(entries_.begin(), entries_.end(),
              [](const PinyinEntry &a, const PinyinEntry &b) {
                  return a.stripped < b.stripped;
              });

    return !entries_.empty();
}

std::vector<PinyinEntry> PinyinDict::lookup(const std::string &prefix) const {
    auto cmp = [](const PinyinEntry &e, const std::string &s) {
        return e.stripped < s;
    };
    auto lo = std::lower_bound(entries_.begin(), entries_.end(), prefix, cmp);

    std::vector<PinyinEntry> result;
    for (auto it = lo; it != entries_.end() &&
                       it->stripped.compare(0, prefix.size(), prefix) == 0;
         ++it) {
        result.push_back(*it);
    }

    std::sort(result.begin(), result.end(),
              [](const PinyinEntry &a, const PinyinEntry &b) {
                  return a.freq < b.freq;
              });

    return result;
}
