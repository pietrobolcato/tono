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
for AMB_TEST in amb_native_test amb_feedback_test amb_transition_test amb_normal_playback_test amb_handoff_test amb_min_release_test amb_texture_test amb_generation_test amb_garden_test amb_ensemble_test amb_routing_test amb_overlap_test amb_monitor_test amb_bar_octave_test amb_landscape_test amb_render_test; do
    cc -O1 -w -Ibuild/gen -Ifirmware/src "tests/$AMB_TEST.c" -lm -o "build/host/$AMB_TEST"
    case "$AMB_TEST" in
        amb_native_test) build/host/amb_native_test build/amb-demo.wav ;;
        amb_render_test) build/host/amb_render_test build/amb-ui ;;
        *) "build/host/$AMB_TEST" ;;
    esac
done
printf '%s\n' 'AMB+ targeted tests completed. Also run the full JIANT suite against an AMBPLUS=0 build and exact-package emulator tests; see ambplus/README.md.'
