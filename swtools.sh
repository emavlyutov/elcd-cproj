#!/bin/bash
# Wrapper for script/py/*.py
cd "$(dirname "$0")"
exec python3 script/py/swtools.py "$@"
