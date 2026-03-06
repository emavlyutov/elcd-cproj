"""
XML parsing: target.xml, submodules.xml, workspace XML.
"""
import sys
import xml.etree.ElementTree as ET

from paths import get_project_root, get_script_xml_dir


def load_xml(ws_name, required=False):
    """Load [WS_NAME].xml from project root. Return None if not found, unless required=True."""
    xml_path = get_project_root() / f"{ws_name}.xml"
    if not xml_path.exists():
        if required:
            print(f"Error: {ws_name}.xml not found in project root", file=sys.stderr)
            sys.exit(1)
        return None
    try:
        return ET.parse(xml_path).getroot()
    except ET.ParseError as e:
        print(f"Error: failed to parse {xml_path}: {e}", file=sys.stderr)
        sys.exit(1)


def load_target_xml():
    """Load script/xml/target.xml. File may have multiple <target> roots."""
    path = get_script_xml_dir() / "target.xml"
    if not path.exists():
        return None
    try:
        content = path.read_text(encoding="utf-8")
        wrapped = f"<targets>{content}</targets>"
        return ET.fromstring(wrapped)
    except ET.ParseError as e:
        print(f"Error: failed to parse {path}: {e}", file=sys.stderr)
        sys.exit(1)


def load_submodules_xml():
    """Load script/xml/submodules.xml."""
    path = get_script_xml_dir() / "submodules.xml"
    if not path.exists():
        return None
    try:
        return ET.parse(path).getroot()
    except ET.ParseError as e:
        print(f"Error: failed to parse {path}: {e}", file=sys.stderr)
        sys.exit(1)


def get_software_projects(root_el):
    """Yield (name,) for each software/project."""
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


def get_valid_targets():
    """Return list of valid target names from target.xml."""
    root = load_target_xml()
    if root is None:
        return []
    return [
        t.find("name").text.strip()
        for t in root.findall("target")
        if t.find("name") is not None and t.find("name").text
    ]


def is_valid_target(target_name):
    """Return True if target exists in target.xml."""
    if not target_name or not str(target_name).strip():
        return False
    root = load_target_xml()
    if root is None:
        return False
    for t in root.findall("target"):
        name_el = t.find("name")
        if name_el is not None and name_el.text and name_el.text.strip() == target_name:
            return True
    return False


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


def get_target_toolchain(target_name):
    """Map TARGET name to toolchain from target.xml. Default gcc_zynqps.mk."""
    root = load_target_xml()
    if root is None:
        return "gcc_zynqps.mk"
    for t in root.findall("target"):
        name_el = t.find("name")
        if name_el is not None and name_el.text and name_el.text.strip() == target_name:
            tc_el = t.find("toolchain")
            if tc_el is not None and tc_el.text:
                return tc_el.text.strip()
    return "gcc_zynqps.mk"


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
        display_name = (name_el.text or "").strip() if name_el is not None and name_el.text else seg
        git_el = el.find("git")
        git_url = (git_el.text or "").strip() if git_el is not None else ""
        configs = [(c.get("enabled") == "y", (c.text or "").strip()) for c in el.findall("config") if (c.text or "").strip()]
        info = {"path": full_path, "name": display_name, "git": git_url, "configs": configs, "parent_path": base_path, "submodules": []}
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
    """Find submodule dict by name or path. Returns None if not found. Case-insensitive for name."""
    if not name_or_path or not isinstance(name_or_path, str):
        return None
    key = name_or_path.strip()
    key_lower = key.lower()

    def search(items):
        for item in items:
            name_match = item["name"].lower() == key_lower if item.get("name") else False
            path_match = item["path"] == key if item.get("path") else False
            if name_match or path_match:
                return item
            found = search(item.get("submodules", []))
            if found:
                return found
        return None
    return search(list(parse_submodules_config()))


def _collect_configs_ordered(node):
    """Recursively collect config symbols from node and descendants in XML document order."""
    result = []
    seen = set()
    for c in node.findall("config"):
        txt = (c.text or "").strip()
        if txt and txt not in seen:
            seen.add(txt)
            result.append(txt)
    for child in node.findall("submodule"):
        for txt in _collect_configs_ordered(child):
            if txt not in seen:
                seen.add(txt)
                result.append(txt)
    return result


def find_by_path_in_list(nodes, path):
    """Find submodule node by path in flat/nested list."""
    for n in nodes:
        if n["path"] == path:
            return n
        found = find_by_path_in_list(n.get("submodules", []), path)
        if found:
            return found
    return None
