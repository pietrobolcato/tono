#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Noninteractive C tests only; no physical-device IO, no golden/budget updates.
set -eu
AMB_PROJECT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$AMB_PROJECT"
mkdir -p build/host build/amb-ui
for AMB_TEST in amb_controls_test amb_policy_test amb_music_test; do
    cc -std=c11 -O2 -Wall -Wextra -Werror "tests/$AMB_TEST.c" -o "build/host/$AMB_TEST"
    "build/host/$AMB_TEST"
done
cc -O1 -w -Ibuild/gen -Ifirmware/src tests/amb_native_test.c -lm -o build/host/amb_native_test
build/host/amb_native_test build/amb-demo.wav
cc -O1 -w -Ibuild/gen -Ifirmware/src tests/amb_feedback_test.c -lm -o build/host/amb_feedback_test
build/host/amb_feedback_test
cc -O1 -w -Ibuild/gen -Ifirmware/src tests/amb_transition_test.c -lm -o build/host/amb_transition_test
build/host/amb_transition_test
cc -O1 -w -Ibuild/gen -Ifirmware/src tests/amb_render_test.c -lm -o build/host/amb_render_test
build/host/amb_render_test build/amb-ui
printf '%s\n' 'AMB+ targeted tests completed. Also run the full JIANT suite against an AMBPLUS=0 build and exact-package emulator tests; see ambplus/README.md.'
