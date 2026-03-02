#!/usr/bin/env python3
"""
script/py/mktarg.py - Workspace and project management for software repo
Usage: python mktarg.py <function> [WS_NAME] [PROJECT_NAME] [args...]
Functions: ws_init, ws_close, prj_init, prj_add_submodule,
           init_workspace, reset_workspace, init_project, reset_project,
           regenerate_project, build_project, clean_project,
           clean_workspace, clean_all, clean_all_workspace,
           init_prj, reset_prj (batch init/reset from XML)
"""

import argparse
import os
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


# --- Exit codes for special conditions ---
EXIT_ALREADY_INIT = 2
EXIT_NOINIT = 3
EXIT_NOTFOUND = 4


def get_script_dir():
    """Directory containing this script (script/py/)."""
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


def load_xml(ws_name):
    """Load [WS_NAME].xml from project root."""
    xml_path = get_project_root() / f"{ws_name}.xml"
    if not xml_path.exists():
        return None
    return ET.parse(xml_path).getroot()


def load_target_xml():
    """Load script/xml/target.xml. File may have multiple <target> roots."""
    path = get_script_xml_dir() / "target.xml"
    if not path.exists():
        return None
    content = path.read_text(encoding="utf-8")
    wrapped = f"<targets>{content}</targets>"
    return ET.fromstring(wrapped)


def load_submodules_xml():
    """Load script/xml/submodules.xml."""
    path = get_script_xml_dir() / "submodules.xml"
    if not path.exists():
        return None
    return ET.parse(path).getroot()


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
    """Return True if path is a git submodule."""
    r = subprocess.run(["git", "submodule", "status", "--cached", path], cwd=cwd, capture_output=True, text=True)
    return r.returncode == 0 and path in (r.stdout or "")


def git_commit(cwd, message, *paths):
    """Stage and commit paths. If no paths, stage all."""
    if paths:
        subprocess.run(["git", "add"] + list(paths), cwd=cwd, check=True)
    else:
        subprocess.run(["git", "add", "-A"], cwd=cwd, check=True)
    r = subprocess.run(["git", "status", "--short"], cwd=cwd, capture_output=True, text=True)
    if r.stdout.strip():
        subprocess.run(["git", "commit", "-m", message], cwd=cwd, check=True)
        return True
    return False


def ensure_branch(cwd, branch):
    r = subprocess.run(["git", "branch", "-a"], cwd=cwd, capture_output=True, text=True)
    has_branch = branch in r.stdout or f"remotes/origin/{branch}" in r.stdout
    subprocess.run(["git", "fetch"], cwd=cwd, capture_output=True)
    if has_branch:
        subprocess.run(["git", "switch", branch], cwd=cwd, check=True)
        subprocess.run(["git", "submodule", "update", "--init", "--recursive"], cwd=cwd, check=True)
        subprocess.run(["git", "pull", "--rebase"], cwd=cwd, capture_output=True, check=False)
    else:
        subprocess.run(["git", "checkout", "-b", branch], cwd=cwd, check=True)
        subprocess.run(["git", "submodule", "update", "--init", "--recursive"], cwd=cwd, check=True)


def get_target_core(target_name):
    """Map TARGET name to core from target.xml. Default arm0."""
    root = load_target_xml()
    if root is None:
        return "arm0"
    for t in root.findall("target"):
        name_el = t.find("name")
        if name_el is not None and name_el.text and name_el.text.strip() == target_name:
            core_el = t.find("core")
            if core_el is not None and core_el.text:
                return core_el.text.strip()
    return "arm0"


def parse_submodules_config(parent_path="sources/shared"):
    """
    Recursively yield submodule info from submodules.xml.
    Each item: dict with path, name, git, configs, submodules (children), parent_path.
    path appends recursively: e.g. sources/shared/elAPI/elHAL.
    """
    root = load_submodules_xml()
    if root is None:
        return

    def _walk(el, base_path):
        path_el = el.find("path")
        name_el = el.find("name")
        seg = ((path_el.text if path_el is not None else None) or (name_el.text if name_el is not None else None) or "").strip()
        if not seg:
            return None
        full_path = f"{base_path}/{seg}".replace("//", "/").strip("/") if base_path else seg
        git_el = el.find("git")
        git_url = (git_el.text or "").strip() if git_el is not None else ""
        configs = [(c.get("enabled") == "y", (c.text or "").strip()) for c in el.findall("config") if (c.text or "").strip()]
        info = {"path": full_path, "name": seg, "git": git_url, "configs": configs, "parent_path": base_path, "submodules": []}
        for sub in el.findall("submodule"):
            child = _walk(sub, full_path)
            if child:
                info["submodules"].append(child)
        return info

    for sub in root.findall("submodule"):
        item = _walk(sub, parent_path)
        if item:
            yield item


def find_submodule_by_name_or_path(name_or_path):
    """Find submodule dict by name or path. Returns None if not found."""
    def search(items):
        for item in items:
            if item["name"] == name_or_path or item["path"] == name_or_path:
                return item
            found = search(item.get("submodules", []))
            if found:
                return found
        return None
    return search(list(parse_submodules_config()))


def _collect_all_config_lines(node, lines, comment_prefix="# "):
    """Recursively collect config lines from submodule node into lines list."""
    for c in node.get("configs", []):
        enabled, txt = c
        line = f"{txt}=y" if txt.endswith("_EN") or txt.endswith("_EN") else f"{txt}=y"
        if not txt.endswith("_EN") and not txt.endswith("_EN"):
            line = f"{txt}=y"
        lines.append((f"{comment_prefix}{line}", line))
    for child in node.get("submodules", []):
        _collect_all_config_lines(child, lines, comment_prefix)


def generate_platform_config_content(target_name):
    """Generate platform.config content from target.xml + submodules.xml."""
    core = get_target_core(target_name)
    lines = [
        "################################## TARGET_CONFIGURATION ##################################",
        f"TARGET_CORE:={core}",
        "",
        "################################## TARGET_CONFIGURATION ##################################",
    ]
    root = load_submodules_xml()
    if root is not None:
        for sub in root.findall("submodule"):
            comment = sub.find("comment")
            sect = (comment.text or "").strip() if comment is not None and comment.text else "configuration"
            lines.append(f"# {sect}")
            for c in sub.findall("config"):
                txt = (c.text or "").strip()
                if txt:
                    lines.append(f"# {txt}=y")
            for csub in sub.findall("submodule"):
                ccomment = csub.find("comment")
                nn = csub.find("name")
                nn = nn if nn is not None else csub.find("path")
                subname = ((ccomment.text or "") if ccomment is not None else "") or (nn.text if nn is not None else "") or "config"
                subname = subname.strip() or "config"
                lines.append(f"## {subname}")
                for cc in csub.findall("config"):
                    txt = (cc.text or "").strip()
                    if txt:
                        lines.append(f"# {txt}=y")
                for gc in csub.findall("submodule"):
                    gcn = gc.find("name")
                    gcn = gcn if gcn is not None else gc.find("path")
                    gcname = (gcn.text if gcn is not None else "").strip() or "config"
                    lines.append(f"## {gcname}")
                    for gcc in gc.findall("config"):
                        txt = (gcc.text or "").strip()
                        if txt:
                            lines.append(f"# {txt}=y")
    lines.append("")
    lines.append("################################## PROJECT_CONFIGURATION ##################################")
    return "\n".join(lines)


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


def ws_init(ws_name, submodule_url=None):
    """
    Initialize workspace: switch/create branch, add submodule for sources/WS_NAME, create common.
    Returns ALREADY_INIT exit if submodule already exists.
    """
    root = get_project_root()
    sources = get_sources_dir()
    ws_path = f"sources/{ws_name}"

    if git_branch_exists(root, ws_name):
        subprocess.run(["git", "switch", ws_name], cwd=root, check=True)
        subprocess.run(["git", "submodule", "update", "--init", "--recursive"], cwd=root, check=True)
    else:
        subprocess.run(["git", "checkout", "-b", ws_name], cwd=root, check=True)

    if git_submodule_exists(root, ws_path):
        print(f"ws_init: {ws_name} already initialized (submodule exists)")
        sys.exit(EXIT_ALREADY_INIT)

    url = submodule_url or f"git@github.com:example/{ws_name}.git"
    subprocess.run(["git", "submodule", "add", url, ws_path], cwd=root, check=True)
    subprocess.run(["git", "submodule", "update", "--init", "--recursive"], cwd=root, check=True)

    ws_dir = sources / ws_name
    common_dir = ws_dir / "common"
    ws_dir.mkdir(parents=True, exist_ok=True)
    common_dir.mkdir(parents=True, exist_ok=True)

    if git_commit(root, f"ws_init: add workspace {ws_name}"):
        print(f"Committed workspace {ws_name}")
    print(f"ws_init {ws_name}: done")


def ws_close():
    """Close workspace: switch to main, remove tracked files."""
    root = get_project_root()
    subprocess.run(["git", "switch", "main"], cwd=root, check=True)
    subprocess.run(["git", "rm", "-rf", "."], cwd=root, capture_output=True, check=False)
    print("ws_close: switched to main")


def update_prj_platform_config(ws_name, prj_name, submodule_name_or_path):
    """
    Uncomment config lines in platform.config for submodule and its parent chain recursively.
    submodule_name_or_path: name (e.g. elHAL) or path (e.g. sources/shared/elAPI/elHAL).
    """
    sub = find_submodule_by_name_or_path(submodule_name_or_path)
    if not sub:
        print(f"update_prj_platform_config: submodule {submodule_name_or_path} not found", file=sys.stderr)
        return
    configs_to_uncomment = set()

    def collect_configs(node):
        for _, c in node["configs"]:
            configs_to_uncomment.add(c)
        parent_path = node.get("parent_path", "")
        if parent_path:
            parent = _find_by_path_in_list(list(parse_submodules_config()), parent_path)
            if parent:
                collect_configs(parent)

    collect_configs(sub)

    proj_dir = get_sources_dir() / ws_name / prj_name
    cfg_path = proj_dir / "platform.config"
    if not cfg_path.exists():
        return
    content = cfg_path.read_text(encoding="utf-8")
    for cfg in configs_to_uncomment:
        commented = f"# {cfg}=y"
        uncommented = f"{cfg}=y"
        content = content.replace(commented, uncommented)
    cfg_path.write_text(content, encoding="utf-8")
    print(f"update_prj_platform_config: uncommented {len(configs_to_uncomment)} configs for {prj_name}")


def _find_by_path_in_list(nodes, path):
    for n in nodes:
        if n["path"] == path:
            return n
        found = _find_by_path_in_list(n.get("submodules", []), path)
        if found:
            return found
    return None


def generate_prj_sources(ws_name, prj_name, target_name="ZynqPS0"):
    """Create project structure: sources/WS_NAME/PRJ_NAME with platform.config from target.xml + submodules.xml."""
    sources = get_sources_dir()
    proj_dir = sources / ws_name / prj_name
    bsp_dir = proj_dir / "bsp"
    include_dir = proj_dir / "include"

    proj_dir.mkdir(parents=True, exist_ok=True)
    bsp_dir.mkdir(parents=True, exist_ok=True)
    include_dir.mkdir(parents=True, exist_ok=True)

    platform_content = generate_platform_config_content(target_name)
    (proj_dir / "platform.config").write_text(platform_content, encoding="utf-8")
    (proj_dir / "platform_common.c").write_text(PLATFORM_COMMON_C, encoding="utf-8")
    (include_dir / "platform_config.h").write_text(PLATFORM_CONFIG_H.format(project_name=prj_name), encoding="utf-8")
    (proj_dir / "ldscript.ld").write_text(LDSCRIPT_LD, encoding="utf-8")

    main_c = sources / ws_name / "main.c"
    if not main_c.exists():
        root_main = get_project_root() / "sources" / "main.c"
        if root_main.exists():
            main_c.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(root_main, main_c)
        else:
            main_c.parent.mkdir(parents=True, exist_ok=True)
            main_c.write_text("int main(void) { return 0; }\n", encoding="utf-8")

    print(f"generate_prj_sources: created {proj_dir}")


def prj_init(prj_name, target_name="ZynqPS0"):
    """
    Initialize project. WS_NAME = current branch.
    Returns NOINIT if on main or sources/WS_NAME submodule not present.
    """
    root = get_project_root()
    ws_name = git_current_branch(root)
    if ws_name == "main":
        print("prj_init: cannot run on main branch", file=sys.stderr)
        sys.exit(EXIT_NOINIT)
    ws_path = f"sources/{ws_name}"
    if not git_submodule_exists(root, ws_path):
        ws_dir = get_sources_dir() / ws_name
        if not ws_dir.exists():
            print("prj_init: workspace not initialized (run ws_init first)", file=sys.stderr)
            sys.exit(EXIT_NOINIT)
    generate_prj_sources(ws_name, prj_name, target_name)
    if git_commit(root, f"prj_init: add project {prj_name}"):
        print(f"Committed project {prj_name}")


def prj_add_submodule(prj_name, submodule_name_or_path):
    """
    Add submodule to project: git submodule add if needed, then update platform.config.
    Returns NOTFOUND if submodule not in submodules.xml.
    """
    root = get_project_root()
    ws_name = git_current_branch(root)
    sub = find_submodule_by_name_or_path(submodule_name_or_path)
    if not sub:
        print(f"prj_add_submodule: submodule {submodule_name_or_path} not found", file=sys.stderr)
        sys.exit(EXIT_NOTFOUND)
    sub_path = sub["path"]
    if not git_submodule_exists(root, sub_path) and sub.get("git"):
        subprocess.run(["git", "submodule", "add", sub["git"], sub_path], cwd=root, check=True)
        subprocess.run(["git", "submodule", "update", "--init", "--recursive"], cwd=root, check=True)
    update_prj_platform_config(ws_name, prj_name, submodule_name_or_path)
    if git_commit(root, f"prj_add_submodule: add {submodule_name_or_path} to {prj_name}"):
        print(f"Committed submodule {submodule_name_or_path}")


def init_workspace(ws_name):
    root = get_project_root()
    sources = get_sources_dir()
    ws_dir = sources / ws_name
    common_dir = ws_dir / "common"

    ensure_branch(root, ws_name)
    ws_dir.mkdir(parents=True, exist_ok=True)
    common_dir.mkdir(parents=True, exist_ok=True)
    print(f"init_workspace {ws_name}: created {ws_dir}, {common_dir}")


def reset_workspace(ws_name):
    root = get_project_root()
    subprocess.run(["git", "switch", "main"], cwd=root, check=True)
    sources = get_sources_dir()
    ws_dir = sources / ws_name
    if ws_dir.exists():
        shutil.rmtree(ws_dir)
        print(f"reset_workspace {ws_name}: removed {ws_dir}")
    else:
        print(f"reset_workspace {ws_name}: {ws_dir} did not exist")


def create_project_structure(ws_name, project_name, proc="arm0", target_name=None):
    """Create project folder structure. Uses target.xml+submodules.xml if target_name set, else proc."""
    sources = get_sources_dir()
    proj_dir = sources / ws_name / project_name
    include_dir = proj_dir / "include"
    bsp_dir = proj_dir / "bsp"

    proj_dir.mkdir(parents=True, exist_ok=True)
    include_dir.mkdir(parents=True, exist_ok=True)
    bsp_dir.mkdir(parents=True, exist_ok=True)

    if target_name:
        platform_content = generate_platform_config_content(target_name)
    else:
        platform_content = PLATFORM_CONFIG.format(proc=proc)
    (proj_dir / "platform.config").write_text(platform_content, encoding="utf-8")
    (proj_dir / "platform_common.c").write_text(PLATFORM_COMMON_C, encoding="utf-8")
    (include_dir / "platform_config.h").write_text(
        PLATFORM_CONFIG_H.format(project_name=project_name), encoding="utf-8"
    )
    (proj_dir / "ldscript.ld").write_text(LDSCRIPT_LD, encoding="utf-8")

    main_c = sources / ws_name / "main.c"
    if not main_c.exists():
        root_main = get_project_root() / "sources" / "main.c"
        if root_main.exists():
            main_c.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(root_main, main_c)
        else:
            main_c.parent.mkdir(parents=True, exist_ok=True)
            main_c.write_text("int main(void) { return 0; }\n", encoding="utf-8")

    print(f"  Created {proj_dir}")


def init_project(ws_name, project_name, proc="arm0", target_name=None):
    ws_dir = get_sources_dir() / ws_name
    if not ws_dir.exists():
        print(f"Error: workspace {ws_name} not initialized (run init_workspace first)", file=sys.stderr)
        sys.exit(1)
    target_name = None
    root_el = load_xml(ws_name)
    if root_el:
        sw = root_el.find("software")
        if sw:
            for p in sw.findall("project"):
                if p.find("name") is not None and p.find("name").text and p.find("name").text.strip() == project_name:
                    pe = p.find("proc")
                    if pe is not None and pe.text:
                        proc = pe.text.strip()
                    te = p.find("target")
                    if te is not None and te.text:
                        target_name = te.text.strip()
                    break
    create_project_structure(ws_name, project_name, proc, target_name)


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

    ws_dir = get_sources_dir() / ws_name
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
        target_el = proj.find("target")
        target_name = target_el.text.strip() if target_el is not None and target_el.text else None
        create_project_structure(ws_name, pname, proc, target_name)

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
    (get_sources_dir() / "Makefile").write_text(makefile_content, encoding="utf-8")
    print(f"init_prj {ws_name}: created projects, updated sources/Makefile")


def reset_project(ws_name, project_name):
    proj_dir = get_sources_dir() / ws_name / project_name
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
    target_name = None
    if root_el:
        sw = root_el.find("software")
        if sw:
            for p in sw.findall("project"):
                if p.find("name") is not None and p.find("name").text and p.find("name").text.strip() == project_name:
                    pe = p.find("proc")
                    proc = pe.text.strip() if pe is not None and pe.text else "arm0"
                    te = p.find("target")
                    if te is not None and te.text:
                        target_name = te.text.strip()
                    break
    create_project_structure(ws_name, project_name, proc, target_name)
    print(f"regenerate_project {ws_name} {project_name}: done")


def build_project(ws_name, project_name=None, bsp_name=None):
    """Build: prefer hardware tools if available, else software Makefile."""
    root_dir = get_project_root()

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
    make_dir = get_sources_dir()
    env = os.environ.copy()
    env["PROJECT_NAME"] = ws_name
    subprocess.run(["make", f"build_{target}"], cwd=str(make_dir), env=env, check=True)


def clean_project(ws_name, project_name=None, bsp_name=None):
    root_dir = get_project_root()
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
    build_dir = get_sources_dir() / "build"
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
    parser.add_argument("--url", dest="submodule_url", help="Submodule URL for ws_init")
    args = parser.parse_args()

    func = args.function.replace("-", "_")
    ws = args.ws_name
    proj = args.project_name

    handlers = {
        "ws_init": lambda: ws_init(ws, args.submodule_url),
        "ws_close": lambda: ws_close(),
        "prj_init": lambda: prj_init(ws or proj or (args.extra[0] if args.extra else ""), proj if (ws and proj) else (args.extra[0] if args.extra else "ZynqPS0")),
        "prj_add_submodule": lambda: prj_add_submodule(ws or proj or (args.extra[0] if args.extra else ""), proj if (ws and proj) else (args.extra[0] if args.extra else "")),
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
    if not ws and func not in ("clean_all_workspace", "ws_close", "prj_init", "prj_add_submodule"):
        if func in ("prj_init", "prj_add_submodule") and (proj or args.extra):
            pass
        else:
            print("ws_name required", file=sys.stderr)
            sys.exit(1)

    handlers[func]()


if __name__ == "__main__":
    main()
