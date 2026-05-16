## Project Plan: FreeWubi — Wubi 86 Input Method for Linux

**Goal**
A Wubi 86 input method engine for Fcitx5 on Linux.

---

### Fcitx5 Plugin

A native Fcitx5 engine plugin written in C++. Once installed, FreeWubi appears in the system input method list and works in any application — no floating bar or focus hacks needed.

**Milestones**

- [x] Dummy plugin — loads in Fcitx5, commits "你好" on keypress `a`
- [x] Real Wubi engine — dictionary lookup, candidate list, full key event handling
- [x] Chinese punctuation substitution
- [x] Temporary pinyin mode (toggle with `]`)
- [x] EN/Wubi toggle (Right Ctrl)
- [x] Z-key repeat last wubi commit
- [x] Candidate list pagination with `-` / `=` keys
- [x] Common-character mode (default) + rare-character mode (toggle with `` ` ``)
  - See **Rare Character Mode** section below for design details

**Tech**
- C++ shared library (`freewubi.so`) using `fcitx::InputMethodEngineV2`
- Wubi 86 dictionary
- User-level install to `~/.local/` (no sudo needed after dev deps are installed)

---

### Z-key: Repeat Last Wubi Commit

Pressing `z` when idle (no composition in progress) immediately re-commits the most recently typed Wubi character or phrase. This saves re-entering the same character repeatedly (e.g. typing 一一一).

**Behaviour**

| Situation | Result |
|-----------|--------|
| `z` pressed idle, previous wubi commit exists | Re-commits that text |
| `z` pressed idle, no previous wubi commit | Passes `z` through to the application |
| `z` pressed mid-composition | Appended to code buffer (no wubi root uses `z`, so candidate list is empty; Escape cancels) |

**What sets `lastCommit`**: Space, number-key, auto-commit (4-key single candidate), `commitTopCandidate` (triggered by punctuation / `[` / `]`).

**What clears `lastCommit`**: `reset()` (focus change), Right Ctrl (English mode toggle), committing from literal mode (`[`).  Punctuation and pinyin commits do *not* clear it.

---

### Candidate List Pagination

When a Wubi code matches more candidates than fit on one page, use `-` and `=` to flip pages.

| Key | Action |
|-----|--------|
| `-` | Previous page |
| `=` | Next page |

---

### Rare Character Mode

**Motivation**

The full Wubi dictionary contains many obscure single characters that share a code with common characters or phrases. In normal typing these rare characters are unreachable in practice (they are always displaced by more frequent entries). Hiding them by default reduces visual noise and candidate-list clutter.

**Definition — "rare character"**

A dictionary entry is marked rare if *all* of the following hold:

1. It is a single character (not a multi-character phrase).
2. The character is **not** present in `data/common_chars.md`.
3. Its Wubi code has at least one **multi-character phrase** entry anywhere in the full dictionary (common_chars membership of that phrase is irrelevant).

A single character that is the *only* entry for its code (no phrase competition) remains visible in default mode — the user can still reach it.

**Two modes**

| Mode | What is shown | How to enter |
|------|--------------|--------------|
| Common mode (default) | All entries except rare characters | Normal typing |
| Rare mode (temporary) | All entries including rare characters | Press `` ` `` while composing a Wubi code |

Pressing `` ` `` mid-composition toggles `state->rareMode` and refreshes the candidate list. It is **one-shot**: committing or cancelling the composition immediately resets rare mode. Backtick while *not* composing passes through as a literal backtick.

**`]` key — temporary pinyin** (replaces the old backtick binding)

Pressing `]` while not composing Wubi enters temporary pinyin mode (existing feature, key reassigned from `` ` `` to `]`).

**Pre-built common dictionary (offline generation)**

Rather than filtering rare characters at runtime, the common vocabulary is generated offline as `data/wubi86_jidian_common.dict.yaml`. This file is committed to the repo and loaded directly by the plugin, keeping the hot path simple.

Generation script: `scripts/build_common_dict.py`

```
python3 scripts/build_common_dict.py \
    --dict         fcitx5-plugin/data/wubi86_jidian.dict.yaml \
    --common-chars fcitx5-plugin/data/common_chars.md \
    --output       fcitx5-plugin/data/wubi86_jidian_common.dict.yaml
```

The plugin loads `wubi86_jidian_common.dict.yaml` by default. When `` ` `` is pressed mid-composition, it switches to the full `wubi86_jidian.dict.yaml` for that lookup only.

**Implementation steps**

- [x] Write `scripts/build_common_dict.py`
- [x] Generate and commit `data/wubi86_jidian_common.dict.yaml`
- [x] Plugin: load both `_common` and full dict at startup
- [x] Plugin: add `rareMode` flag to `FreeWubiState`
- [x] Plugin: backtick mid-composition toggles `rareMode`, refreshes UI
- [x] Plugin: `]` key enters temporary pinyin mode (renamed from backtick)
- [x] Plugin: `lookup()` / `promptCandidates()` route to common or full dict based on `rareMode`
