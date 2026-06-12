# Usage Guide

## Wubi Input

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

## Temp Pinyin Mode

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

## Literal Text Mode

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

## Slash Mode

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

Examples: `/` + `Space` → `/` · `/rewind` + `Space` → `/rewind`

## English/Chinese Toggle

Press **Right Ctrl** to toggle between Chinese and English mode. The fcitx5
taskbar icon shows "En" when in English mode. In English mode, all keys pass
through normally.

## Chinese Punctuation

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
(`"`` ""` and `'`'`), alternating between opening and closing.
