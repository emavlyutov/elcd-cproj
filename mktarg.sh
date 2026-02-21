#!/bin/bash
# Wrapper for software/mktarg.py
cd "$(dirname "$0")"
exec python3 mktarg.py "$@"
