"""
Project commands: prj_init, prj_add_submodule, prj_remove_submodule, etc.
"""
import shutil
import subprocess
import sys

from paths import get_project_root, get_sources_dir, ensure_git_repo
from git_ops import git_submodule_exists, find_workspace_for_project
from xml_parse import find_submodule_by_name_or_path, parse_submodules_config, find_by_path_in_list
from platform_config import update_prj_platform_config
from prj_sources import generate_prj_sources, create_project_structure
from xml_parse import load_xml, is_valid_target, get_valid_targets


from checks import (
    EXIT_NOINIT,
    EXIT_NOTFOUND,
    EXIT_INVALID_TARGET,
    require_valid_ws_name,
    require_valid_prj_name,
    require,
)


def prj_init(ws_name, prj_name, target_name="ZynqPS0"):
    """
    Initialize project. WS_NAME required (first arg) when not on workspace branch.
    Returns NOINIT if sources/WS_NAME not present.
    """
    require_valid_ws_name(ws_name)
    require_valid_prj_name(prj_name)
    root = ensure_git_repo()
    ws_path = f"sources/{ws_name}"
    ws_dir = get_sources_dir() / ws_name
    if not ws_dir.exists() and not git_submodule_exists(root, ws_path):
        print("prj_init: workspace not initialized (run ws_init first)", file=sys.stderr)
        sys.exit(EXIT_NOINIT)
    require(is_valid_target(target_name),
            f"prj_init: invalid target '{target_name}' (valid: {', '.join(get_valid_targets())})",
            EXIT_INVALID_TARGET)
    generate_prj_sources(ws_name, prj_name, target_name)


def prj_add_submodule(ws_name, prj_name, submodule_name_or_path):
    """
    Add submodule to project: git submodule add if needed, then update platform.config.
    WS_NAME required (first arg). Returns NOTFOUND if submodule not in submodules.xml.
    """
    from checks import require_git_ok

    require_valid_ws_name(ws_name)
    require_valid_prj_name(prj_name)
    require(submodule_name_or_path and str(submodule_name_or_path).strip(),
            "prj_add_submodule: submodule name required (e.g. elHAL, elCli)",
            EXIT_NOTFOUND)
    root = ensure_git_repo()
    sub = find_submodule_by_name_or_path(submodule_name_or_path)
    if not sub:
        print(f"prj_add_submodule: submodule {submodule_name_or_path} not found in submodules.xml", file=sys.stderr)
        sys.exit(EXIT_NOTFOUND)
    sub_path = sub["path"]
    git_add_all = True
    if not git_submodule_exists(root, sub_path) and sub.get("git"):
        r = subprocess.run(
            ["git", "submodule", "add", sub["git"], sub_path],
            cwd=root, capture_output=True, text=True
        )
        err_text = (r.stderr or r.stdout or "")
        retry_clean = (
            r.returncode != 0
            and (
                "--force" in err_text
                or "reuse" in err_text.lower()
                or "does not have a commit checked out" in err_text
                or "branch yet to be born" in err_text
            )
        )
        if r.returncode != 0:
            if retry_clean:
                sub_path_abs = root / sub_path
                modules_dir = root / ".git" / "modules" / sub_path.replace("\\", "/")
                for d in (sub_path_abs, modules_dir):
                    if d.exists():
                        try:
                            shutil.rmtree(d)
                        except OSError:
                            pass
                subprocess.run(["git", "rm", "--cached", "-f", sub_path], cwd=root, capture_output=True, check=False)
                subprocess.run(["git", "config", "--remove-section", f"submodule.{sub_path}"], cwd=root, capture_output=True, check=False)
                r = subprocess.run(
                    ["git", "submodule", "add", sub["git"], sub_path],
                    cwd=root, capture_output=True, text=True
                )
            if r.returncode != 0:
                print(f"prj_add_submodule: git submodule add failed (repo may be invalid): {err_text}", file=sys.stderr)
                print("  Continuing to update platform.config - ensure sources exist at", sub_path, file=sys.stderr)
                sub_path_abs = root / sub_path
                if sub_path_abs.exists():
                    try:
                        shutil.rmtree(sub_path_abs)
                        subprocess.run(["git", "rm", "--cached", "-f", sub_path], cwd=root, capture_output=True, check=False)
                    except OSError:
                        pass
                git_add_all = False
            else:
                r2 = subprocess.run(["git", "submodule", "update", "--init", "--recursive"], cwd=root, capture_output=True, text=True)
                if r2.returncode != 0:
                    require_git_ok(r2, "submodule update", root)
        else:
            r2 = subprocess.run(["git", "submodule", "update", "--init", "--recursive"], cwd=root, capture_output=True, text=True)
            if r2.returncode != 0:
                require_git_ok(r2, "submodule update", root)
    update_prj_platform_config(ws_name, prj_name, submodule_name_or_path)
    msg = f"prj_add_submodule: add {sub['name']} to {prj_name}"
    cfg_path = get_sources_dir() / ws_name / prj_name / "platform.config"
    if cfg_path.exists():
        print(f"platform.config updated for {sub['name']}")
    elif not git_add_all:
        print(f"Error: {cfg_path} not found (wrong ws/prj?)", file=sys.stderr)


def prj_remove_submodule(ws_name, prj_name, submodule_name_or_path):
    """
    Remove submodule from project: comment config lines in platform.config.
    Does not remove the git submodule (shared across projects).
    """
    require_valid_ws_name(ws_name)
    require_valid_prj_name(prj_name)
    require(submodule_name_or_path and str(submodule_name_or_path).strip(),
            "prj_remove_submodule: submodule name required (e.g. elHAL, elCli)",
            EXIT_NOTFOUND)
    sub = find_submodule_by_name_or_path(submodule_name_or_path)
    if not sub:
        print(f"prj_remove_submodule: submodule {submodule_name_or_path} not found in submodules.xml", file=sys.stderr)
        sys.exit(EXIT_NOTFOUND)
    configs_to_comment = set()

    def collect_configs_enabled(node):
        for enabled, c in node["configs"]:
            if enabled:
                configs_to_comment.add(c)
        parent_path = node.get("parent_path", "")
        if parent_path:
            parent = find_by_path_in_list(list(parse_submodules_config()), parent_path)
            if parent:
                collect_configs_enabled(parent)

    collect_configs_enabled(sub)
    proj_dir = get_sources_dir() / ws_name / prj_name
    cfg_path = proj_dir / "platform.config"
    if not cfg_path.exists():
        print(f"prj_remove_submodule: {cfg_path} not found", file=sys.stderr)
        return
    content = cfg_path.read_text(encoding="utf-8")
    uncommented_set = {f"{c}=y" for c in configs_to_comment}
    lines = content.splitlines(keepends=True)
    new_lines = []
    for line in lines:
        stripped = line.strip().rstrip("\r\n")
        if stripped in uncommented_set:
            indent = line[: len(line) - len(line.lstrip())]
            new_lines.append(f"{indent}# {stripped}\n")
        elif stripped.startswith("# # ") and stripped[4:].rstrip() in uncommented_set:
            new_lines.append(line.replace("# # ", "# ", 1))
        else:
            new_lines.append(line)
    cfg_path.write_text("".join(new_lines), encoding="utf-8")
    print(f"prj_remove_submodule: commented {len(configs_to_comment)} configs for {prj_name}")


def init_project(ws_name, project_name, proc="arm0", target_name=None):
    require_valid_ws_name(ws_name)
    require_valid_prj_name(project_name)
    ws_dir = get_sources_dir() / ws_name
    require(ws_dir.exists(), f"Workspace {ws_name} not initialized (run init_workspace first)")
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
    require_valid_ws_name(ws_name)
    root_el = load_xml(ws_name, required=True)

    sw = root_el.find("software")
    require(sw is not None, f"No <software> section in {ws_name}.xml")

    ws_dir = get_sources_dir() / ws_name
    if not ws_dir.exists():
        from workspace import init_workspace
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

    print(f"init_prj {ws_name}: created projects")


def prj_remove(ws_name, prj_name):
    """Remove project directory (alias for reset_project). WS_NAME required."""
    require_valid_ws_name(ws_name)
    require_valid_prj_name(prj_name)
    reset_project(ws_name, prj_name)


def prj_close(ws_name, prj_name):
    """Remove project folder. Same as prj_remove."""
    prj_remove(ws_name, prj_name)


def prj_regenerate(ws_name, prj_name):
    """Remove and recreate project. WS_NAME required."""
    require_valid_ws_name(ws_name)
    require_valid_prj_name(prj_name)
    regenerate_project(ws_name, prj_name)


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
