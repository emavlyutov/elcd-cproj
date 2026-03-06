"""
Build and clean commands.
"""
import os
import shutil
import subprocess
import sys

from paths import get_project_root, get_sources_dir
from checks import require_valid_ws_name, require_subprocess_ok


def build_project(ws_name, project_name=None, bsp_name=None):
    """Build: prefer hardware tools if available, else software Makefile."""
    require_valid_ws_name(ws_name)
    root_dir = get_project_root()
    sources = get_sources_dir()
    ws_dir = sources / ws_name
    if not ws_dir.exists():
        print(f"Error: workspace {ws_name} does not exist: {ws_dir}", file=sys.stderr)
        sys.exit(1)

    hw_xilinx = root_dir / "hardware" / "Xilinx"
    if not hw_xilinx.exists():
        hw_xilinx = root_dir / "hardware" / "xilinx"
    bsp_output = root_dir / "bsp" / "output" / "xilinx" / ws_name
    prj_dirs = list(bsp_output.glob("prj_*")) if bsp_output.exists() else []

    target = project_name
    if target:
        prj_dir = bsp_output / f"prj_{target}"
        if prj_dir.exists():
            r = subprocess.run(["make", "-C", str(prj_dir)], capture_output=True, text=True)
            if r.returncode != 0:
                require_subprocess_ok(r, f"make in {prj_dir}")
            return
    else:
        for d in prj_dirs:
            r = subprocess.run(["make", "-C", str(d)], capture_output=True, text=True)
            if r.returncode != 0:
                require_subprocess_ok(r, f"make in {d}")
        return

    make_dir = get_sources_dir()
    env = os.environ.copy()
    env["PROJECT_NAME"] = ws_name
    r = subprocess.run(["make", f"build_{target}"], cwd=str(make_dir), env=env, capture_output=True, text=True)
    if r.returncode != 0:
        require_subprocess_ok(r, f"make build_{target}")


def clean_project(ws_name, project_name=None, bsp_name=None):
    require_valid_ws_name(ws_name)
    root_dir = get_project_root()
    bsp_output = root_dir / "bsp" / "output" / "xilinx" / ws_name

    if project_name:
        prj_dir = bsp_output / f"prj_{project_name}"
        if prj_dir.exists():
            subprocess.run(["make", "-C", str(prj_dir), "clean"], check=False)
    else:
        for d in (bsp_output.glob("prj_*") if bsp_output.exists() else []):
            subprocess.run(["make", "-C", str(d), "clean"], check=False)

    sw_root = get_sources_dir() / ws_name
    build_dir = sw_root / "build"
    if build_dir.exists():
        shutil.rmtree(build_dir)
    print(f"clean_project {ws_name}: done")


def clean_workspace(ws_name):
    require_valid_ws_name(ws_name)
    build_dir = get_sources_dir() / "build"
    if build_dir.exists():
        shutil.rmtree(build_dir)
    print(f"clean_workspace {ws_name}: done")


def clean_all(ws_name):
    clean_workspace(ws_name)
    print(f"clean_all {ws_name}: done")


def clean_all_workspace(ws_name):
    clean_all(ws_name)
