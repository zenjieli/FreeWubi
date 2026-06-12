#pragma once

#include <cstdint>
#include <string>
#include <vector>

// --- Modifier flags (bitmask, same bit positions as fcitx::KeyState) ---
enum class Modifiers : uint32_t {
  kNone = 0,
  kShift = 1 << 0,
  kCtrl = 1 << 2,
  kAlt = 1 << 3,
  kSuper = 1 << 6,
};

inline Modifiers operator|(Modifiers a, Modifiers b) {
  return static_cast<Modifiers>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline Modifiers operator&(Modifiers a, Modifiers b) {
  return static_cast<Modifiers>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline bool hasAny(Modifiers value, Modifiers flag) {
  return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
}
inline bool operator==(Modifiers a, Modifiers b) { return static_cast<uint32_t>(a) == static_cast<uint32_t>(b); }

// --- Key input (portable, no Fcitx5 types) ---
struct KeyInput {
  uint32_t sym;
  Modifiers modifiers;

  KeyInput() : sym(0), modifiers(Modifiers::kNone) {}
  KeyInput(uint32_t s, Modifiers m = Modifiers::kNone) : sym(s), modifiers(m) {}

  // Printable ASCII character, or 0
  char asciiChar() const { return (sym >= 0x21 && sym <= 0x7E) ? static_cast<char>(sym) : 0; }
  // Lowercase letter a-z, or 0
  char lowercase() const { return (sym >= 0x61 && sym <= 0x7A) ? static_cast<char>(sym) : 0; }
  // Uppercase letter A-Z, or 0
  char uppercase() const { return (sym >= 0x41 && sym <= 0x5A) ? static_cast<char>(sym) : 0; }
  bool isUnmodified() const { return modifiers == Modifiers::kNone; }
  bool hasAny(Modifiers m) const { return ::hasAny(modifiers, m); }
};

// Well-known keysym constants (X11 keysym values, matching FcitxKey_*)
namespace keys {
constexpr uint32_t Space = 0x0020;
constexpr uint32_t Exclam = 0x0021;
constexpr uint32_t Quotedbl = 0x0022;
constexpr uint32_t Apostrophe = 0x0027;
constexpr uint32_t _0 = 0x0030;
constexpr uint32_t _1 = 0x0031;
constexpr uint32_t _5 = 0x0035;
constexpr uint32_t _6 = 0x0036;
constexpr uint32_t _9 = 0x0039;
constexpr uint32_t Colon = 0x003a;
constexpr uint32_t Semicolon = 0x003b;
constexpr uint32_t Less = 0x003c;
constexpr uint32_t Equal = 0x003d;
constexpr uint32_t Greater = 0x003e;
constexpr uint32_t Question = 0x003f;
constexpr uint32_t A = 0x0041;
constexpr uint32_t Z = 0x005a;
constexpr uint32_t BracketLeft = 0x005b;
constexpr uint32_t Backslash = 0x005c;
constexpr uint32_t BracketRight = 0x005d;
constexpr uint32_t Asciicircum = 0x005e;
constexpr uint32_t Underscore = 0x005f;
constexpr uint32_t Grave = 0x0060;
constexpr uint32_t a = 0x0061;
constexpr uint32_t z = 0x007a;
constexpr uint32_t BraceLeft = 0x007b;
constexpr uint32_t Asciitilde = 0x007e;
constexpr uint32_t Comma = 0x002c;
constexpr uint32_t Minus = 0x002d;
constexpr uint32_t Period = 0x002e;
constexpr uint32_t Slash = 0x002f;
constexpr uint32_t BackSpace = 0xff08;
constexpr uint32_t Tab = 0xff09;
constexpr uint32_t Return = 0xff0d;
constexpr uint32_t Escape = 0xff1b;
constexpr uint32_t Control_L = 0xffe3;
constexpr uint32_t Control_R = 0xffe4;
}  // namespace keys

// --- Engine state (plain struct, no fcitx base class) ---
struct EngineState {
  std::string code;
  bool literalMode = false;
  bool literalModeAuto = false;  // true = triggered by capital letter
  std::string literalBuffer;
  bool quoteOpenDouble = true;
  bool quoteOpenSingle = true;
  bool englishMode = false;
  bool tempPinyinMode = false;
  std::string pinyinCode;
  bool slashMode = false;
  std::string slashBuffer;
  bool rareMode = false;
  std::string lastCommit;  // last text committed via normal wubi input
  int pageOffset = 0;      // candidate list page offset
};

// --- Candidate data passed to the output layer ---
struct CandidateEntry {
  std::string text;   // the text to commit
  std::string label;  // display label (may include code hints)
};

// --- Output interface (implemented by Fcitx5 adapter and test double) ---
class IEngineOutput {
 public:
  virtual ~IEngineOutput() = default;
  virtual void commit(const std::string& text) = 0;
  virtual void setPreedit(const std::string& text) = 0;
  virtual void clearPanel() = 0;
  virtual void setCandidates(const std::vector<CandidateEntry>& candidates) = 0;
  virtual void updateStatus() = 0;
};

// --- Constants ---
static constexpr int kMaxCodeLen = 4;
static constexpr int kDefaultPageSize = 5;
static constexpr int kMaxPageSize = 10;
static constexpr int kMaxLiteralLen = 100;
