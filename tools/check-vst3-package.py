#!/usr/bin/env python3
"""Check the exact installable package, not a pre-staging build directory."""
import argparse
import json
from pathlib import Path
import xml.etree.ElementTree as ET


def sdk_json(text):
    # SDK 3.8.1 emits trailing commas. Remove only commas outside strings;
    # never rewrite plugin/vendor text or relax other JSON validation.
    output = []
    quoted = escaped = False
    for i, char in enumerate(text):
        if quoted:
            output.append(char)
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
        elif char == '"':
            quoted = True
            output.append(char)
        elif char == ",":
            j = i + 1
            while j < len(text) and text[j].isspace():
                j += 1
            if j == len(text) or text[j] not in "}]":
                output.append(char)
        else:
            output.append(char)
    return json.loads("".join(output))


def check(bundle: Path, headless: bool = False):
    name = "125A VoiceMorph"
    if bundle.name != name + ".vst3":
        raise ValueError("Wrong official bundle name")
    windows = bundle / "Contents/x86_64-win" / (name + ".vst3")
    linux = bundle / "Contents/x86_64-linux" / (name + ".so")
    if not (windows.is_file() or linux.is_file()):
        raise ValueError("Outer bundle and architecture module names must match")
    if headless and windows.is_file():
        raise ValueError("Shipping Windows packages must contain the real GUI")
    resources = bundle / "Contents/Resources"
    for filename in ("LICENSE.signalsmith-linear.txt", "LICENSE.signalsmith-stretch.txt"):
        if not (resources / filename).is_file():
            raise ValueError("Missing dependency notice: " + filename)
    if not headless:
        ET.parse(resources / "voicemorph.uidesc")
        png = (resources / "125A_Logo_Master.png").read_bytes()
        if not png.startswith(b"\x89PNG\r\n\x1a\n"):
            raise ValueError("Missing or invalid logo PNG")
    moduleinfo = resources / "moduleinfo.json"
    if not moduleinfo.is_file():
        raise ValueError("Missing moduleinfo.json")
    if moduleinfo.exists():
        info = sdk_json(moduleinfo.read_text())
        if info.get("Name") != name or info.get("Version") != "0.1.0":
            raise ValueError("Unexpected module name or version metadata")
        ids = {c["CID"].upper() for c in info["Classes"]}
        if not {"FB9F13015F2946D1A1F900170B11A009", "FB9F13025F2946D1A1F900170B11A009"} <= ids:
            raise ValueError("Unexpected processor/controller class IDs")
    print("Exact package naming, resources, licenses and available class metadata PASS")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("bundle", type=Path)
    parser.add_argument("--headless", action="store_true")
    args = parser.parse_args()
    check(args.bundle, args.headless)
