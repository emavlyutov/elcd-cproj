# Software Repo - C Make System

Independent C build system for embedded projects. Uses simple Makefiles and shared source code (HAL, RTOS, CLI, etc.). Can be used standalone or as submodule of the main repo.

## Quick Start

**From main repo:**
```batch
python mktarg.py init_workspace elvpn
python mktarg.py init_prj elvpn
```

**Standalone (from software/):**
```batch
python mktarg.py init_workspace elvpn
python mktarg.py init_prj elvpn
python mktarg.py build_project elvpn fsbl
```

## mktarg.py Commands

`python mktarg.py <function> [WS_NAME] [PROJECT_NAME] [args...]`

| Function | Description |
|----------|-------------|
| `init_workspace` | Create branch, `sources/[WS_NAME]/`, `sources/[WS_NAME]/common/` |
| `reset_workspace` | Switch to main, remove workspace folders |
| `init_project` | Create single project in workspace |
| `init_prj` | Create all projects from root `[WS_NAME].xml` |
| `reset_project` | Remove one project |
| `reset_prj` | Remove all projects in workspace |
| `regenerate_project` | Reset + re-create one project |
| `build_project` | Build (prefer hardware tools if available) |
| `clean_project` | Clean build artifacts |
| `clean_workspace` | Clean build dir |
| `clean_all` | Full clean |
| `clean_all_workspace` | Alias for clean_all |

## Layout

- `sources/[WS_NAME]/` - workspace root
- `sources/[WS_NAME]/common/` - shared per-workspace
- `sources/[WS_NAME]/[PROJECT]/` - each project (Makefile, platform.config, platform_common.c, etc.)
- `sources/Makefile` - auto-generated, lists TARGETS
- `makesys/` - build system (prj_init.mk, toolchain.mk)
- `sources/shared/` - shared ElAPI, thirdparty

## Build

```batch
make build_fsbl
make build_syscpu
make release
```
Requires `PROJECT_NAME` and `TARGETS` in sources/Makefile (set by init_prj).

## Wrappers

- `mktarg.sh` / `mktarg.bat` - wrappers that call `python mktarg.py`
