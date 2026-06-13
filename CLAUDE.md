# CLAUDE.md

Instructions for AI coding assistants working in this repository.
See [docs/development.md](docs/development.md) for architecture, build, code style, and contributing.

## Project Overview

FreeWubi is a Wubi 86 input method engine for Fcitx5 on Linux. Users type Chinese characters via Wubi codes (a-z), with temporary modes for pinyin lookup and literal English text.

## Engine Logic (engine_logic.cpp)

The `EngineLogic::processKey()` method is a large state machine. Key processing order matters — early checks take priority:
1. Right Ctrl toggles English mode
2. Active sub-modes (temp pinyin, literal, slash) handle their own keys first
3. Mode trigger keys (configurable: `[`, `]`, `/`, `;`)
4. Punctuation keys (Chinese punctuation mapping)
5. Letter keys → Wubi code input

Configurable trigger keys (`tempEnglishKey_`, `secondTempEnglishKey_`, `tempPinyinKey_`) are `uint32_t` keysyms set via setters. The literal/slash buffer prefixes and preedit display use the configured character.

### Configuration (freewubi_config.h)

Fcitx5 config defined via `FCITX_CONFIGURATION` macro. Uses `FCITX_CONFIG_ENUM` / `FCITX_CONFIG_ENUM_NAME` for dropdown enums (trigger key selection), `IntConstrain` for ranges, `ListDisplayOptionAnnotation` for list UI, and `ToolTipAnnotation` for tooltips.

Config sync: `syncConfigToEngine()` in freewubi.cpp reads config values and pushes them to the engine. Called from both `setConfig()` (UI save) and `reloadConfig()` (file change). The engine never reads config directly.

### Testing (tests/test_engine.cpp)

Engine logic is fully testable without Fcitx5 via the `IEngineOutput` interface. `TestOutput` records all calls (commit, setPreedit, clearPanel, setCandidates, updateStatus). `EngineFixture` loads real dictionaries from `data/`.

## Conventions

- C++20
- Configurable keysyms are stored as `uint32_t` in engine, convertible to char via `static_cast<char>(sym)` for buffer prefixes
- Custom phrases are stored as `code → [phrase, ...]` mapping; `computePhraseCode()` auto-deduces wubi code from phrase text
- All text is UTF-8 `std::string`; dictionary lookup returns UTF-8 characters/phrases

### Memory safety

No raw `new`/`delete`. Use `std::make_unique`/`std::make_shared` for ownership.
Raw pointers are acceptable only for non-owning observation (e.g., Fcitx5 API types like
`fcitx::Instance*`). Prefer references over raw pointers when null is not a valid value.

### Naming — two deliberate deviations from Google style

**Do not "fix" these:**
- **Functions/methods are camelCase** (`processKey`), not Google's CapWords (`ProcessKey`).
- **`keys::` keysym constants stay CapWords** (`keys::Control_L`), not `k`+CapWords, because they mirror the X11 keysym names they map to (`XK_Control_L`).

Full naming table and clang-format rules are in [docs/development.md](docs/development.md#naming).
