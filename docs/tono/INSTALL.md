# Try or install Tono

## Browser emulator

[Open the Tono player](https://pietrobolcato.github.io/tono/), click **Start**, then click **PLAY** for Garden. The browser runs the current stable firmware through [fm1-emu](https://github.com/joakimeriksson/fm1-emu). For a larger panel, open the instrument in its own tab.

## macOS emulator

Download and extract the macOS ARM64 bundle from [Releases](https://github.com/pietrobolcato/tono/releases). From the extracted folder, run `./launch-emulator.sh`. The bundle runs the exact released firmware at 240 MHz with an emulated UC25HQ80. Saved emulator state stays on your computer. The launcher opens the emulator with physical MIDI disabled.

## M-VAVE FM-1

Download the firmware and release notes. Make a current whole-flash backup with FM-SWITCH, keep a separate copy and export any recordings you need. Verify the tool detects your flash variant and that you have a recovery route.

Import the versioned `.fwsc` into [FM-SWITCH](https://fm-1.drewr.dev/fm-switch/). Keep **Start the new firmware fresh** unchecked when retaining saved songs or settings. Review the write plan before confirming. Expect product `FM-1_900` and application writes within `0x04000–0x92FFF`. Sector count depends on the starting image. Stop if the identity or write area differs.

Keep USB and power connected through verification. After restart, check the version subtitle, play at low volume and recall a saved slot. A stock M-VAVE bootloader can require a USB_KEY if an application fails to start. See the [owner-run release check](../../.agents/skills/fm1-release-gate/SKILL.md) for backup and recovery details.
