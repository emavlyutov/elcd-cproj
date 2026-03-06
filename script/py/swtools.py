#!/usr/bin/env python3
"""
script/py/swtools.py - Workspace and project management for software repo
Usage: python swtools.py <function> [WS_NAME] [PROJECT_NAME] [args...]
Functions: ws_init, ws_close, prj_init, prj_close, prj_remove, prj_regenerate,
           prj_add_submodule, prj_remove_submodule,
           init_workspace, reset_workspace, init_project, reset_project,
           regenerate_project, build_project, clean_project,
           clean_workspace, clean_all, clean_all_workspace,
           init_prj, reset_prj (batch init/reset from XML)
"""

import argparse
import sys

# Ensure script/py is on path for local imports
from pathlib import Path
_script_dir = Path(__file__).resolve().parent
if str(_script_dir) not in sys.path:
    sys.path.insert(0, str(_script_dir))

from xml_parse import find_submodule_by_name_or_path
from git_ops import find_workspace_for_project
from workspace import ws_init, ws_close, init_workspace, reset_workspace
from project import (
    prj_init,
    prj_add_submodule,
    prj_remove_submodule,
    init_project,
    init_prj,
    prj_remove,
    prj_close,
    prj_regenerate,
    reset_project,
    reset_prj,
    regenerate_project,
)
from build import build_project, clean_project, clean_workspace, clean_all, clean_all_workspace


def main():
    parser = argparse.ArgumentParser(description="software swtools")
    parser.add_argument("function", help="Command")
    parser.add_argument("ws_name", nargs="?", help="Workspace name")
    parser.add_argument("project_name", nargs="?", help="Project name (for project-specific commands)")
    parser.add_argument("extra", nargs="*")
    parser.add_argument("--bsp", dest="bsp_filter")
    parser.add_argument("--project", dest="proj_filter")
    parser.add_argument("--url", dest="submodule_url", help="Submodule URL for ws_init")
    args = parser.parse_args()

    func = args.function.replace("-", "_")
    ws = args.ws_name
    proj = args.project_name

    handlers = {
        "ws_init": lambda: ws_init(ws, args.submodule_url),
        "ws_close": lambda: ws_close(ws or (args.extra[0] if args.extra else None)),
        "prj_init": lambda: prj_init(ws, proj or (args.extra[0] if args.extra else ""), args.extra[0] if ws and proj and args.extra else (args.extra[1] if len(args.extra) > 1 else "ZynqPS0")),
        "prj_remove": lambda: prj_remove(ws, proj or (args.extra[0] if args.extra else "")),
        "prj_close": lambda: prj_close(ws, proj or (args.extra[0] if args.extra else "")),
        "prj_regenerate": lambda: prj_regenerate(ws, proj or (args.extra[0] if args.extra else "")),
        "prj_add_submodule": lambda: prj_add_submodule(ws, proj or (args.extra[0] if args.extra else ""), _get_submodule_arg(args, ws, proj)),
        "prj_remove_submodule": lambda: prj_remove_submodule(ws, proj or (args.extra[0] if args.extra else ""), _get_submodule_arg(args, ws, proj)),
        "init_workspace": lambda: init_workspace(ws),
        "reset_workspace": lambda: reset_workspace(ws),
        "init_project": lambda: init_project(ws, proj or (args.extra[0] if args.extra else "")),
        "init_prj": lambda: init_prj(ws, args.bsp_filter, args.proj_filter),
        "reset_project": lambda: reset_project(ws, proj or (args.extra[0] if args.extra else "")),
        "reset_prj": lambda: reset_prj(ws),
        "regenerate_project": lambda: regenerate_project(ws, proj or (args.extra[0] if args.extra else "")),
        "build_project": lambda: build_project(ws, proj or (args.extra[0] if args.extra else None), args.bsp_filter),
        "clean_project": lambda: clean_project(ws, proj or (args.extra[0] if args.extra else None), args.bsp_filter),
        "clean_workspace": lambda: clean_workspace(ws),
        "clean_all": lambda: clean_all(ws),
        "clean_all_workspace": lambda: clean_all_workspace(ws),
    }

    def _get_submodule_arg(args, ws, proj):
        """Extract submodule name. prj_add_submodule test1 liblwip: submodule=liblwip (from 2nd arg)."""
        if ws and proj and args.extra:
            return args.extra[0]
        if len(args.extra) > 1:
            return args.extra[1]
        if args.extra:
            return args.extra[0]
        orig = args.project_name or proj
        if ws and orig and find_submodule_by_name_or_path(orig):
            return orig
        return ""

    if func not in handlers:
        func, ws = "ws_init", args.function
    if not ws and func not in ("clean_all_workspace", "ws_close"):
        if func == "ws_close" and args.extra:
            ws = args.extra[0]
        elif func in ("prj_init", "prj_add_submodule", "prj_remove_submodule", "prj_remove", "prj_regenerate") and args.extra:
            ws = args.extra[0]
            if len(args.extra) > 1 and not proj:
                proj = args.extra[1]
        if not ws and func not in ("prj_init", "prj_close", "prj_add_submodule", "prj_remove_submodule", "prj_remove", "prj_regenerate"):
            print("ws_name required", file=sys.stderr)
            sys.exit(1)
        elif not ws and func in ("prj_init", "prj_close", "prj_add_submodule", "prj_remove_submodule", "prj_remove", "prj_regenerate"):
            print("WS_NAME required as first arg (e.g. swtools prj_init elvpn fsbl)", file=sys.stderr)
            sys.exit(1)
    if func in ("prj_add_submodule", "prj_remove_submodule") and ws and proj and not args.extra:
        if find_submodule_by_name_or_path(proj):
            found_ws = find_workspace_for_project(ws)
            if found_ws:
                ws, proj = found_ws, ws
            else:
                proj = ws

    handlers[func]()


if __name__ == "__main__":
    main()
