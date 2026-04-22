from __future__ import annotations

import re


_SPACE_RE = re.compile(r"\s+")
_ALNUM_RE = re.compile(r"[a-z0-9]+")
_CJK_BLOCK_RE = re.compile(r"[\u4e00-\u9fff]+")


def normalize_text(text: str) -> str:
    compact = _SPACE_RE.sub(" ", (text or "").strip().lower())
    return compact


def _bigrams(chunk: str) -> list[str]:
    if len(chunk) < 2:
        return [chunk]
    result = [chunk]
    for index in range(len(chunk) - 1):
        result.append(chunk[index : index + 2])
    return result


def extract_terms(text: str) -> list[str]:
    normalized = normalize_text(text)
    terms: list[str] = []
    seen: set[str] = set()

    for token in _ALNUM_RE.findall(normalized):
        if token not in seen:
            seen.add(token)
            terms.append(token)

    for chunk in _CJK_BLOCK_RE.findall(normalized):
        for token in _bigrams(chunk):
            if token not in seen:
                seen.add(token)
                terms.append(token)

    return terms


def build_fts_document(*parts: str) -> str:
    tokens: list[str] = []
    seen: set[str] = set()
    for part in parts:
        for token in extract_terms(part):
            if token not in seen:
                seen.add(token)
                tokens.append(token)
    return " ".join(tokens)


def build_fts_query(text: str) -> str:
    terms = extract_terms(text)
    if not terms:
        return ""
    return " OR ".join(f'"{term}"' for term in terms)
