# SPDX-License-Identifier: MIT
from __future__ import annotations
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from upstream import blob_sha, commit_id, safe_path, verify, changes, inventory
from check import validate_manifest

class ToolTests(unittest.TestCase):
    def test_known_git_blob(self):
        self.assertEqual(blob_sha(b""), "e69de29bb2d1d6434b8b29ae775ad8c2e48c5391")
    def test_paths_reject_escape(self):
        for path in ("", "../secret", "/etc/passwd", "a/../b", "a\\b", "a\0b", "a//b", "a/./b"):
            with self.subTest(path=path):
                with self.assertRaises(ValueError): safe_path(path)
    def test_full_commit_required(self):
        for value in ("main", "v0.1.0", "a" * 39, "G" * 40):
            with self.assertRaises(ValueError): commit_id(value)
        self.assertEqual(commit_id("a" * 40), "a" * 40)
    def test_drafts_are_honest(self):
        data = json.loads((ROOT / "upstream/manifest.json").read_text())
        validate_manifest(data, ROOT)
    def test_claim_without_evidence_fails(self):
        data = json.loads((ROOT / "upstream/manifest.json").read_text())
        data["components"][0]["status"] = "parity-reviewed"
        with self.assertRaises(ValueError): validate_manifest(data, ROOT)
    def test_declared_pass_still_needs_evidence(self):
        data = json.loads((ROOT / "upstream/manifest.json").read_text())
        item = data["components"][0]
        item.update(status="implemented", native_build="passed", interaction="passed")
        with self.assertRaises(ValueError): validate_manifest(data, ROOT)
    def test_local_git_verification_and_changes(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            def run(*args):
                return subprocess.check_output(["git", "-C", directory, *args], stderr=subprocess.DEVNULL).decode().strip()
            run("init", "-b", "main")
            run("config", "user.name", "Test")
            run("config", "user.email", "test@example.invalid")
            (path / "ui").mkdir()
            (path / "ui/button.tsx").write_bytes(b"first\n")
            run("add", "."); run("commit", "-m", "first")
            first = run("rev-parse", "HEAD")
            manifest = {"upstream": {"commit":first, "profile":"ui"}, "sources":[],
                        "components":[{"id":"button", "source_path":"ui/button.tsx", "blob_sha":blob_sha(b"first\n")}]}
            self.assertEqual(len(verify(path, manifest)["verified_source_objects"]), 1)
            self.assertEqual(len(inventory(path, manifest)["files"]), 1)
            (path / "ui/button.tsx").write_bytes(b"second\n")
            run("add", "."); run("commit", "-m", "second")
            second = run("rev-parse", "HEAD")
            self.assertEqual(changes(path, manifest, second)["changed_paths"], ["M\tui/button.tsx"])
            broken = copy.deepcopy(manifest); broken["components"][0]["blob_sha"] = "0" * 40
            with self.assertRaises(ValueError): verify(path, broken)

if __name__ == "__main__": unittest.main()
