"""
Git operations: submodule status, commit, branch, etc.
"""
import re
import subprocess
import sys
from pathlib import Path

from paths import get_project_root, get_sources_dir
from checks import EXIT_GIT_ERROR, require_git_ok


def _run_git(cwd, *args, check=True):
    """Run git command. If check=True, exit on non-zero."""
    cwd_path = Path(cwd) if cwd else get_project_root()
    r = subprocess.run(["git"] + list(args), cwd=cwd_path, capture_output=True, text=True)
    if check and r.returncode != 0:
        require_git_ok(r, " ".join(args), cwd_path)
    return r


def git_cmd(cwd, *args, check=True):
    r = _run_git(cwd, *args, check=check)
    return r


def git_current_branch(cwd):
    """Return current branch name."""
    r = subprocess.run(["git", "rev-parse", "--abbrev-ref", "HEAD"], cwd=cwd, capture_output=True, text=True)
    return r.stdout.strip() if r.returncode == 0 else "main"


def git_branch_exists(cwd, branch):
    """Return True if branch exists locally or remotely."""
    r = subprocess.run(["git", "branch", "-a"], cwd=cwd, capture_output=True, text=True)
    out = r.stdout or ""
    return branch in out or f"remotes/origin/{branch}" in out


def git_submodule_exists(cwd, path):
    """Return True if path is a git submodule (in .gitmodules or index)."""
    r = subprocess.run(["git", "submodule", "status", "--cached", path], cwd=cwd, capture_output=True, text=True)
    if r.returncode == 0 and path in (r.stdout or ""):
        return True
    r2 = subprocess.run(["git", "ls-files", "-s", path], cwd=cwd, capture_output=True, text=True)
    return r2.returncode == 0 and r2.stdout.strip().startswith("160000")


def git_commit(cwd, message, *paths):
    """Stage and commit paths. If no paths, stage all. Returns True if committed."""
    cwd_path = Path(cwd) if isinstance(cwd, (str, Path)) else cwd
    if not cwd_path.exists():
        print(f"Error: commit cwd does not exist: {cwd_path}", file=sys.stderr)
        return False
    if paths:
        r = subprocess.run(["git", "add"] + list(paths), cwd=cwd_path, capture_output=True, text=True)
        if r.returncode != 0:
            print(f"git add failed: {r.stderr or r.stdout}", file=sys.stderr)
            return False
    else:
        r = subprocess.run(["git", "add", "-A"], cwd=cwd_path, capture_output=True, text=True)
        if r.returncode != 0:
            print(f"git add -A failed: {r.stderr or r.stdout}", file=sys.stderr)
            return False
    r = subprocess.run(["git", "status", "--short"], cwd=cwd_path, capture_output=True, text=True)
    if not r.stdout.strip():
        return False
    rc = subprocess.run(["git", "commit", "-m", message], cwd=cwd_path, capture_output=True, text=True)
    if rc.returncode != 0:
        print(rc.stderr or rc.stdout, file=sys.stderr)
        return False
    return True


def commit_cwd_for_prj(ws_name):
    """CWD for committing project files. Use ws submodule dir when it's a submodule."""
    root = get_project_root()
    ws_path = f"sources/{ws_name}"
    if git_submodule_exists(root, ws_path):
        return get_sources_dir() / ws_name
    return root


def find_workspace_for_project(prj_name):
    """Return ws_name if sources/WS/PRJ_NAME/platform.config exists, else None."""
    sources = get_sources_dir()
    if not sources.exists():
        return None
    for ws_dir in sources.iterdir():
        if ws_dir.is_dir():
            cfg = ws_dir / prj_name / "platform.config"
            if cfg.exists():
                return ws_dir.name
    return None


def ensure_branch(cwd, branch):
    """Switch to branch, creating if needed. Fails on git errors."""
    cwd_path = Path(cwd) if isinstance(cwd, (str, Path)) else cwd
    if not (cwd_path / ".git").exists():
        print(f"Error: not a git repo: {cwd_path}", file=sys.stderr)
        sys.exit(EXIT_GIT_ERROR)
    r = subprocess.run(["git", "branch", "-a"], cwd=cwd_path, capture_output=True, text=True)
    if r.returncode != 0:
        require_git_ok(r, "branch -a", cwd_path)
    has_branch = branch in (r.stdout or "") or f"remotes/origin/{branch}" in (r.stdout or "")
    r = subprocess.run(["git", "fetch"], cwd=cwd_path, capture_output=True, text=True)
    if r.returncode != 0:
        print(f"Warning: git fetch failed: {r.stderr or r.stdout}", file=sys.stderr)
    if has_branch:
        r = subprocess.run(["git", "switch", branch], cwd=cwd_path, capture_output=True, text=True)
        if r.returncode != 0:
            require_git_ok(r, f"switch {branch}", cwd_path)
        r = subprocess.run(["git", "submodule", "update", "--init", "--recursive"], cwd=cwd_path, capture_output=True, text=True)
        if r.returncode != 0:
            require_git_ok(r, "submodule update", cwd_path)
        subprocess.run(["git", "pull", "--rebase"], cwd=cwd_path, capture_output=True, check=False)
    else:
        r = subprocess.run(["git", "checkout", "-b", branch], cwd=cwd_path, capture_output=True, text=True)
        if r.returncode != 0:
            require_git_ok(r, f"checkout -b {branch}", cwd_path)
        r = subprocess.run(["git", "submodule", "update", "--init", "--recursive"], cwd=cwd_path, capture_output=True, text=True)
        if r.returncode != 0:
            require_git_ok(r, "submodule update", cwd_path)


def get_all_submodule_paths(cwd):
    """Return list of submodule paths from .gitmodules, deepest first (for safe removal)."""
    gitmodules = Path(cwd) / ".gitmodules"
    if not gitmodules.exists():
        return []
    paths = []
    for m in re.finditer(r'\[submodule\s+"([^"]+)"\][^\[]*?path\s*=\s*(\S+)', gitmodules.read_text(encoding="utf-8"), re.DOTALL):
        p = m.group(2).strip()
        if p and p not in paths:
            paths.append(p)
    return sorted(paths, key=lambda x: -x.count("/"))
