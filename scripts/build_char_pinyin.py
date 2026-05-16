"""
Trim CC-CEDICT down to single-character entries only.

Input:  cedict_ts.u8  (full CC-CEDICT)
Output: char_pinyin.tsv  (tab-separated: traditional, simplified, pinyins)

Each output line has exactly one traditional char, one simplified char, and all
known pinyins for that (traditional, simplified) pair joined by spaces.

Example:
    中\t中\tZhong1 zhong1 zhong4
"""

import re
import sys
from collections import defaultdict
from pathlib import Path

ENTRY_RE = re.compile(r"^(\S+) (\S+) \[([^\]]+)\]")


def is_single_cjk(s: str) -> bool:
    """Return True if s is exactly one CJK / CJK-extension Unicode character."""
    if len(s) != 1:
        return False
    cp = ord(s)
    return (
        0x4E00 <= cp <= 0x9FFF    # CJK Unified Ideographs
        or 0x3400 <= cp <= 0x4DBF  # CJK Extension A
        or 0x20000 <= cp <= 0x2A6DF  # CJK Extension B
        or 0x2A700 <= cp <= 0x2CEAF  # CJK Extensions C-F
        or 0xF900 <= cp <= 0xFAFF    # CJK Compatibility Ideographs
        or 0x2F800 <= cp <= 0x2FA1F  # CJK Compatibility Supplement
    )


def build(input_path: Path, output_path: Path) -> None:
    # (traditional, simplified) -> ordered list of pinyins (deduplicated)
    pinyin_map: dict[tuple[str, str], list[str]] = defaultdict(list)

    with input_path.open(encoding="utf-8") as f:
        for line in f:
            if line.startswith("#"):
                continue
            m = ENTRY_RE.match(line)
            if not m:
                continue
            trad, simp, pinyin_raw = m.group(1), m.group(2), m.group(3)
            if not (is_single_cjk(trad) and is_single_cjk(simp)):
                continue
            key = (trad, simp)
            pinyin_lower = pinyin_raw.lower()
            if pinyin_lower not in pinyin_map[key]:
                pinyin_map[key].append(pinyin_lower)

    with output_path.open("w", encoding="utf-8") as f:
        for (trad, simp), pinyins in sorted(pinyin_map.items()):
            f.write(f"{trad}\t{simp}\t{' '.join(pinyins)}\n")

    print(f"Written {len(pinyin_map):,} entries → {output_path}", file=sys.stderr)


if __name__ == "__main__":
    repo_root = Path(__file__).parent.parent
    data_dir = repo_root / "src" / "freewubi" / "data"
    build(
        input_path=data_dir / "cedict_ts.u8",
        output_path=data_dir / "char_pinyin.tsv",
    )
