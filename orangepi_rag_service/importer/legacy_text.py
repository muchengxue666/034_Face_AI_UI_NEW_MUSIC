from __future__ import annotations

import re
from pathlib import Path

from orangepi_rag_service.models.memory import MemoryRecord


_SECTION_RE = re.compile(r"^==\s*(.+?)\s*==$")

_SECTION_MAPPING = {
    "基本信息": ("profile", "基本信息", ["姓名", "年龄", "住址", "性格"]),
    "家庭成员": ("family", "家庭成员", ["家人", "儿子", "女儿", "孙子", "孙女"]),
    "健康状况": ("health", "健康状况", ["高血压", "糖尿病", "膝盖", "白内障", "心脏"]),
    "用药记录": ("medication", "用药记录", ["吃药", "用药", "硝苯地平", "二甲双胍", "阿司匹林"]),
    "饮食习惯": ("diet", "饮食习惯", ["早餐", "午餐", "晚餐", "忌口"]),
    "作息时间": ("routine", "作息时间", ["起床", "午睡", "晚饭", "睡前", "就寝"]),
    "兴趣爱好": ("preference", "兴趣爱好", ["京剧", "聊天", "麻将", "养花"]),
    "近期状况": ("event", "近期状况", ["近期", "复查", "体检", "感冒"]),
    "重要日期": ("event", "重要日期", ["生日", "纪念日", "重阳节"]),
    "紧急联系": ("contact", "紧急联系", ["联系人", "家庭医生", "医院"]),
    "特别提醒": ("safety", "特别提醒", ["提醒", "头晕", "救护车", "观察"]),
}


def parse_legacy_text(path: Path) -> list[MemoryRecord]:
    if not path.exists():
        raise FileNotFoundError(path)

    sections: list[tuple[str, list[str]]] = []
    current_title = ""
    current_lines: list[str] = []

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        match = _SECTION_RE.match(line)
        if match:
            if current_title and current_lines:
                sections.append((current_title, current_lines))
            current_title = match.group(1)
            current_lines = []
            continue
        current_lines.append(line)

    if current_title and current_lines:
        sections.append((current_title, current_lines))

    records: list[MemoryRecord] = []
    for title, lines in sections:
        category, normalized_title, keywords = _SECTION_MAPPING.get(title, ("note", title, [title]))
        records.append(
            MemoryRecord.from_mapping(
                {
                    "category": category,
                    "title": normalized_title,
                    "content": "\n".join(lines),
                    "keywords": keywords,
                    "source": "legacy_text",
                },
                default_source="legacy_text",
            )
        )
    return records
