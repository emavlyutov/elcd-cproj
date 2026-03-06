"""
Platform config generation from target.xml and submodules.xml.
"""
import sys

from paths import get_sources_dir
from xml_parse import (
    get_target_core,
    get_target_toolchain,
    load_submodules_xml,
    _collect_configs_ordered,
    find_submodule_by_name_or_path,
    parse_submodules_config,
    find_by_path_in_list,
)

# --- Template content ---
PLATFORM_CONFIG = """TARGET_CORE:={proc}

# CONFIG_AMP_EN=y
# CONFIG_RTOS_EN=y

# ElAPI configuration
## HAL configuration
# CONFIG_ELAPI_HAL_EN=y
## ElRTOS configuration
# CONFIG_ELAPI_ELRTOS_EN=y
## CLI configuration
# CONFIG_ELAPI_CLI_EN=y
"""

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

PROJECT_MAKEFILE = '''# '{project_name}' project
SRCS+= \\
\t$(PROJECT_PATH)platform_common.c

OBJS+=
LIBS+=

INCFLAGS+= \\
\t-I"$(PROJECT_PATH)include"

DEFFLAGS+=
LIBFLAGS+=
'''


def generate_platform_config_content(target_name=None, core=None):
    """
    Generate platform.config from target.xml + submodules.xml.
    Style: TARGET/SUBMODULES/PROJECT CONFIGURATION separators, comments from XML only, CRLF line endings.
    """
    if target_name:
        core = get_target_core(target_name)
        toolchain = get_target_toolchain(target_name)
    elif core is None:
        core = "arm0"
        toolchain = "gcc_zynqps.mk"
    else:
        toolchain = "gcc_zynqps.mk"
    lines = [
        "################################## TARGET CONFIGURATION ##################################",
        f"TARGET_CORE:={core}",
        f'TARGET_TOOLCHAIN:="{toolchain}"',
        "",
        "################################## SUBMODULES CONFIGURATION ##################################",
        "",
    ]
    root = load_submodules_xml()
    if root is not None:
        root_comment = root.find("comment")
        sect = (root_comment.text or "").strip() if root_comment is not None and root_comment.text else ""
        if sect:
            lines.append(f"## {sect}")
            lines.append("")
        for sub in root.findall("submodule"):
            path_el = sub.find("path")
            sub_path = (path_el.text or "").strip() if path_el is not None else ""
            comment = sub.find("comment")
            sect = (comment.text or "").strip() if comment is not None and comment.text else ""
            if "thirdparty" in sub_path:
                if sect:
                    lines.append(f"# {sect}")
                for txt in _collect_configs_ordered(sub):
                    lines.append(f"# {txt}=y")
                lines.append("")
            else:
                if sect:
                    lines.append(f"## {sect}")
                    lines.append("")
                for csub in sub.findall("submodule"):
                    ccomment = csub.find("comment")
                    nn = csub.find("name")
                    if nn is None:
                        nn = csub.find("path")
                    bloc = (ccomment.text if ccomment is not None and ccomment.text else (nn.text if nn is not None else "")) or ""
                    if bloc:
                        lines.append(f"## {bloc}")
                    for txt in _collect_configs_ordered(csub):
                        lines.append(f"# {txt}=y")
                    lines.append("")
    lines.append("################################## PROJECT CONFIGURATION ##################################")
    return "\r\n".join(lines) + "\r\n"


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

    def collect_configs_enabled(node):
        for enabled, c in node["configs"]:
            if enabled:
                configs_to_uncomment.add(c)
        parent_path = node.get("parent_path", "")
        if parent_path:
            parent = find_by_path_in_list(list(parse_submodules_config()), parent_path)
            if parent:
                collect_configs_enabled(parent)

    collect_configs_enabled(sub)

    proj_dir = get_sources_dir() / ws_name / prj_name
    cfg_path = proj_dir / "platform.config"
    if not cfg_path.exists():
        print(f"update_prj_platform_config: {cfg_path} not found, skipping", file=sys.stderr)
        return
    content = cfg_path.read_text(encoding="utf-8")
    for cfg in configs_to_uncomment:
        commented = f"# {cfg}=y"
        uncommented = f"{cfg}=y"
        content = content.replace(commented, uncommented)
    cfg_path.write_text(content, encoding="utf-8")
    print(f"update_prj_platform_config: uncommented {len(configs_to_uncomment)} configs for {prj_name}")
