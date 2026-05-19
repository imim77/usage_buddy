try:
    Import("env")  # type: ignore[name-defined]
except NameError:
    env = {}

from pathlib import Path


def parse_env_file(env_path):
    parsed = {}

    if not env_path.exists():
        return parsed

    for raw_line in env_path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()

        if not line or line.startswith("#") or "=" not in line:
            continue

        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip()

        if not key:
            continue

        quoted = (value.startswith('"') and value.endswith('"')) or (
            value.startswith("'") and value.endswith("'")
        )

        if quoted:
            value = value[1:-1]

        parsed[key] = (value, quoted)

    return parsed


def to_define(value, quoted):
    if not quoted and value.lstrip("-").isdigit():
        return value

    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'


project_dir = Path(env["PROJECT_DIR"])
env_vars = parse_env_file(project_dir / ".env")
header_path = project_dir / "include" / "env_config.h"

lines = ["#pragma once", ""]
for key, (value, quoted) in env_vars.items():
    lines.append(f"#define {key} {to_define(value, quoted)}")

lines.append("")
header_path.write_text("\n".join(lines), encoding="utf-8")
