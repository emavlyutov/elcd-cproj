"""
Validation and error checks for swtools. Exit codes, strict checks, sanity validation.
"""
import sys
from pathlib import Path
from typing import Optional, Any

# Exit codes
EXIT_SUCCESS = 0
EXIT_GENERIC = 1
EXIT_ALREADY_INIT = 2
EXIT_NOINIT = 3
EXIT_NOTFOUND = 4
EXIT_INVALID_TARGET = 5
EXIT_INVALID_STATE = 6
EXIT_GIT_ERROR = 7
EXIT_PATH_EXISTS = 8


def _fail(msg: str, exit_code: int = EXIT_GENERIC) -> None:
    """Print error and exit."""
    print(msg, file=sys.stderr)
    sys.exit(exit_code)


def require(cond: bool, msg: str, exit_code: int = EXIT_GENERIC) -> None:
    """Exit if condition is false."""
    if not cond:
        _fail(msg, exit_code)


def require_not_empty(value: Any, name: str, hint: str = "") -> None:
    """Require non-empty string value."""
    if value is None:
        _fail(f"{name} is required {hint}".strip(), EXIT_NOINIT)
    s = str(value).strip()
    if not s:
        _fail(f"{name} is required {hint}".strip(), EXIT_NOINIT)


def require_path_exists(path: Path, desc: str = "path", exit_code: int = EXIT_NOTFOUND) -> None:
    """Require path exists."""
    if not path.exists():
        _fail(f"{desc} does not exist: {path}", exit_code)


def require_path_not_exists(path: Path, desc: str = "path", exit_code: int = EXIT_PATH_EXISTS) -> None:
    """Require path does not exist (for create operations)."""
    if path.exists():
        _fail(f"{desc} already exists: {path}", exit_code)


def require_git_ok(r, cmd_desc: str, cwd: Optional[Path] = None) -> None:
    """Require git command returned 0. Exit with EXIT_GIT_ERROR otherwise."""
    if r.returncode != 0:
        err = (r.stderr or r.stdout or "unknown error").strip()
        cwd_s = f" (cwd={cwd})" if cwd else ""
        _fail(f"git {cmd_desc} failed{cwd_s}: {err}", EXIT_GIT_ERROR)


def require_subprocess_ok(r, cmd_desc: str) -> None:
    """Require subprocess returned 0."""
    if r.returncode != 0:
        err = (r.stderr or r.stdout or "unknown error").strip()
        _fail(f"{cmd_desc} failed (exit {r.returncode}): {err}", EXIT_GENERIC)


def sanitize_name(name: str, allow_slash: bool = False) -> str:
    """Return name stripped and validated. Reject path traversal."""
    s = str(name).strip()
    if ".." in s or (not allow_slash and "/" in s):
        _fail(f"Invalid name (path traversal?): {name}", EXIT_INVALID_STATE)
    return s


def is_valid_name_char(c: str) -> bool:
    """Allow alphanumeric, underscore, hyphen for ws/prj names."""
    return c.isalnum() or c in "_-"


def require_valid_ws_name(ws_name: str) -> None:
    """Validate workspace name format."""
    require_not_empty(ws_name, "Workspace name", "(e.g. elvpn)")
    s = str(ws_name).strip()
    if not all(is_valid_name_char(c) for c in s):
        _fail(f"Invalid workspace name (use alphanumeric, underscore, hyphen): {ws_name}", EXIT_INVALID_STATE)


def require_valid_prj_name(prj_name: str) -> None:
    """Validate project name format."""
    require_not_empty(prj_name, "Project name", "(e.g. fsbl)")
    s = str(prj_name).strip()
    if not all(is_valid_name_char(c) for c in s):
        _fail(f"Invalid project name (use alphanumeric, underscore, hyphen): {prj_name}", EXIT_INVALID_STATE)
