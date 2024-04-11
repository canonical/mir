#!/usr/bin/env bash
cd "$(dirname "$0")"
rm -rf venv
python3 -m venv venv
source venv/bin/activate
pip install -r ./requirements.txt
./main.py "$@"
