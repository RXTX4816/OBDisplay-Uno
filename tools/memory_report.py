#!/usr/bin/env python3
"""Turn the RAM/Flash lines of `pio run -e uno -e uno_debug` into release outputs.

Usage:
    pio run -e uno -e uno_debug | tee build.log
    python3 tools/memory_report.py build.log OUT_DIR

Writes to OUT_DIR:
    memory.json          both builds, attached to the GitHub release
    memory.md            "Memory usage" section for the release notes
    badges/*.json        shields.io endpoint badges (ram, flash, ram-debug, flash-debug)
"""

import json
import os
import re
import sys

ENVS = (("uno", ""), ("uno_debug", "-debug"))

PROCESSING = re.compile(r"^Processing (\S+)")
USAGE = re.compile(r"^(RAM|Flash):.*?([\d.]+)% \(used (\d+) bytes from (\d+) bytes\)")


def parse(log):
    """{env: {"ram": {...}, "flash": {...}}} from PlatformIO build output."""
    result = {}
    env = None
    for line in log.splitlines():
        m = PROCESSING.match(line)
        if m:
            env = m.group(1)
            continue
        m = USAGE.match(line)
        if m and env:
            kind = m.group(1).lower()
            result.setdefault(env, {})[kind] = {
                "percent": float(m.group(2)),
                "used": int(m.group(3)),
                "total": int(m.group(4)),
            }
    for env, _ in ENVS:
        if set(result.get(env, {})) != {"ram", "flash"}:
            sys.exit("memory_report: no RAM/Flash lines for env '%s' in build log" % env)
    return {env: result[env] for env, _ in ENVS}


def color(percent):
    if percent < 75:
        return "brightgreen"
    if percent < 90:
        return "yellow"
    if percent < 97:
        return "orange"
    return "red"


def badge(label, usage):
    return {
        "schemaVersion": 1,
        "label": label,
        "message": "%.1f%% of %dB" % (usage["percent"], usage["total"]),
        "color": color(usage["percent"]),
    }


def markdown(mem):
    rows = [
        "### Memory usage",
        "",
        "| Build | Flash | RAM |",
        "|---|---|---|",
    ]
    for env, _ in ENVS:
        f, r = mem[env]["flash"], mem[env]["ram"]
        rows.append(
            "| `%s` | %d / %d bytes (%.1f%%) | %d / %d bytes (%.1f%%) |"
            % (env, f["used"], f["total"], f["percent"], r["used"], r["total"], r["percent"])
        )
    rows += ["", "The `.hex` attached to this release is the `uno` build.", ""]
    return "\n".join(rows)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    with open(sys.argv[1], encoding="utf-8", errors="replace") as fh:
        mem = parse(fh.read())
    out = sys.argv[2]
    os.makedirs(os.path.join(out, "badges"), exist_ok=True)

    with open(os.path.join(out, "memory.json"), "w") as fh:
        json.dump(mem, fh, indent=2)
        fh.write("\n")
    with open(os.path.join(out, "memory.md"), "w") as fh:
        fh.write(markdown(mem))
    for env, suffix in ENVS:
        tag = " (debug)" if suffix else ""
        for kind, label in (("ram", "RAM"), ("flash", "flash")):
            path = os.path.join(out, "badges", "%s%s.json" % (kind, suffix))
            with open(path, "w") as fh:
                json.dump(badge(label + tag, mem[env][kind]), fh)
                fh.write("\n")


if __name__ == "__main__":
    main()
