#!/usr/bin/env python3
"""Runnable wrapper so the CLI works without `pip install` (mirrors ./RE6Commander).

Usage:
    ./re7commander.py import test_bp/SMS.csv
    ./re7commander.py dump MobilePromo1

Once installed via pyproject, the console script `re7commander` does the same.
"""

import sys

from re_cli.cli import main

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
