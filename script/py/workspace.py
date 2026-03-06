"""
Workspace commands: ws_init, ws_close, init_workspace, reset_workspace.
"""
import re
import shutil
import subprocess
import sys

from paths import get_project_root, get_sources_dir, ensure_git_repo
from git_ops import (
    git_submodule_exists,
    get_all_submodule_paths,
    ensure_branch,
)
from checks import require_valid_ws_name, require_git_ok


def ws_init(ws_name, submodule_url=None):
    """
    Initialize workspace: add submodule for sources/WS_NAME, create common.
    Works in current repo/branch only (no branch switching).
    If no --url given, creates a local empty bare repo and adds it as submodule.
    Returns ALREADY_INIT exit if submodule already exists.
    """
    require_valid_ws_name(ws_name)
    root = ensure_git_repo()
    sources = get_sources_dir()
    ws_path = f"sources/{ws_name}"

    # Check: workspace dir exists but is NOT a submodule -> conflict
    ws_dir = sources / ws_name
    if ws_dir.exists() and not ws_dir.joinpath(".git").exists() and not git_submodule_exists(root, ws_path):
        # Plain directory with content - could be leftover
        if any(ws_dir.iterdir()):
            print(f"Error: {ws_name} directory exists and is not empty (not a submodule)", file=sys.stderr)
            sys.exit(1)

    if git_submodule_exists(root, ws_path):
        print(f"ws_init: {ws_name} already initialized (submodule exists)")
        sys.exit(EXIT_ALREADY_INIT)

    if submodule_url:
        url = str(submodule_url).strip()
        if not url:
            print("Error: --url given but empty", file=sys.stderr)
            sys.exit(1)
    else:
        ws_dir = sources / ws_name
        ws_dir.mkdir(parents=True, exist_ok=True)
        r = subprocess.run(["git", "init"], cwd=ws_dir, capture_output=True, text=True)
        if r.returncode != 0:
            require_git_ok(r, "init", ws_dir)
        r = subprocess.run(
            ["git", "-c", "user.email=elcd@local", "-c", "user.name=elcd", "commit", "--allow-empty", "-m", "init"],
            cwd=ws_dir, capture_output=True, text=True
        )
        if r.returncode != 0:
            require_git_ok(r, "commit --allow-empty", ws_dir)
        url = f"./sources/{ws_name}"

    add_cmd = ["git", "submodule", "add", url, ws_path]
    if url.startswith("file://"):
        add_cmd = ["git", "-c", "protocol.file.allow=always", "submodule", "add", url, ws_path]

    def _clean_submodule_state(err):
        """Remove broken/leftover submodule state so add can succeed."""
        mod_dir = root / ".git" / "modules" / "sources" / ws_name
        ws_dir = root / "sources" / ws_name
        if ws_dir.exists():
            shutil.rmtree(ws_dir, ignore_errors=True)
        if mod_dir.exists():
            shutil.rmtree(mod_dir, ignore_errors=True)
        if "already exists in the index" in err:
            subprocess.run(["git", "rm", "--cached", "-f", ws_path], cwd=root, capture_output=True, check=False)
            subprocess.run(["git", "submodule", "deinit", "-f", ws_path], cwd=root, capture_output=True, check=False)
            gitmodules = root / ".gitmodules"
            if gitmodules.exists():
                content = gitmodules.read_text(encoding="utf-8")
                pattern = r'\[submodule "[^"]+"\]\s*\n\s*path\s*=\s*[^\n]+\n\s*url\s*=\s*[^\n]+\n?'
                for m in re.finditer(pattern, content):
                    if ws_path in m.group(0):
                        content = content.replace(m.group(0), "")
                content = re.sub(r'\n{3,}', '\n\n', content).strip()
                gitmodules.write_text((content + "\n") if content else "\n", encoding="utf-8")

    r = subprocess.run(add_cmd, cwd=root, capture_output=True, text=True)
    err = r.stderr or ""
    if r.returncode != 0 and ("already exists in the index" in err or "A git directory for" in err or "not a git repository" in err):
        _clean_submodule_state(err)
        r2 = subprocess.run(add_cmd, cwd=root, capture_output=True, text=True)
        if r2.returncode != 0:
            print(f"Error: git submodule add failed after cleanup: {r2.stderr or r2.stdout}", file=sys.stderr)
            sys.exit(1)
    elif r.returncode != 0:
        print(f"Error: git submodule add failed: {r.stderr or r.stdout}", file=sys.stderr)
        sys.exit(1)
    r = subprocess.run(["git", "submodule", "update", "--init", "--recursive"], cwd=root, capture_output=True, text=True)
    if r.returncode != 0:
        require_git_ok(r, "submodule update --init --recursive", root)

    ws_dir = sources / ws_name
    common_dir = ws_dir / "common"
    ws_dir.mkdir(parents=True, exist_ok=True)
    common_dir.mkdir(parents=True, exist_ok=True)

    print(f"ws_init {ws_name}: done")


def ws_close(ws_name):
    """
    Remove workspace and all submodules from repo (no branch switch - single repo mode).
    Deinits every submodule, removes from index, cleans .git/modules.
    """
    require_valid_ws_name(ws_name)
    ensure_git_repo()
    root = get_project_root()
    ws_path = f"sources/{ws_name}"
    all_paths = get_all_submodule_paths(root)
    if git_submodule_exists(root, ws_path) and ws_path not in all_paths:
        all_paths.append(ws_path)
        all_paths = sorted(all_paths, key=lambda x: -x.count("/"))
    if not all_paths:
        ws_dir = root / "sources" / ws_name
        mod_dir = root / ".git" / "modules" / "sources" / ws_name
        for d in (ws_dir, mod_dir):
            if d.exists():
                subprocess.run(["cmd", "/c", "rmdir", "/s", "/q", str(d)], capture_output=True, check=False)
                shutil.rmtree(d, ignore_errors=True)
        print(f"ws_close {ws_name}: cleaned leftover dirs (no submodules)")
        return
    subprocess.run(["git", "add", ".gitmodules"], cwd=root, capture_output=True, check=False)
    for path in all_paths:
        subprocess.run(["git", "submodule", "deinit", "-f", path], cwd=root, capture_output=True, check=False)
        subprocess.run(["git", "rm", "-f", path], cwd=root, capture_output=True, check=False)
        sub_dir = root / path
        mod_dir = root / ".git" / "modules" / path
        for d in (sub_dir, mod_dir):
            if d.exists():
                subprocess.run(["cmd", "/c", "rmdir", "/s", "/q", str(d)], cwd=root, capture_output=True, check=False)
                shutil.rmtree(d, ignore_errors=True)
    print(f"ws_close {ws_name}: done")


def init_workspace(ws_name):
    require_valid_ws_name(ws_name)
    root = ensure_git_repo()
    sources = get_sources_dir()
    ws_dir = sources / ws_name
    common_dir = ws_dir / "common"

    ensure_branch(root, ws_name)
    ws_dir.mkdir(parents=True, exist_ok=True)
    common_dir.mkdir(parents=True, exist_ok=True)
    print(f"init_workspace {ws_name}: created {ws_dir}, {common_dir}")


def reset_workspace(ws_name):
    require_valid_ws_name(ws_name)
    root = ensure_git_repo()
    r = subprocess.run(["git", "switch", "main"], cwd=root, capture_output=True, text=True)
    if r.returncode != 0:
        require_git_ok(r, "switch main", root)
    sources = get_sources_dir()
    ws_dir = sources / ws_name
    if ws_dir.exists():
        shutil.rmtree(ws_dir)
        print(f"reset_workspace {ws_name}: removed {ws_dir}")
    else:
        print(f"reset_workspace {ws_name}: {ws_dir} did not exist")
