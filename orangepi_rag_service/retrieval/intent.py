from __future__ import annotations

from dataclasses import dataclass

from orangepi_rag_service.retrieval.tokenizer import normalize_text


@dataclass(frozen=True)
class IntentDecision:
    intent: str
    categories: tuple[str, ...]


_INTENT_RULES = [
    (
        "medication",
        ("medication", "health", "safety"),
        ("吃药", "服药", "药", "药片", "降压", "降糖", "阿司匹林", "二甲双胍", "硝苯地平"),
    ),
    (
        "routine",
        ("routine", "preference", "diet"),
        ("几点", "作息", "起床", "午睡", "晚上", "早饭", "早餐", "晚饭", "睡前", "平时"),
    ),
    (
        "contact",
        ("contact", "family"),
        ("电话", "联系人", "联系", "号码", "医生", "医院", "打给", "谁的电话"),
    ),
    (
        "family",
        ("family", "profile", "event"),
        ("女儿", "儿子", "孙子", "孙女", "家人", "老伴", "奶奶", "小明", "美华"),
    ),
    (
        "health",
        ("health", "medication", "safety"),
        ("血压", "膝盖", "头晕", "白内障", "心律", "贫血", "感冒", "身体", "生病"),
    ),
    (
        "diet",
        ("diet", "health", "routine"),
        ("吃", "喝", "饮食", "忌口", "西柚", "小米粥", "晚餐", "午餐", "早餐"),
    ),
    (
        "event",
        ("event", "family"),
        ("生日", "纪念日", "最近", "近期", "上周", "今年", "重阳节"),
    ),
    (
        "safety",
        ("safety", "health", "contact"),
        ("危险", "注意", "提醒", "扶手", "救护车", "应急", "头晕", "超过"),
    ),
]

_MEMORY_QUERY_HINTS = (
    "奶奶",
    "家人",
    "家里",
    "女儿",
    "儿子",
    "孙子",
    "孙女",
    "老伴",
    "王医生",
    "医生",
    "医院",
    "电话",
    "联系人",
    "吃药",
    "药",
    "血压",
    "头晕",
    "身体",
    "早餐",
    "午睡",
    "晚饭",
    "作息",
    "生日",
    "纪念日",
    "忌口",
    "喜欢",
    "爱吃",
    "留言",
    "来看",
    "回家",
)


def classify_intent(query: str) -> IntentDecision:
    normalized = normalize_text(query)
    best_rule: IntentDecision | None = None
    best_score = 0

    for intent, categories, keywords in _INTENT_RULES:
        score = sum(1 for keyword in keywords if keyword in normalized)
        if score > best_score:
            best_score = score
            best_rule = IntentDecision(intent=intent, categories=categories)

    if best_rule:
        return best_rule
    return IntentDecision(intent="generic", categories=("profile", "family", "health", "diet", "routine", "preference", "event", "contact", "safety", "note", "medication"))


def should_use_memory(query: str, decision: IntentDecision) -> bool:
    if decision.intent != "generic":
        return True
    normalized = normalize_text(query)
    return any(keyword in normalized for keyword in _MEMORY_QUERY_HINTS)
