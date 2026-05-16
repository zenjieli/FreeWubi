# FreeWubi

A Wubi 86 input method engine for Fcitx5 on Linux.

## Usage

### Wubi input

Type wubi 86 codes (a-z). Up to 5 candidates are shown per page with
frequency-based ordering and prefix matching. Candidates display remaining
code letters as hints (e.g. `3.式aa` means the full code is `aa` + shown
suffix).

| Key | Action |
|---|---|
| `a`-`z` | Append letter to wubi code |
| `1`-`5` | Select candidate by number |
| `Space` | Commit first candidate |
| `Enter` | Commit raw code as literal text |
| `Esc` | Cancel and clear code input |
| `Backspace` | Delete last code letter |
| `=` / `-` | Next / previous candidate page |
| `` ` `` | Toggle rare character mode (mid-composition) |
| `z` | Repeat last wubi commit (when idle) |

Auto-commit happens when a 4-letter code resolves to exactly one candidate.

### Temp pinyin mode

Press `]` (right bracket) to enter temp pinyin mode. Type pinyin (no tones) to
look up characters/phrases. Each candidate shows its wubi code in brackets
(e.g. `1.中 [khk]`). After selecting a candidate, the mode auto-exits back to
wubi input.

| Key | Action |
|---|---|
| `]` | Enter temp pinyin mode |
| `a`-`z` | Append letter to pinyin input |
| `1`-`5` | Select candidate and exit |
| `Space` | Commit first candidate and exit |
| `Enter` | Commit raw pinyin as literal text and exit |
| `Esc` | Cancel and exit |
| `=` / `-` | Next / previous candidate page |

### Literal text mode

Press `[` (left bracket) to enter literal text mode. Everything typed is
collected as-is (no Wubi interpretation). Press `Enter` to commit the text
(the leading `[` is stripped).

| Key | Action |
|---|---|
| `[` | Enter literal text mode |
| Printable chars | Append to buffer |
| `Space` | Append space to buffer |
| `Enter` | Commit text (without leading `[`) |
| `Esc` | Cancel |

### Slash mode

Press `/` to enter slash mode. Type a short English string; press `Space` or
`Enter` to commit it. The leading `/` is included in the output. Useful for
typing commands or paths without switching to English mode.

| Key | Action |
|---|---|
| `/` | Enter slash mode |
| Printable chars | Append to buffer |
| `Space` | Commit entire buffer (including `/`) |
| `Enter` | Commit entire buffer (including `/`) |
| `Backspace` | Delete last char (cancel if only `/` remains) |
| `Esc` | Cancel |

Examples: `/` + `Space` → `/` &nbsp;·&nbsp; `/rewind` + `Space` → `/rewind`

### English/Chinese toggle

Press **Right Ctrl** to toggle between Chinese and English mode. The fcitx5
taskbar icon shows "En" when in English mode. In English mode, all keys pass
through normally.

### Chinese punctuation

In Chinese mode, the following punctuation keys produce Chinese equivalents:

| Key | Output | Key | Output |
|---|---|---|---|
| `,` | ， | `.` | 。 |
| `!` | ！ | `?` | ？ |
| `;` | ； | `:` | ： |
| `^` | …… | `\` | 、 |
| `<` | 《 | `>` | 》 |
| `_` | —— | | |

Double-quote (`"`) and single-quote (`'`) produce smart Chinese quotes
(`"“"”` and `'‘'’`), alternating between opening and closing.

---

## Repo structure

```
fcitx5-plugin/
  freewubi.cpp / freewubi.h   # Fcitx5 engine: key handling, UI, modes
  wubi_dict.cpp / wubi_dict.h # Wubi dictionary loader and lookup
  pinyin_dict.cpp / pinyin_dict.h # Pinyin dictionary loader and lookup
  data/
    wubi86_jidian.dict.yaml   # Wubi 86 code table (traditional chars with
                              #   duplicate codes removed; see .original)
    wubi86_jidian.dict.original.yaml  # Full original table, before cleanup
    wubi86_jidian_common.dict.yaml    # Common chars only (generated, see
                              #   scripts/build_common_dict.py)
    common_chars.md           # Common character list (levels 1+2+3)
    frequency.txt             # Global frequency rankings
    pinyin.txt                # Pinyin-to-character mappings
  inputmethod/
    freewubi.conf.in             # Input method entry config
  freewubi-addon.conf.in         # Fcitx5 addon registration
  CMakeLists.txt              # Build configuration
scripts/
  build_char_pinyin.py        # Utility script for building pinyin data
  build_common_dict.py        # Generates common-char-only dictionary
tests/
  test_engine.py              # Engine tests
```

### pinyin.txt format

Tab-separated file with three columns: `text<tab>pinyin<tab>frequency`.

- **text**: the character or phrase (e.g. `啊`, `阿爸`)
- **pinyin**: pinyin with tone numbers for multi-syllable entries (e.g. `a1'ba4`),
  plain ASCII for single-char supplement entries (e.g. `bi`). Tone numbers and
  apostrophes are stripped at load time; only letters are used for lookup.
- **frequency**: lower = more common. Single-char supplement entries use 57000
  (level 1), 58000 (level 2), 59000 (level 3).

Multiple pronunciations for the same character appear as separate lines.

---

## Installation

### Install dev dependencies (one-time)

```bash
sudo apt install -y libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev \
  extra-cmake-modules build-essential cmake
```

### Install Fcitx5 (if not already installed)

```bash
sudo apt install -y fcitx5 fcitx5-chinese-addons \
  fcitx5-frontend-gtk3 fcitx5-frontend-gtk4 \
  fcitx5-frontend-qt5 fcitx5-config-qt im-config

im-config -n fcitx5  # Set fcitx5 as default input method framework
```

Log out and back in for the IM framework change to take effect.

### Build and install the plugin

```bash
cd fcitx5-plugin
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local

make
make install
```

This installs to user-local dirs (no sudo):
- `~/.local/lib/fcitx5/freewubi.so`
- `~/.local/share/fcitx5/addon/freewubi.conf`
- `~/.local/share/fcitx5/inputmethod/freewubi.conf`
- `~/.local/share/fcitx5/data/` (dictionary and pinyin data)

### Register the addon library path

Fcitx5 does not search `~/.local/lib/fcitx5/` by default. Set `FCITX_ADDON_DIRS` so it finds the plugin.

If `~/.xinputrc` already exists (created by `im-config -n fcitx5`), add the export before the `run_im` line:

```bash
# ~/.xinputrc
export FCITX_ADDON_DIRS=/usr/lib/x86_64-linux-gnu/fcitx5:$HOME/.local/lib/fcitx5
run_im fcitx5
```

If `~/.xinputrc` does not exist, create it:

```bash
cat > ~/.xinputrc << 'EOF'
export FCITX_ADDON_DIRS=/usr/lib/x86_64-linux-gnu/fcitx5:$HOME/.local/lib/fcitx5
run_im fcitx5
EOF
```

Log out and back in for the change to take effect.

### Verify

1. Run `fcitx5-configtool` -- FreeWubi should appear in the available input methods
2. Add **FreeWubi** to your active input methods
3. Open any text editor, switch to FreeWubi (Ctrl+Space)
4. Press `a` -- should show candidates like `1.工 2.戈`
5. Press `1` to commit `工`, or `Space` to commit the top candidate

---

## Debugging

For plugin debugging in VS Code, see `.vscode/launch.json` for the launch
configuration.

---

## Running tests

### C++ engine tests (Catch2)

```bash
cd fcitx5-plugin
cmake -B build -DBUILD_TESTING=ON
cmake --build build
cd build && ctest --output-on-failure
```

### Python dictionary tests

```bash
uv run pytest
```
