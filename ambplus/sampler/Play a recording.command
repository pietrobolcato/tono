#!/bin/sh
set -eu
dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
take=$(osascript -e 'POSIX path of (choose file with prompt "Choose a voice or instrument recording for Tono")') || exit 0
exec "$dir/launch-emulator.sh" --take "$take"
