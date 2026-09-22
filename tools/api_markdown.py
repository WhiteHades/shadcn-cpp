#!/usr/bin/env python3
"""Format clang-doc Markdown for the Docusaurus reference."""
from pathlib import Path
import re
import sys

generated_qt_members = {"metaObject", "qt_metacall", "qt_metacast", "tr"}

def normalize(text: str) -> str:
    lines = text.splitlines()
    class_name = None
    filtered = []
    skip = False
    for line in lines:
        class_match = re.fullmatch(r"# class (.+)", line)
        if class_match:
            class_name = class_match.group(1)
        if class_name:
            line = line.replace(f"*public void {class_name}(", f"*public {class_name}(")
        if line.startswith("### ") and line[4:].strip() in generated_qt_members:
            skip = True
            continue
        if skip and line.startswith("##"):
            skip = False
        if skip or line.strip() == "public static QMetaObject staticMetaObject":
            continue
        filtered.append(line)

    normalized = []
    enum_name = None
    for line in filtered:
        match = re.fullmatch(r"\| enum class ([^|]+) \|", line.strip())
        if match:
            enum_name = match.group(1).strip()
            normalized.extend([f"### `{enum_name}`", "", "Values:", ""])
            continue
        if enum_name:
            value = re.fullmatch(r"\| ([^|]+) \|", line.strip())
            if value:
                normalized.append(f"- `{value.group(1).strip()}`")
                continue
            if line.strip() == "--":
                continue
            if line.startswith("*Defined at"):
                normalized.extend([line, ""])
                enum_name = None
                continue
        normalized.append(line)

    text = "\n".join(normalized).rstrip() + "\n"
    text = re.sub(r"(?m)^## Members\n(?:\n)*## Functions\n", "## Functions\n", text)
    text = re.sub(r"(?m)^(public|protected) ([^\n]+)$", r"```cpp\n\1 \2\n```", text)
    text = re.sub(r"(?m)^\*(?!Defined at)(.+)\*$", r"```cpp\n\1\n```", text)
    text = re.sub(r"\*Defined at (include/shadcn/[^#]+)#(\d+)\*",
                  r"[Source](https://github.com/WhiteHades/shadcn-cpp/blob/main/\1#L\2)", text)
    return "---\nmdx:\n  format: md\n---\n\n" + text


if __name__ == "__main__":
    source, output = map(Path, sys.argv[1:])
    for path in source.glob("*.md"):
        (output / path.name).write_text(normalize(path.read_text()))
