#!/usr/bin/env python3
"""Check version, catalogue and parity evidence without network access."""
from __future__ import annotations
import json
from pathlib import Path
import re
import sys
from upstream import commit_id, safe_path

ROOT = Path(__file__).resolve().parents[1]
ALLOWED = {"planned", "draft", "implemented", "parity-reviewed"}


def validate_manifest(data: dict, root: Path) -> None:
    commit_id(data["upstream"]["commit"])
    safe_path(data["upstream"]["profile"])
    seen: set[str] = set()
    for item in data["components"]:
        name = item["id"]
        if name in seen: raise ValueError(f"Duplicate component: {name}")
        seen.add(name)
        safe_path(item["source_path"])
        commit_id(item["blob_sha"])
        if item["status"] not in ALLOWED: raise ValueError(f"Invalid component status: {name}")
        required: list[str] = []
        if item["status"] in ("implemented", "parity-reviewed"):
            required = ["native_build", "interaction"]
        if item["status"] == "parity-reviewed":
            required += ["visual", "animation", "accessibility"]
        for check in required:
            if item[check] != "passed": raise ValueError(f"{name} has not passed {check}")
            evidence = item.get("evidence", {}).get(check)
            if not isinstance(evidence, str): raise ValueError(f"{name} needs evidence for {check}")
            safe_path(evidence)
            candidate = (root / evidence).resolve()
            if not candidate.is_relative_to(root.resolve()) or not candidate.is_file():
                raise ValueError(f"Missing or unsafe evidence file: {evidence}")
    for source in data["sources"]:
        safe_path(source["path"]); commit_id(source["blob_sha"])


def main() -> int:
    try:
        version = (ROOT / "VERSION").read_text().strip()
        if version != "0.1.0": raise ValueError("The maintainer has not authorised a version change")
        header = (ROOT / "include/shadcn/core.hpp").read_text()
        if f'version = "{version}"' not in header: raise ValueError("C++ version differs from VERSION")
        if not (ROOT / "docs/api.md").is_file(): raise ValueError("C++ API guide is missing")
        manifest = json.loads((ROOT / "upstream/manifest.json").read_text())
        if manifest["library_version"] != version: raise ValueError("Manifest version differs")
        validate_manifest(manifest, ROOT)
        package = json.loads((ROOT / "website/package.json").read_text())
        if package["version"] != version: raise ValueError("Documentation package version differs")
        if not (ROOT / "website/package-lock.json").is_file():
            raise ValueError("Documentation package lockfile is missing")
        catalogue = json.loads((ROOT / "upstream/catalogue.json").read_text())
        ids = [x["id"] for x in catalogue["items"]]
        if len(ids) != len(set(ids)): raise ValueError("Duplicate catalogue item")
        if catalogue["commit"] != manifest["upstream"]["commit"]: raise ValueError("Catalogue pin differs")
        for item in catalogue["items"]:
            if item["status"] not in ALLOWED: raise ValueError("Invalid catalogue status")
        by_id = {x["id"]: x for x in catalogue["items"]}
        for item in manifest["components"]:
            if by_id[item["id"]]["status"] != item["status"]: raise ValueError("Catalogue status differs")
        licence = (ROOT / "LICENSES/shadcn-MIT.txt").read_text()
        if "Copyright (c) 2023 shadcn" not in licence: raise ValueError("Upstream notice is missing")
        for path in (ROOT / ".github/workflows").glob("*.yml"):
            for action in re.findall(r"uses:\s*([^\s#]+)", path.read_text()):
                if not re.fullmatch(r"[^@]+@[0-9a-f]{40}", action):
                    raise ValueError(f"Action must use a commit pin: {action}")
        print(f"Version {version}; {len(ids)} registry entries; {len(manifest['components'])} draft records checked")
        print("Manifest consistency passed. This is not a native build or parity certification.")
        return 0
    except (OSError, ValueError, KeyError) as error:
        print(f"check: {error}", file=sys.stderr)
        return 1

if __name__ == "__main__": raise SystemExit(main())
