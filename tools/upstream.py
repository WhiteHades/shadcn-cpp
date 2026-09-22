#!/usr/bin/env python3
"""Read upstream Git objects and compare source pins. Never execute upstream code."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys
from typing import Any

ROOT = Path(__file__).resolve().parents[1]


def blob_sha(content: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(content)).encode("ascii") + b"\0" + content).hexdigest()


def safe_path(value: str) -> str:
    path = PurePosixPath(value)
    if not value or path.is_absolute() or any(p in ("", ".", "..") for p in value.split("/")) or "\\" in value or "\0" in value:
        raise ValueError(f"Unsafe repository path: {value!r}")
    return value


def commit_id(value: str) -> str:
    if not re.fullmatch(r"[0-9a-f]{40}", value):
        raise ValueError("Expected a full 40-character commit SHA, not a moving branch or tag")
    return value


def git(checkout: Path, *args: str) -> bytes:
    result = subprocess.run(["git", "-C", str(checkout), *args], capture_output=True, check=False)
    if result.returncode:
        raise ValueError(result.stderr.decode("utf-8", errors="replace").strip())
    return result.stdout


def sources(manifest: dict[str, Any]) -> list[tuple[str, str, str]]:
    rows = [(x["id"], x["path"], x["blob_sha"]) for x in manifest["sources"]]
    rows += [(x["id"], x["source_path"], x["blob_sha"]) for x in manifest["components"]]
    return rows


def verify(checkout: Path, manifest: dict[str, Any]) -> dict[str, Any]:
    commit = commit_id(manifest["upstream"]["commit"])
    checked = []
    for name, path, expected in sources(manifest):
        data = git(checkout, "show", f"{commit}:{safe_path(path)}")
        actual = blob_sha(data)
        if actual != expected:
            raise ValueError(f"Source mismatch for {name}: expected {expected}, read {actual}")
        checked.append({"id": name, "path": path, "blob_sha": actual})
    return {"commit": commit, "verified_source_objects": checked,
            "note": "Source identity only. This does not validate the C++ implementation."}


def changes(checkout: Path, manifest: dict[str, Any], candidate: str) -> dict[str, Any]:
    before = commit_id(manifest["upstream"]["commit"])
    after = commit_id(candidate)
    paths = [manifest["upstream"]["profile"], "apps/v4/registry/themes.ts"]
    data = git(checkout, "diff", "--name-status", before, after, "--", *paths)
    return {"pinned_commit": before, "candidate_commit": after,
            "changed_paths": data.decode("utf-8").splitlines(),
            "note": "Review these changes. No pins or component statuses were modified."}


def inventory(checkout: Path, manifest: dict[str, Any]) -> dict[str, Any]:
    commit = commit_id(manifest["upstream"]["commit"])
    profile = safe_path(manifest["upstream"]["profile"])
    data = git(checkout, "ls-tree", "-r", commit, "--", profile)
    files = []
    for line in data.decode("utf-8").splitlines():
        meta, path = line.split("\t", 1)
        mode, kind, sha = meta.split()
        if kind == "blob" and path.endswith(".tsx"):
            files.append({"path": path, "blob_sha": sha})
    return {"commit": commit, "files": files, "note": "Inventory is not an implementation status."}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("operation", choices=["verify", "changes", "inventory"])
    parser.add_argument("--checkout", type=Path, required=True, help="Local upstream repository checkout")
    parser.add_argument("--candidate", help="Full candidate commit SHA, required for changes")
    parser.add_argument("--output", type=Path, help="Optional JSON report destination")
    parser.add_argument("--manifest", type=Path, default=ROOT / "upstream/manifest.json")
    args = parser.parse_args()
    try:
        manifest = json.loads(args.manifest.read_text())
        if args.operation == "verify": report = verify(args.checkout, manifest)
        elif args.operation == "inventory": report = inventory(args.checkout, manifest)
        else:
            if not args.candidate: raise ValueError("changes requires --candidate")
            report = changes(args.checkout, manifest, args.candidate)
        text = json.dumps(report, indent=2) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(text)
        else: print(text, end="")
        return 0
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        print(f"upstream: {error}", file=sys.stderr)
        return 1

if __name__ == "__main__":
    raise SystemExit(main())
