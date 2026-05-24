#!/usr/bin/env python3
"""
Verify MsgIDs.h enum alignment with .lng localization files.

Usage:
    python scripts/verify_lng_alignment.py

Returns exit code 0 on success, 1 on failure.
Prints diagnostic output including the line number where each enum maps
to its string in every .lng file.

Rule: Every NB_* and MSG_* enum identifier in MsgIDs.h must have a matching
quoted string at the same zero-based index in every .lng file. Blank lines
and the .Language= header do not count as strings.
"""

import os
import re
import sys
# Force UTF-8 output on Windows to handle non-ASCII translations
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)

MSGIDS_H = os.path.join(PROJECT_ROOT, "src", "base", "MsgIDs.h")
LNG_DIR = os.path.join(PROJECT_ROOT, "src", "NetBox")
LNG_FILES = [
    "NetBoxEng.lng",
    "NetBoxRus.lng",
    "NetBoxFr.lng",
    "NetBoxPol.lng",
    "NetBoxSpa.lng",
]


def parse_msgids(path):
    """Extract ordered list of enum identifiers from MsgIDs.h."""
    with open(path, "rb") as f:
        text = f.read().decode("utf-8", errors="replace")

    enums = []
    for line in text.replace("\r\n", "\n").split("\n"):
        stripped = line.strip()
        m = re.match(r"^(NB_|MSG_)\w+\s*[;,]", stripped)
        if m and not stripped.startswith("//"):
            name = re.match(r"^(NB_|MSG_)\w+", stripped).group(0)
            enums.append(name)
    return enums


def parse_lng(path):
    """Extract ordered list of (1_based_line, text) for quoted strings."""
    with open(path, "rb") as f:
        text = f.read().decode("utf-8", errors="replace")

    strings = []
    for idx, line in enumerate(text.replace("\r\n", "\n").split("\n"), start=1):
        stripped = line.strip()
        if stripped.startswith('"'):
            strings.append((idx, stripped))
    return strings


def main():
    errors = 0

    if not os.path.isfile(MSGIDS_H):
        print(f"ERROR: {MSGIDS_H} not found")
        return 1

    enums = parse_msgids(MSGIDS_H)
    print(f"MsgIDs.h      : {len(enums)} enum identifiers")

    lng_data = {}
    for fname in LNG_FILES:
        fpath = os.path.join(LNG_DIR, fname)
        if not os.path.isfile(fpath):
            print(f"ERROR: {fpath} not found")
            errors += 1
            continue
        strings = parse_lng(fpath)
        lng_data[fname] = strings
        print(f"{fname:14s}: {len(strings)} quoted strings")

        if len(strings) != len(enums):
            errors += 1
            print(f"  ^^^ MISMATCH: expected {len(enums)}, got {len(strings)}")

    if errors:
        print(f"\nFAILED: {errors} file(s) have mismatched string counts.")
        return 1

    # Verify key indices for preset block (known hard points)
    preset_enums = [
        "NB_TRANSFER_PRESET_LABEL",
        "NB_TRANSFER_PRESET_DEFAULT",
        "NB_TRANSFER_PRESET_TEXT_ASCII",
        "NB_TRANSFER_PRESET_BINARY_ALL",
        "NB_TRANSFER_PRESET_NO_PRESERVE_TIME",
    ]
    for ename in preset_enums:
        try:
            eidx = enums.index(ename)
        except ValueError:
            print(f"WARNING: {ename} not found in MsgIDs.h")
            continue

        print(f"\n  {ename} (enum index {eidx}):")
        for fname in LNG_FILES:
            line_num, text = lng_data[fname][eidx]
            print(f"    {fname:14s} line {line_num:4d}: {text[:50]}")

    print("\nPASSED: All .lng files aligned with MsgIDs.h")
    return 0


if __name__ == "__main__":
    sys.exit(main())
