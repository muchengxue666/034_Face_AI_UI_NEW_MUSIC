from __future__ import annotations

from orangepi_rag_service.models.memory import MemoryHit


def _clip_text(text: str, limit: int) -> str:
    cleaned = text.strip()
    if len(cleaned) <= limit:
        return cleaned
    return cleaned[: max(limit - 1, 0)].rstrip() + "…"


def _build_summary(hits: list[MemoryHit], max_summary_chars: int) -> str:
    summary_lines: list[str] = []
    used = 0
    for index, hit in enumerate(hits, start=1):
        line = f"{index}. [{hit.record.category}] {hit.record.title}：{_clip_text(hit.record.content, 180)}"
        extra = len(line) + (1 if summary_lines else 0)
        if used + extra > max_summary_chars:
            break
        summary_lines.append(line)
        used += extra
    return "\n".join(summary_lines)


def compose_system_prompt(
    query: str,
    intent: str,
    hits: list[MemoryHit],
    max_prompt_chars: int,
    max_summary_chars: int,
) -> tuple[str, bool]:
    base = (
        "你是陪伴老人的温暖智能助手小柚子。"
        "请用简洁、亲切、口语化的中文回答，不超过60字，不使用Markdown。"
        "只有在检索到明确家庭记忆时才能引用家庭事实；如果没有命中或信息不确定，必须直接说明暂时不知道，并建议联系家属确认。"
    )
    guardrail = f"当前用户问题意图：{intent}。问题：{query.strip()}"

    if not hits:
        prompt = (
            f"{base}\n\n{guardrail}\n\n"
            "当前没有命中的家庭记忆。"
            "不要编造任何关于家人、药物、病史、作息、联系方式、重要日期的具体事实。"
        )
        return _clip_text(prompt, max_prompt_chars), False

    summary = _build_summary(hits, max_summary_chars)
    prompt = (
        f"{base}\n\n{guardrail}\n\n"
        "以下是命中的家庭记忆，只能基于这些记忆回答：\n"
        f"{summary}\n\n"
        "如果这些记忆仍不足以支持明确回答，就明确说不知道，不要自行补全。"
    )
    return _clip_text(prompt, max_prompt_chars), True
