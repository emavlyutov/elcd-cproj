"""
Path helpers for swtools. Resolve script dir, project root, sources, XML dir.
"""
import sys
from pathlib import Path


def get_script_dir():
    """Directory containing the main script (script/py/)."""
    return Path(__file__).resolve().parent


def get_project_root():
    """Project root (repo root with .git, parent of script/)."""
    return get_script_dir().parent.parent


def get_sources_dir():
    """sources/ at project root."""
    return get_project_root() / "sources"


def get_script_xml_dir():
    """script/xml/ directory."""
    return get_project_root() / "script" / "xml"


def ensure_git_repo():
    """Verify we are in a git repository. Exit if not."""
    root = get_project_root()
    git_dir = root / ".git"
    if not git_dir.exists():
        print("Error: not a git repository (no .git in project root)", file=sys.stderr)
        sys.exit(1)
    return root


def ensure_sources_dir():
    """Verify sources/ exists or can be created. Return path."""
    sources = get_sources_dir()
    if not sources.parent.exists():
        print(f"Error: project root does not exist: {sources.parent}", file=sys.stderr)
        sys.exit(1)
    return sources
