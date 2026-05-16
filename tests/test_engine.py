"""Unit tests for WubiEngine."""

import pytest

from freewubi.engine import WubiEngine


@pytest.fixture(scope="module")
def engine() -> WubiEngine:
    return WubiEngine()


class TestLookup:
    def test_returns_list(self, engine: WubiEngine):
        result = engine.lookup("llll")
        assert isinstance(result, list)

    def test_unknown_code_returns_empty(self, engine: WubiEngine):
        assert engine.lookup("zzzz") == []

    def test_empty_code_returns_empty(self, engine: WubiEngine):
        assert engine.lookup("") == []

    def test_llll_first_candidate_is_tian(self, engine: WubiEngine):
        # 田 (tián) has weight 40 — highest for code 'llll'
        assert engine.lookup("llll")[0] == "田"

    def test_llll_candidates_ordered_by_weight(self, engine: WubiEngine):
        # Expected order by weight: 田(40) > 团团圆圆(30) > 〇(20) > 轠(10)
        result = engine.lookup("llll")
        assert result == ["田", "团团圆圆", "〇", "轠"]

    def test_aaaa_top_candidate(self, engine: WubiEngine):
        # 工 has weight 20, 恭恭敬敬 has weight 10
        result = engine.lookup("aaaa")
        assert result[0] == "工"
        assert "恭恭敬敬" in result

    def test_gggg_top_candidate_is_wang(self, engine: WubiEngine):
        assert engine.lookup("gggg")[0] == "王"

    def test_single_key_code(self, engine: WubiEngine):
        # 'a' maps to 工(20) and 戈(10)
        result = engine.lookup("a")
        assert result[0] == "工"
        assert result[1] == "戈"

    def test_phrase_in_candidates(self, engine: WubiEngine):
        # Phrases are included alongside single characters
        result = engine.lookup("llll")
        assert "团团圆圆" in result


class TestHasCode:
    def test_known_code_returns_true(self, engine: WubiEngine):
        assert engine.has_code("llll") is True
        assert engine.has_code("gggg") is True
        assert engine.has_code("a") is True

    def test_unknown_code_returns_false(self, engine: WubiEngine):
        assert engine.has_code("zzzz") is False
        assert engine.has_code("") is False


class TestLoadIntegrity:
    def test_table_is_non_empty(self, engine: WubiEngine):
        # Dictionary has ~89k entries across many codes
        assert len(engine._table) > 1000

    def test_all_codes_are_lowercase_alpha(self, engine: WubiEngine):
        for code in engine._table:
            assert code.isalpha() and code == code.lower(), (
                f"Unexpected code format: {repr(code)}"
            )

    def test_all_candidate_lists_are_non_empty(self, engine: WubiEngine):
        for code, entries in engine._table.items():
            assert len(entries) > 0, f"Empty entry list for code {repr(code)}"
