#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Only Joakim Eriksson's hardware emulator. No physical MIDI/USB connection.
set -eu
AMB_PROJECT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
AMB_EMULATOR="${AMB_EMULATOR:-$AMB_PROJECT/../dependencies/emulator-bin/fm1-emu-0.5.1-macos-arm64/fm1-live}"
AMB_PACKAGE="${1:-$AMB_PROJECT/build/felucca.fwsc}"
[ -x "$AMB_EMULATOR" ] || { printf '%s\n' 'Set AMB_EMULATOR to the pinned fm1-live executable.' >&2; exit 1; }
[ -f "$AMB_PACKAGE" ] || { printf '%s\n' 'Package missing; run ambplus/build.sh.' >&2; exit 1; }
exec "$AMB_EMULATOR" --fwsc "$AMB_PACKAGE" --midi none --mhz 240
