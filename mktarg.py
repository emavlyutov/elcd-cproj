#!/usr/bin/env python3
"""
software/mktarg.py - Workspace and project management for software repo
Usage: python mktarg.py <function> [WS_NAME] [PROJECT_NAME] [args...]
Functions: init_workspace, reset_workspace, init_project, reset_project,
           regenerate_project, build_project, clean_project,
           clean_workspace, clean_all, clean_all_workspace
           init_prj, reset_prj (batch init/reset from XML - when called from root)
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path


def get_script_dir():
    return Path(__file__).resolve().parent


def get_root_dir():
    """Project root (parent of software/)."""
    return get_script_dir().parent


def load_xml(ws_name):
    """Load [WS_NAME].xml from project root."""
    root_dir = get_root_dir()
    xml_path = root_dir / f"{ws_name}.xml"
    if not xml_path.exists():
        return None
    import xml.etree.ElementTree as ET
    return ET.parse(xml_path).getroot()


def get_software_projects(root_el):
    """Yield (name, proc, os) for each software/project."""
    if root_el is None:
        return
    sw = root_el.find("software")
    if sw is None:
        return
    for proj in sw.findall("project"):
        name_el = proj.find("name")
        if name_el is None or not name_el.text:
            continue
        yield (name_el.text.strip(),)


def git_cmd(cwd, *args, check=True):
    r = subprocess.run(["git"] + list(args), cwd=cwd, capture_output=True, text=True)
    if check and r.returncode != 0:
        print(f"git {' '.join(args)} failed: {r.stderr}", file=sys.stderr)
        sys.exit(1)
    return r


def ensure_branch(cwd, branch):
    r = subprocess.run(["git", "branch", "-a"], cwd=cwd, capture_output=True, text=True)
    has_branch = branch in r.stdout or f"remotes/origin/{branch}" in r.stdout
    subprocess.run(["git", "fetch"], cwd=cwd, capture_output=True)
    if has_branch:
        git_cmd(cwd, "checkout", branch)
        git_cmd(cwd, "submodule", "update", "--init", "--recursive")
        subprocess.run(["git", "pull", "--rebase"], cwd=cwd, capture_output=True, check=False)
    else:
        git_cmd(cwd, "checkout", "-b", branch)
        git_cmd(cwd, "submodule", "update", "--init", "--recursive")


# --- Template content (create dynamically, no template folder copy) ---
PLATFORM_CONFIG = '''TARGET_CORE:={proc}

# CONFIG_AMP_EN=y
# CONFIG_RTOS_EN=y

# ElAPI configuration
## HAL configuration
# CONFIG_ELAPI_HAL_EN=y
## ElRTOS configuration
# CONFIG_ELAPI_ELRTOS_EN=y
## CLI configuration
# CONFIG_ELAPI_CLI_EN=y
'''

PROJECT_MAKEFILE = '''# Project-specific sources and flags
SRCS+= \\
\t$(PROJECT_PATH)platform_common.c

OBJS+=
LIBS+=

INCFLAGS+= \\
\t-I"$(PROJECT_PATH)include"

DEFFLAGS=
LIBFLAGS=
'''

PLATFORM_COMMON_C = '''/**
 * @file                platform_common.c
 * @brief               Platform initialization and application entry.
 */

#include "platform_config.h"
#ifdef OS_ELRTOS_EN
#include "elrtos.h"
#endif
#include "platform_common.h"

int platform_init(void) {
\treturn 0;
}

int platform_deinit(void) {
\treturn 0;
}

int application_start(void) {
\treturn 0;
}
'''

PLATFORM_CONFIG_H = '''/**
 * @file    platform_config.h
 * @brief   Platform configuration for {project_name}
 */
#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

#endif
'''

# Minimal linker script placeholder - BSP/link_sdk provides real one when using HW tools
LDSCRIPT_LD = '''/* Placeholder - replace with BSP-generated lscript.ld when using hardware tools */
SECTIONS
{
\t.text : {{ *(.text) }}
\t.data : {{ *(.data) }}
\t.bss  : {{ *(.bss) }}
}
'''


def init_workspace(ws_name):
    sw_root = get_script_dir()
    sources = sw_root / "sources"
    ws_dir = sources / ws_name
    common_dir = ws_dir / "common"

    ensure_branch(sw_root, ws_name)
    ws_dir.mkdir(parents=True, exist_ok=True)
    common_dir.mkdir(parents=True, exist_ok=True)
    print(f"init_workspace {ws_name}: created {ws_dir}, {common_dir}")


def reset_workspace(ws_name):
    sw_root = get_script_dir()
    git_cmd(sw_root, "checkout", "main")
    sources = sw_root / "sources"
    ws_dir = sources / ws_name
    if ws_dir.exists():
        shutil.rmtree(ws_dir)
        print(f"reset_workspace {ws_name}: removed {ws_dir}")
    else:
        print(f"reset_workspace {ws_name}: {ws_dir} did not exist")


def create_project_structure(ws_name, project_name, proc="arm0"):
    """Create project folder structure matching template/sdk_prj / cproj structure."""
    sw_root = get_script_dir()
    sources = sw_root / "sources"
    proj_dir = sources / ws_name / project_name
    include_dir = proj_dir / "include"

    proj_dir.mkdir(parents=True, exist_ok=True)
    include_dir.mkdir(parents=True, exist_ok=True)

    (proj_dir / "platform.config").write_text(
        PLATFORM_CONFIG.format(proc=proc), encoding="utf-8"
    )
    (proj_dir / "Makefile").write_text(PROJECT_MAKEFILE, encoding="utf-8")
    (proj_dir / "platform_common.c").write_text(PLATFORM_COMMON_C, encoding="utf-8")
    (include_dir / "platform_config.h").write_text(
        PLATFORM_CONFIG_H.format(project_name=project_name), encoding="utf-8"
    )
    (proj_dir / "ldscript.ld").write_text(LDSCRIPT_LD, encoding="utf-8")

    main_c = sources / ws_name / "main.c"
    if not main_c.exists():
        main_src = sw_root / "sources" / "main.c"
        if (sw_root / "sources" / "main.c").exists():
            shutil.copy(sw_root / "sources" / "main.c", main_c)
        else:
            (main_c).write_text("""int main(void) { return 0; }
""", encoding="utf-8")

    print(f"  Created {proj_dir}")


def init_project(ws_name, project_name, proc="arm0"):
    sw_root = get_script_dir()
    ws_dir = sw_root / "sources" / ws_name
    if not ws_dir.exists():
        print(f"Error: workspace {ws_name} not initialized (run init_workspace first)", file=sys.stderr)
        sys.exit(1)
    root_el = load_xml(ws_name)
    if root_el:
        sw = root_el.find("software")
        if sw:
            for p in sw.findall("project"):
                if p.find("name") is not None and p.find("name").text and p.find("name").text.strip() == project_name:
                    pe = p.find("proc")
                    if pe is not None and pe.text:
                        proc = pe.text.strip()
                    break
    create_project_structure(ws_name, project_name, proc)


def init_prj(ws_name, bsp_filter=None, proj_filter=None):
    """Initialize all projects from XML."""
    root_el = load_xml(ws_name)
    if root_el is None:
        print(f"Error: {ws_name}.xml not found in project root", file=sys.stderr)
        sys.exit(1)

    sw = root_el.find("software")
    if sw is None:
        print("Error: no software section in XML", file=sys.stderr)
        sys.exit(1)

    ws_dir = get_script_dir() / "sources" / ws_name
    if not ws_dir.exists():
        init_workspace(ws_name)

    for proj in sw.findall("project"):
        name_el = proj.find("name")
        if name_el is None or not name_el.text:
            continue
        pname = name_el.text.strip()
        if proj_filter and pname != proj_filter:
            continue
        proc_el = proj.find("proc")
        proc = proc_el.text.strip() if proc_el is not None and proc_el.text else "arm0"
        create_project_structure(ws_name, pname, proc)

    # Generate sources/Makefile
    projects = [p.find("name").text.strip() for p in sw.findall("project") if p.find("name") is not None and p.find("name").text]
    if proj_filter:
        projects = [p for p in projects if p == proj_filter]
    targets = " ".join(projects)
    makefile_content = f"""# Auto-generated by mktarg init_prj - do not edit
PROJECT_NAME := {ws_name}
TARGETS := {targets}
.PHONY: $(PROJECT_NAME) $(addprefix build_,$(TARGETS))
$(PROJECT_NAME): $(addprefix build_,$(TARGETS))
release: $(PROJECT_NAME)
"""
    (get_script_dir() / "sources" / "Makefile").write_text(makefile_content, encoding="utf-8")
    print(f"init_prj {ws_name}: created projects, updated sources/Makefile")


def reset_project(ws_name, project_name):
    sw_root = get_script_dir()
    proj_dir = sw_root / "sources" / ws_name / project_name
    if proj_dir.exists():
        shutil.rmtree(proj_dir)
        print(f"reset_project {ws_name} {project_name}: removed {proj_dir}")
    else:
        print(f"reset_project: {proj_dir} did not exist")


def reset_prj(ws_name):
    root_el = load_xml(ws_name)
    if root_el is None:
        return
    sw = root_el.find("software")
    if sw is None:
        return
    for proj in sw.findall("project"):
        name_el = proj.find("name")
        if name_el is None or not name_el.text:
            continue
        reset_project(ws_name, name_el.text.strip())
    print(f"reset_prj {ws_name}: done")


def regenerate_project(ws_name, project_name):
    reset_project(ws_name, project_name)
    root_el = load_xml(ws_name)
    proc = "arm0"
    if root_el:
        sw = root_el.find("software")
        if sw:
            for p in sw.findall("project"):
                if p.find("name") is not None and p.find("name").text and p.find("name").text.strip() == project_name:
                    pe = p.find("proc")
                    proc = pe.text.strip() if pe is not None and pe.text else "arm0"
                    break
    create_project_structure(ws_name, project_name, proc)
    print(f"regenerate_project {ws_name} {project_name}: done")


def build_project(ws_name, project_name=None, bsp_name=None):
    """Build: prefer hardware tools if available, else software Makefile."""
    sw_root = get_script_dir()
    root_dir = get_root_dir()

    # Try hardware build first (bsp/output/xilinx/elvpn/prj_XXX or hardware/Xilinx)
    hw_xilinx = root_dir / "hardware" / "Xilinx"
    if not hw_xilinx.exists():
        hw_xilinx = root_dir / "hardware" / "xilinx"
    bsp_output = root_dir / "bsp" / "output" / "xilinx" / ws_name
    prj_dirs = list(bsp_output.glob("prj_*")) if bsp_output.exists() else []

    target = project_name
    if target:
        prj_dir = bsp_output / f"prj_{target}"
        if prj_dir.exists():
            subprocess.run(["make", "-C", str(prj_dir)], check=True)
            return
    else:
        # Build all
        for d in prj_dirs:
            subprocess.run(["make", "-C", str(d)], check=True)
        return

    # Fallback: software build via Makefile
    env = os.environ.copy()
    env["PROJECT_NAME"] = ws_name
    subprocess.run(["make", f"build_{target}"], cwd=str(sw_root), env=env, check=True)


def clean_project(ws_name, project_name=None, bsp_name=None):
    sw_root = get_script_dir()
    root_dir = get_root_dir()
    bsp_output = root_dir / "bsp" / "output" / "xilinx" / ws_name

    if project_name:
        prj_dir = bsp_output / f"prj_{project_name}"
        if prj_dir.exists():
            subprocess.run(["make", "-C", str(prj_dir), "clean"], check=False)
    else:
        for d in (bsp_output.glob("prj_*") if bsp_output.exists() else []):
            subprocess.run(["make", "-C", str(d), "clean"], check=False)

    build_dir = sw_root / "build"
    if build_dir.exists():
        shutil.rmtree(build_dir)
    print(f"clean_project {ws_name}: done")


def clean_workspace(ws_name):
    sw_root = get_script_dir()
    build_dir = sw_root / "build"
    if build_dir.exists():
        shutil.rmtree(build_dir)
    print(f"clean_workspace {ws_name}: done")


def clean_all(ws_name):
    clean_workspace(ws_name)
    print(f"clean_all {ws_name}: done")


def clean_all_workspace(ws_name):
    clean_all(ws_name)


def main():
    parser = argparse.ArgumentParser(description="software mktarg")
    parser.add_argument("function", help="Command")
    parser.add_argument("ws_name", nargs="?", help="Workspace name")
    parser.add_argument("project_name", nargs="?", help="Project name (for project-specific commands)")
    parser.add_argument("extra", nargs="*")
    parser.add_argument("--bsp", dest="bsp_filter")
    parser.add_argument("--project", dest="proj_filter")
    args = parser.parse_args()

    func = args.function.replace("-", "_")
    ws = args.ws_name
    proj = args.project_name

    handlers = {
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

    if func not in handlers:
        print(f"Unknown: {args.function}", file=sys.stderr)
        sys.exit(1)
    if not ws and func not in ("clean_all_workspace",):
        print("ws_name required", file=sys.stderr)
        sys.exit(1)

    handlers[func]()


if __name__ == "__main__":
    main()
