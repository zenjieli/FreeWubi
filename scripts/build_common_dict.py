#!/usr/bin/env python3
"""Generate wubi86_jidian_common.dict.yaml by removing rare characters.

A dictionary entry is marked rare if ALL of the following hold:
  1. It is a single Unicode character (not a multi-character phrase).
  2. The character is NOT present in common_chars.md.
  3. Its Wubi code collides with at least one other entry — specifically, at
     least one of:
       3.1. A multi-character phrase entry anywhere in the full dictionary
            (common_chars membership of that phrase is irrelevant).
       3.2. A character present in common_chars.md.

A single character that is the sole entry for its code (no collision) is kept
— it is still reachable in common mode because it displaces nothing.

Usage:
    python scripts/build_common_dict.py \\
        --dict         fcitx5-plugin/data/wubi86_jidian.dict.yaml \\
        --common-chars fcitx5-plugin/data/common_chars.md \\
        --output       fcitx5-plugin/data/wubi86_jidian_common.dict.yaml
"""

import argparse


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Build a common-character-only Wubi dictionary."
    )
    parser.add_argument("--dict", required=True, help="Input dict YAML file")
    parser.add_argument("--common-chars", required=True, help="common_chars.md file")
    parser.add_argument("--output", required=True, help="Output YAML file")
    args = parser.parse_args()

    # --- Load common character set ---
    # common_chars.md format: markdown headers (## ...) and lines of
    # concatenated characters. Blank lines and lines starting with '#' are
    # skipped; every other character on a data line is added to the set.
    common_set: set[str] = set()
    with open(args.common_chars, encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            for ch in line:
                common_set.add(ch)

    # --- Parse the dict YAML ---
    # Format: YAML header, then "..." separator, then tab-separated body.
    # Body line: text \t code \t weight [\t stem]
    header_lines: list[str] = []
    # Each body entry: (text, code, raw_line)
    body_entries: list[tuple[str, str, str]] = []

    in_body = False
    with open(args.dict, encoding="utf-8") as f:
        for raw in f:
            if not in_body:
                header_lines.append(raw)
                if raw.startswith("..."):
                    in_body = True
                continue
            stripped = raw.rstrip("\n")
            if not stripped or stripped.startswith("#"):
                body_entries.append(("", "", raw))
                continue
            parts = stripped.split("\t")
            text = parts[0]
            code = parts[1] if len(parts) > 1 else ""
            body_entries.append((text, code, raw))

    # --- Find codes that collide with a phrase or a common character ---
    # Condition 3.1: code has at least one multi-character phrase entry.
    # Condition 3.2: code has at least one character present in common_chars.md.
    code_has_phrase: set[str] = set()
    code_has_freq_char: set[str] = set()
    for text, code, _ in body_entries:
        if not code:
            continue
        if len(text) > 1:
            code_has_phrase.add(code)
        elif text in common_set:
            code_has_freq_char.add(code)

    # --- Identify rare (text, code) pairs ---
    # A pair is rare if the text is a single char, not in common_set, and its code
    # collides with a phrase (3.1) or a common character (3.2).
    rare_pairs: set[tuple[str, str]] = set()
    for text, code, _ in body_entries:
        if not code:
            continue
        if (
            len(text) == 1
            and text not in common_set
            and (code in code_has_phrase or code in code_has_freq_char)
        ):
            rare_pairs.add((text, code))

    # --- Write output, skipping rare entries ---
    with open(args.output, "w", encoding="utf-8") as out:
        for line in header_lines:
            out.write(line)
        for text, code, raw in body_entries:
            if (text, code) in rare_pairs:
                continue
            out.write(raw)

    total = sum(1 for t, c, _ in body_entries if c)
    removed = len(rare_pairs)
    print(f"Total entries : {total}")
    print(f"Rare removed  : {removed}")
    print(f"Remaining     : {total - removed}")
    print(f"Output        : {args.output}")


if __name__ == "__main__":
    main()
