#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Local compilation only. Does not discover or communicate with an FM1.
set -eu
AMB_PROJECT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
export AMBPLUS=1
export PYTHON="${PYTHON:-$AMB_PROJECT/../dependencies/python/bin/python}"
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$AMB_PROJECT/../dependencies/jieli/jieli-linux-toolchains-20260730.1}"
export AC79_SDK="${AC79_SDK:-$AMB_PROJECT/../dependencies/ac79-sdk}"
export JIELI_DOCKER_IMAGE="${JIELI_DOCKER_IMAGE:-debian@sha256:734a72b3d6142a71993b4bc580ca01d8ca687f77a2c422d96a961d3376b179bc}"
exec "$AMB_PROJECT/build.sh" "$@"
