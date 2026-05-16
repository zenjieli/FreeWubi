#include <catch2/catch_test_macros.hpp>

#include "engine_logic.h"
#include "engine_types.h"
#include "pinyin_dict.h"
#include "wubi_dict.h"

#include <string>

// --- Test output: records all calls ---
struct TestOutput : public IEngineOutput {
    std::vector<std::string> commits;
    std::vector<std::string> preedits;
    int clearPanelCount = 0;
    std::vector<std::vector<CandidateEntry>> candidateSets;
    int statusUpdateCount = 0;

    void commit(const std::string &text) override { commits.push_back(text); }
    void setPreedit(const std::string &text) override {
        preedits.push_back(text);
    }
    void clearPanel() override { ++clearPanelCount; }
    void setCandidates(
        const std::vector<CandidateEntry> &c) override {
        candidateSets.push_back(c);
    }
    void updateStatus() override { ++statusUpdateCount; }

    void reset() {
        commits.clear();
        preedits.clear();
        clearPanelCount = 0;
        candidateSets.clear();
        statusUpdateCount = 0;
    }
};

// --- Shared test data ---
static const std::string kDataDir = TEST_DATA_DIR;

struct DictHolder {
    WubiDict dict;
    WubiDict dict_common;
    PinyinDict pinyin;

    DictHolder() {
        dict.loadFromFile(kDataDir + "/wubi86_jidian.dict.yaml");
        dict_common.loadFromFile(
            kDataDir + "/wubi86_jidian_common.dict.yaml");
        dict.loadFrequency(kDataDir + "/frequency.txt");
        dict_common.loadFrequency(kDataDir + "/frequency.txt");
        pinyin.loadFromFile(kDataDir + "/pinyin.txt");
    }
};

// --- Fixture: all members directly accessible in TEST_CASE_METHOD ---
struct EngineFixture {
    DictHolder dicts;
    EngineLogic logic;
    EngineState state;
    TestOutput output;

    EngineFixture()
        : logic(dicts.dict, dicts.dict_common, dicts.pinyin) {}

    bool feed(uint32_t sym, Modifiers mods = Modifiers::None) {
        return logic.processKey(KeyInput(sym, mods), &state, &output);
    }

    void type(const std::string &letters) {
        for (char c : letters)
            feed(static_cast<uint32_t>(c));
    }

    void resetOutput() { output.reset(); }
};

// =====================================================================
// Tests
// =====================================================================

TEST_CASE_METHOD(EngineFixture, "Basic wubi: type 'a' then space commits top candidate") {
    bool consumed = feed(Keys::a);
    REQUIRE(consumed);
    REQUIRE(state.code == "a");

    resetOutput();
    consumed = feed(Keys::Space);
    REQUIRE(consumed);
    REQUIRE(state.code.empty());
    REQUIRE_FALSE(output.commits.empty());
    // 'a' maps to 工 as top candidate in common dict
    REQUIRE(output.commits.back() == "\xe5\xb7\xa5"); // 工
}

TEST_CASE_METHOD(EngineFixture, "Auto-commit at 4-key unique code") {
    // aaad has exactly one candidate in common dict
    feed('a');
    feed('a');
    feed('a');
    REQUIRE(state.code == "aaa");

    resetOutput();
    bool consumed = feed('d');
    REQUIRE(consumed);
    REQUIRE(state.code.empty());
    REQUIRE_FALSE(output.commits.empty());
    REQUIRE(output.commits.back() == "\xe5\xb7\xa5\xe6\x9c\x9f"); // 工期
}

TEST_CASE_METHOD(EngineFixture, "Slash mode: /rewind<space> commits '/rewind'") {
    feed(Keys::Slash);
    REQUIRE(state.slashMode);
    REQUIRE(state.slashBuffer == "/");

    type("rewind");
    REQUIRE(state.slashBuffer == "/rewind");

    resetOutput();
    bool consumed = feed(Keys::Space);
    REQUIRE(consumed);
    REQUIRE_FALSE(state.slashMode);
    REQUIRE(state.slashBuffer.empty());
    REQUIRE(output.commits.size() == 1);
    REQUIRE(output.commits[0] == "/rewind");
}

TEST_CASE_METHOD(EngineFixture, "Slash mode: bare /<space> commits '/'") {
    feed(Keys::Slash);
    resetOutput();

    bool consumed = feed(Keys::Space);
    REQUIRE(consumed);
    REQUIRE(output.commits.size() == 1);
    REQUIRE(output.commits[0] == "/");
}

TEST_CASE_METHOD(EngineFixture, "Slash mode: Escape cancels") {
    feed(Keys::Slash);
    type("abc");
    resetOutput();

    bool consumed = feed(Keys::Escape);
    REQUIRE(consumed);
    REQUIRE_FALSE(state.slashMode);
    REQUIRE(state.slashBuffer.empty());
    REQUIRE(output.commits.empty());
}

TEST_CASE_METHOD(EngineFixture, "Slash mode: Enter commits") {
    feed(Keys::Slash);
    type("hello");
    resetOutput();

    bool consumed = feed(Keys::Return);
    REQUIRE(consumed);
    REQUIRE(output.commits.size() == 1);
    REQUIRE(output.commits[0] == "/hello");
}

TEST_CASE_METHOD(EngineFixture, "Slash mode: Backspace deletes, cancels at /") {
    feed(Keys::Slash);
    type("ab");
    REQUIRE(state.slashBuffer == "/ab");

    feed(Keys::BackSpace);
    REQUIRE(state.slashBuffer == "/a");

    feed(Keys::BackSpace);
    REQUIRE(state.slashBuffer == "/");
    REQUIRE(state.slashMode);

    feed(Keys::BackSpace);
    REQUIRE_FALSE(state.slashMode);
    REQUIRE(state.slashBuffer.empty());
}

TEST_CASE_METHOD(EngineFixture, "Slash mode: numbers are literals") {
    feed(Keys::Slash);
    type("v2");
    resetOutput();

    feed(Keys::Space);
    REQUIRE(output.commits[0] == "/v2");
}

TEST_CASE_METHOD(EngineFixture, "Z-key repeat: re-commits last wubi output") {
    // First commit something via wubi
    feed(Keys::a);
    feed(Keys::Space);
    REQUIRE_FALSE(output.commits.empty());
    std::string committed = output.commits.back();
    REQUIRE_FALSE(committed.empty());

    resetOutput();
    // Press z when idle → repeat-last mode
    bool consumed = feed(Keys::z);
    REQUIRE(consumed);
    REQUIRE(state.code == "z");

    resetOutput();
    consumed = feed(Keys::Space);
    REQUIRE(consumed);
    REQUIRE(output.commits.size() == 1);
    REQUIRE(output.commits[0] == committed);
}

TEST_CASE_METHOD(EngineFixture, "Z-key: no lastCommit passes z through") {
    REQUIRE(state.lastCommit.empty());
    bool consumed = feed(Keys::z);
    REQUIRE_FALSE(consumed);
}

TEST_CASE_METHOD(EngineFixture, "Literal auto-mode: capital H + i + space") {
    feed(0x48); // 'H' uppercase sym
    REQUIRE(state.literalMode);
    REQUIRE(state.literalModeAuto);
    REQUIRE(state.literalBuffer == "H");

    type("i");
    REQUIRE(state.literalBuffer == "Hi");

    resetOutput();
    bool consumed = feed(Keys::Space);
    REQUIRE(consumed);
    REQUIRE_FALSE(state.literalMode);
    REQUIRE(output.commits.size() == 1);
    REQUIRE(output.commits[0] == "Hi");
}

TEST_CASE_METHOD(EngineFixture, "Literal bracket mode: bracket-hello-Enter strips bracket") {
    feed(Keys::BracketLeft);
    REQUIRE(state.literalMode);
    REQUIRE(state.literalBuffer == "[");

    type("hello");
    resetOutput();

    feed(Keys::Return);
    REQUIRE_FALSE(state.literalMode);
    REQUIRE(output.commits.size() == 1);
    REQUIRE(output.commits[0] == "hello");
}

TEST_CASE_METHOD(EngineFixture, "Literal bracket mode: bracket-space commits bracket") {
    feed(Keys::BracketLeft);
    resetOutput();

    feed(Keys::Space);
    REQUIRE(output.commits.size() == 1);
    REQUIRE(output.commits[0] == "[");
}

TEST_CASE_METHOD(EngineFixture, "Literal bracket mode: space inserts space char") {
    feed(Keys::BracketLeft);
    type("hello");
    feed(Keys::Space); // space is a character in [ mode
    type("world");
    resetOutput();

    feed(Keys::Return);
    REQUIRE(output.commits[0] == "hello world");
}

TEST_CASE_METHOD(EngineFixture, "English mode toggle") {
    feed(Keys::Control_R);
    REQUIRE(state.englishMode);
    REQUIRE(output.statusUpdateCount > 0);

    resetOutput();
    bool consumed = feed(Keys::a);
    REQUIRE_FALSE(consumed);

    feed(Keys::Control_R);
    REQUIRE_FALSE(state.englishMode);
}

TEST_CASE_METHOD(EngineFixture, "Chinese punctuation: comma") {
    bool consumed = feed(Keys::Comma);
    REQUIRE(consumed);
    REQUIRE(output.commits.size() == 1);
    REQUIRE(output.commits[0] == "\xef\xbc\x8c"); // ，
}

TEST_CASE_METHOD(EngineFixture, "Chinese punctuation: period") {
    feed(Keys::Period);
    REQUIRE(output.commits.back() == "\xe3\x80\x82"); // 。
}

TEST_CASE_METHOD(EngineFixture, "Smart quotes: double-quote alternates") {
    feed(Keys::Quotedbl);
    REQUIRE(output.commits.back() == "\xe2\x80\x9c"); // "

    feed(Keys::Quotedbl);
    REQUIRE(output.commits.back() == "\xe2\x80\x9d"); // "
}

TEST_CASE_METHOD(EngineFixture, "Smart quotes: single-quote alternates") {
    feed(Keys::Apostrophe);
    REQUIRE(output.commits.back() == "\xe2\x80\x98"); // '

    feed(Keys::Apostrophe);
    REQUIRE(output.commits.back() == "\xe2\x80\x99"); // '
}

TEST_CASE_METHOD(EngineFixture, "Backspace during composition") {
    feed(Keys::a);
    feed('b');
    REQUIRE(state.code == "ab");

    feed(Keys::BackSpace);
    REQUIRE(state.code == "a");

    feed(Keys::BackSpace);
    REQUIRE(state.code.empty());
}

TEST_CASE_METHOD(EngineFixture, "Escape cancels composition") {
    feed(Keys::a);
    feed('b');
    REQUIRE_FALSE(state.code.empty());

    bool consumed = feed(Keys::Escape);
    REQUIRE(consumed);
    REQUIRE(state.code.empty());
    REQUIRE_FALSE(state.rareMode);
}

TEST_CASE_METHOD(EngineFixture, "Enter commits raw code as literal") {
    feed(Keys::a);
    feed('b');
    feed('x');
    resetOutput();

    feed(Keys::Return);
    REQUIRE(output.commits.size() == 1);
    REQUIRE(output.commits[0] == "abx");
}

TEST_CASE_METHOD(EngineFixture, "Punctuation mid-composition commits candidate with punct") {
    feed(Keys::a);
    resetOutput();

    feed(Keys::Comma);
    // commitTopCandidate appends punctuation suffix to the candidate
    REQUIRE(output.commits.size() == 1);
    REQUIRE_FALSE(output.commits[0].empty());
    // The committed text should end with Chinese comma
    REQUIRE(output.commits[0].substr(output.commits[0].size() - 3) == "\xef\xbc\x8c"); // ，
}

TEST_CASE_METHOD(EngineFixture, "Rare mode toggle mid-composition") {
    feed(Keys::a);
    REQUIRE_FALSE(state.rareMode);

    feed(Keys::Grave);
    REQUIRE(state.rareMode);

    feed(Keys::Grave);
    REQUIRE_FALSE(state.rareMode);
}

TEST_CASE_METHOD(EngineFixture, "Grave when idle passes through") {
    bool consumed = feed(Keys::Grave);
    REQUIRE_FALSE(consumed);
}

TEST_CASE_METHOD(EngineFixture, "Number keys select candidates") {
    feed(Keys::a);
    REQUIRE_FALSE(state.code.empty());

    resetOutput();
    bool consumed = feed(Keys::_1);
    REQUIRE(consumed);
    REQUIRE(state.code.empty());
    REQUIRE_FALSE(output.commits.empty());
}

TEST_CASE_METHOD(EngineFixture, "Right Ctrl clears active modes") {
    feed(Keys::Slash);
    REQUIRE(state.slashMode);

    feed(Keys::Control_R);
    REQUIRE(state.englishMode);
    REQUIRE_FALSE(state.slashMode);
}

TEST_CASE_METHOD(EngineFixture, "Temp pinyin: basic lookup") {
    feed(Keys::BracketRight);
    REQUIRE(state.tempPinyinMode);

    type("ni");
    REQUIRE(state.pinyinCode == "ni");

    resetOutput();
    feed(Keys::Space);
    REQUIRE_FALSE(state.tempPinyinMode);
    REQUIRE_FALSE(output.commits.empty());
}

TEST_CASE_METHOD(EngineFixture, "Temp pinyin: Escape cancels") {
    feed(Keys::BracketRight);
    type("ni");
    resetOutput();

    feed(Keys::Escape);
    REQUIRE_FALSE(state.tempPinyinMode);
    REQUIRE(output.commits.empty());
}

TEST_CASE_METHOD(EngineFixture, "Temp pinyin: Enter commits raw pinyin") {
    feed(Keys::BracketRight);
    type("ni");
    resetOutput();

    feed(Keys::Return);
    REQUIRE_FALSE(state.tempPinyinMode);
    REQUIRE(output.commits.size() == 1);
    REQUIRE(output.commits[0] == "ni");
}

TEST_CASE_METHOD(EngineFixture, "Slash mode entered mid-composition") {
    feed(Keys::a);
    REQUIRE_FALSE(state.code.empty());

    resetOutput();
    feed(Keys::Slash);
    REQUIRE(state.slashMode);
    REQUIRE(state.slashBuffer == "/");
    REQUIRE_FALSE(output.commits.empty());
}

// =====================================================================
// Configurable page size tests
// =====================================================================

TEST_CASE_METHOD(EngineFixture, "Default page size is 5") {
    REQUIRE(logic.pageSize() == 5);
}

TEST_CASE_METHOD(EngineFixture, "Custom page size limits candidates shown") {
    logic.setPageSize(3);
    feed(Keys::a);
    REQUIRE_FALSE(state.code.empty());
    REQUIRE_FALSE(output.candidateSets.empty());

    // Should show at most 3 candidates
    auto &cands = output.candidateSets.back();
    REQUIRE(cands.size() <= 3);
}

TEST_CASE_METHOD(EngineFixture, "Key 6 not a selection key at page size 5") {
    logic.setPageSize(5);
    feed(Keys::a);
    resetOutput();

    // Key 6 should NOT be consumed as selection (page size is 5)
    bool consumed = feed(Keys::_6);
    REQUIRE_FALSE(consumed);
}

TEST_CASE_METHOD(EngineFixture, "Key 6 selects candidate at page size 7") {
    logic.setPageSize(7);
    feed(Keys::a);
    REQUIRE_FALSE(output.candidateSets.empty());

    // There should be at least 6 candidates for code 'a'
    auto &cands = output.candidateSets.back();
    if (cands.size() >= 6) {
        resetOutput();
        bool consumed = feed(Keys::_6);
        REQUIRE(consumed);
        REQUIRE(state.code.empty());
        REQUIRE_FALSE(output.commits.empty());
    }
}

TEST_CASE_METHOD(EngineFixture, "Key 0 selects 10th candidate at page size 10") {
    logic.setPageSize(10);
    feed(Keys::a);
    REQUIRE_FALSE(output.candidateSets.empty());

    auto &cands = output.candidateSets.back();
    if (cands.size() >= 10) {
        resetOutput();
        bool consumed = feed(Keys::_0);
        REQUIRE(consumed);
        REQUIRE(state.code.empty());
        REQUIRE_FALSE(output.commits.empty());
    }
}

TEST_CASE_METHOD(EngineFixture, "Key 0 not a selection key at page size 5") {
    logic.setPageSize(5);
    feed(Keys::a);
    resetOutput();

    bool consumed = feed(Keys::_0);
    REQUIRE_FALSE(consumed);
}

TEST_CASE_METHOD(EngineFixture, "Page size 1 shows single candidate") {
    logic.setPageSize(1);
    feed(Keys::a);
    REQUIRE_FALSE(output.candidateSets.empty());

    auto &cands = output.candidateSets.back();
    REQUIRE(cands.size() == 1);
    REQUIRE(cands[0].label.substr(0, 2) == "1.");
}

// =====================================================================
// Custom phrase tests
// =====================================================================

TEST_CASE_METHOD(EngineFixture, "Custom phrase appears as first candidate") {
    logic.setCustomPhrases({{"ab", "hello"}});
    feed(Keys::a);
    feed('b');
    REQUIRE(state.code == "ab");
    REQUIRE_FALSE(output.candidateSets.empty());

    auto &cands = output.candidateSets.back();
    REQUIRE_FALSE(cands.empty());
    REQUIRE(cands[0].text == "hello");
}

TEST_CASE_METHOD(EngineFixture, "Custom phrase committed by space") {
    logic.setCustomPhrases({{"ab", "hello"}});
    feed(Keys::a);
    feed('b');
    resetOutput();

    feed(Keys::Space);
    REQUIRE(state.code.empty());
    REQUIRE_FALSE(output.commits.empty());
    REQUIRE(output.commits.back() == "hello");
}

TEST_CASE_METHOD(EngineFixture, "Custom phrase committed by number key") {
    logic.setCustomPhrases({{"ab", "hello"}});
    feed(Keys::a);
    feed('b');
    resetOutput();

    feed(Keys::_1);
    REQUIRE(state.code.empty());
    REQUIRE_FALSE(output.commits.empty());
    REQUIRE(output.commits.back() == "hello");
}

TEST_CASE_METHOD(EngineFixture, "Custom phrase prepended before dictionary candidates") {
    logic.setCustomPhrases({{"a", "custom"}});
    feed(Keys::a);
    REQUIRE_FALSE(output.candidateSets.empty());

    auto &cands = output.candidateSets.back();
    REQUIRE(cands[0].text == "custom");
    // Should still have dictionary candidates after it
    REQUIRE(cands.size() > 1);
}

TEST_CASE_METHOD(EngineFixture, "Multiple custom phrases for same code") {
    logic.setCustomPhrases({{"ab", "first"}, {"ab", "second"}});
    feed(Keys::a);
    feed('b');
    REQUIRE_FALSE(output.candidateSets.empty());

    auto &cands = output.candidateSets.back();
    REQUIRE(cands.size() >= 2);
    REQUIRE(cands[0].text == "first");
    REQUIRE(cands[1].text == "second");
}

TEST_CASE_METHOD(EngineFixture, "Custom phrase updated on setCustomPhrases") {
    logic.setCustomPhrases({{"ab", "old"}});
    feed(Keys::a);
    feed('b');
    resetOutput();
    feed(Keys::Space);
    REQUIRE(output.commits.back() == "old");

    // Update phrases
    logic.setCustomPhrases({{"ab", "new"}});
    feed(Keys::a);
    feed('b');
    resetOutput();
    feed(Keys::Space);
    REQUIRE(output.commits.back() == "new");
}

TEST_CASE_METHOD(EngineFixture, "Empty custom phrases do not interfere") {
    logic.setCustomPhrases({{"", "empty_code"}, {"ab", ""}});
    feed(Keys::a);
    feed('b');
    resetOutput();
    // Should behave like normal — no crash, no phantom candidates
    feed(Keys::Space);
    REQUIRE(state.code.empty());
    REQUIRE_FALSE(output.commits.empty());
}

TEST_CASE_METHOD(EngineFixture, "Custom phrase commitTopCandidate prefers custom") {
    logic.setCustomPhrases({{"a", "custom_top"}});
    feed(Keys::a);
    feed(Keys::Comma); // triggers commitTopCandidate with punctuation suffix
    REQUIRE_FALSE(output.commits.empty());
    REQUIRE(output.commits.back().substr(0, 10) == "custom_top");
}

// =====================================================================
// computePhraseCode tests
// =====================================================================

// Helper: get hex for a UTF-8 string
static std::string hex(const std::string &s) {
    std::string r;
    for (unsigned char c : s) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02x", c);
        r += buf;
    }
    return r;
}

TEST_CASE_METHOD(EngineFixture, "computePhraseCode: single char returns its code") {
    // Single character should return a valid (non-empty) code
    std::string code = logic.computePhraseCode("\xe5\xb7\xa5"); // 工
    REQUIRE_FALSE(code.empty());
}

TEST_CASE_METHOD(EngineFixture, "computePhraseCode: 2-char phrase") {
    // 中(code "k") + 国(code "l") → take(k,2)+take(l,2) = "kl"
    std::string code = logic.computePhraseCode("\xe4\xb8\xad\xe5\x9b\xbd"); // 中国
    REQUIRE(code == "kl");
}

TEST_CASE_METHOD(EngineFixture, "computePhraseCode: 2-char phrase with long codes") {
    // 工(code "aaaa") + 期(code "adwe") → take(aaaa,2)+take(adwe,2) = "aaad"
    std::string code =
        logic.computePhraseCode("\xe5\xb7\xa5\xe6\x9c\x9f"); // 工期
    REQUIRE(code == "aaad");
}

TEST_CASE_METHOD(EngineFixture, "computePhraseCode: 3-char phrase") {
    // 中("k") + 国("l") + 人("w") → k+l+take(w,2) = "klw"
    std::string code =
        logic.computePhraseCode("\xe4\xb8\xad\xe5\x9b\xbd\xe4\xba\xba"); // 中国人
    REQUIRE(code == "klw");
}

TEST_CASE_METHOD(EngineFixture, "computePhraseCode: 4-char phrase") {
    // 中("k") + 国("l") + 人("w") + 民("n") → k+l+w+n = "klwn"
    std::string code = logic.computePhraseCode(
        "\xe4\xb8\xad\xe5\x9b\xbd\xe4\xba\xba\xe6\xb0\x91"); // 中国人民
    REQUIRE(code == "klwn");
}

TEST_CASE_METHOD(EngineFixture, "computePhraseCode: 5-char phrase uses 1st,2nd,3rd,last") {
    // 中("k") + 国("l") + 人("w") + 民("n") + 大("dd")
    // → k + l + w + d = "klwd"
    std::string code = logic.computePhraseCode(
        "\xe4\xb8\xad\xe5\x9b\xbd\xe4\xba\xba\xe6\xb0\x91\xe5\xa4\xa7"); // 中国人民大
    REQUIRE(code == "klwd");
}

TEST_CASE_METHOD(EngineFixture, "computePhraseCode: unknown char returns empty") {
    // Use a char that's unlikely in the dictionary: 𐍈 (Gothic letter, 4-byte UTF-8)
    std::string code = logic.computePhraseCode("\xf0\x90\x8d\x88"); // 𐍈
    REQUIRE(code.empty());
}

TEST_CASE_METHOD(EngineFixture, "computePhraseCode: empty phrase returns empty") {
    std::string code = logic.computePhraseCode("");
    REQUIRE(code.empty());
}

TEST_CASE_METHOD(EngineFixture, "computePhraseCode: phrase with unknown char returns empty") {
    // 中 + unknown → empty
    std::string code =
        logic.computePhraseCode("\xe4\xb8\xad\xf0\x90\x8d\x88"); // 中𐍈
    REQUIRE(code.empty());
}
