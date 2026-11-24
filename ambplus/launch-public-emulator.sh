#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")"
version=$(cat VERSION)
image="tono-$version.fwsc"
state="emulator-state-$version.bin"
exec ./fm1-live --fwsc "$image" --flash "$state" --midi none --mhz 240 --flash-chip uc25hq80 "$@"
