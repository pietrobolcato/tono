# Build Tono

Follow the toolchain, SDK and platform prerequisites in [BUILDING.md](../../BUILDING.md). Use the pinned toolchain, SDK, Python packages and container in the release's SOURCES_LOCK.json to reproduce a binary. On Apple Silicon, Docker runs the Linux x86-64 toolchain.

From the clone root, with Pillow and fontTools installed:

```sh
export JIELI_TOOLCHAIN="$HOME/.jieli/toolchain"
export AC79_SDK="$HOME/fw-AC79_AIoT_SDK"
export PYTHON=python3
./ambplus/build.sh
./ambplus/test.sh
```

The Tono firmware is `build/felucca.fwsc` and its matching debug image is `build/felucca.elf`. Released packages use the version in `firmware/src/tono_version.h` in the filename and startup subtitle.
