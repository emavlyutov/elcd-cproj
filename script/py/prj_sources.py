"""
Project source generation: platform.config, platform_common.c, Makefile, etc.
"""
from paths import get_project_root, get_sources_dir, ensure_sources_dir
from platform_config import (
    generate_platform_config_content,
    PLATFORM_COMMON_C,
    PLATFORM_CONFIG_H,
    PROJECT_MAKEFILE,
    PLATFORM_CONFIG,
)


def generate_prj_sources(ws_name, prj_name, target_name="ZynqPS0"):
    """Create project structure: sources/WS_NAME/PRJ_NAME with platform.config from target.xml + submodules.xml."""
    sources = get_sources_dir()
    proj_dir = sources / ws_name / prj_name
    include_dir = proj_dir / "include"

    proj_dir.mkdir(parents=True, exist_ok=True)
    include_dir.mkdir(parents=True, exist_ok=True)

    platform_content = generate_platform_config_content(target_name)
    (proj_dir / "platform.config").write_bytes(platform_content.encode("utf-8"))
    (proj_dir / "platform_common.c").write_text(PLATFORM_COMMON_C, encoding="utf-8")
    (include_dir / "platform_config.h").write_text(PLATFORM_CONFIG_H.format(project_name=prj_name), encoding="utf-8")
    (proj_dir / "Makefile").write_text(PROJECT_MAKEFILE.format(project_name=prj_name), encoding="utf-8")

    print(f"generate_prj_sources: created {proj_dir}")


def create_project_structure(ws_name, project_name, proc="arm0", target_name=None):
    """Create project folder structure. Uses target.xml+submodules.xml if target_name set, else proc."""
    from paths import get_script_xml_dir

    sources = ensure_sources_dir()
    ws_dir = sources / ws_name
    if not ws_dir.exists():
        raise FileNotFoundError(f"Workspace directory does not exist: {ws_dir}")
    proj_dir = sources / ws_name / project_name
    include_dir = proj_dir / "include"

    proj_dir.mkdir(parents=True, exist_ok=True)
    include_dir.mkdir(parents=True, exist_ok=True)

    target_xml = get_script_xml_dir() / "target.xml"
    submodules_xml = get_script_xml_dir() / "submodules.xml"
    if target_xml.exists() and submodules_xml.exists():
        platform_content = generate_platform_config_content(target_name=target_name, core=proc)
    else:
        platform_content = PLATFORM_CONFIG.format(proc=proc)
    (proj_dir / "platform.config").write_bytes(platform_content.encode("utf-8"))
    (proj_dir / "platform_common.c").write_text(PLATFORM_COMMON_C, encoding="utf-8")
    (include_dir / "platform_config.h").write_text(
        PLATFORM_CONFIG_H.format(project_name=project_name), encoding="utf-8"
    )
    (proj_dir / "Makefile").write_text(PROJECT_MAKEFILE.format(project_name=project_name), encoding="utf-8")

    print(f"  Created {proj_dir}")
