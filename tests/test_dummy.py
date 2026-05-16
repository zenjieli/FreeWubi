import importlib.resources


def test_wubi_dict_is_present():
    with importlib.resources.open_binary("freewubi.data", "wubi86_jidian.dict.yaml") as f:
        content = f.read(100)
    assert b"wubi86_jidian" in content


def test_cedict_is_present():
    with importlib.resources.open_binary("freewubi.data", "char_pinyin.tsv") as f:
        first_bytes = f.read(256)
    # TSV starts with a CJK character (first traditional char entry)
    assert first_bytes

