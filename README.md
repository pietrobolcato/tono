# Tono

**A four-layer ambient instrument for the M-VAVE FM-1.** Bring in a bassline, let a pad hang over it, then turn a texture into a drum pulse while the piece keeps playing. Each layer has its own voice, effects and sequencer.

![Meet Tono. The FM-1 against four coloured waves.](docs/tono/media/meet-tono.jpg)

<!-- Link this poster and the film title to the YouTube watch URL once available. -->

**[Play Tono in your browser](https://pietrobolcato.github.io/tono/)** · *Meet Tono · coming to YouTube* · **[Get the firmware](https://github.com/pietrobolcato/tono/releases/latest)**

## Listen

At **1:00** in the film, a rolling snare gives way to three grand pianos and a bowed cello. At **1:45**, *First Ferry* opens a short medley that closes with *Symphony for Mars*.

[Unhurried Machines on YouTube](https://www.youtube.com/@unhurriedmachines)

## Play your first piece

1. Open the browser player, click **Start**, then **PLAY**. The first arrangement is *Garden*.
2. On **HOME**, turn **SELECT** to choose a layer. Turn **MASTER** to fade that layer in or out. Try leaving the pad ringing while you bring the bass back.
3. Turn **ALGORITHM** to evolve the piece. To evolve just the selected layer, hold **HOME** while turning **ALGORITHM**. Press **HOME + EDIT** for help with the current screen.

Explore the factory arrangements, Generate presets, A22 sound bank and sampled textures. In **GLO**, choose Studio Dark, Studio Light or Dial. The Contour and Train Ride displays respond to the music. [Controls](ambplus/CONTROLS.md) · [Help screens](docs/tono/HELP.md)

## Install on an FM-1

Tono targets the **M-VAVE FM-1**. Release **v0.74.2-opt** recognises two flash chips for saving songs and settings. Check the flash ID reported by FM-SWITCH before writing.

| Flash chip | JEDEC ID |
| --- | --- |
| Puya P25Q80 | `0x856014` |
| UCUN UC25HQ80 | `0xB36014` |

1. Download the versioned **.fwsc** and **SHA256SUMS** from the same release. On macOS, run `shasum -a 256 tono-0.74.2-opt.fwsc` and compare the result with the entry for that file. [Release files and evidence](https://github.com/pietrobolcato/tono/releases/tag/v0.74.2-opt)
2. In **FM-SWITCH**, make a whole-flash backup, keep a separate copy and export recordings you want to keep. Check your recovery route first. A stock M-VAVE bootloader can require a **USB_KEY** if the application fails to start. [FM-SWITCH](https://fm-1.drewr.dev/fm-switch/) · [Install and recover](docs/tono/INSTALL.md)
3. Import the **.fwsc**. To keep songs and settings, leave **Start the new firmware fresh** unchecked. Confirm product **FM-1_900** and application writes within **0x04000–0x92FFF**. Stop if the product, flash ID or write range differs from the values above.
4. Keep power and USB connected until verification finishes. After restart, check the version on the startup screen, play at low volume and recall a saved song.

**On an Apple silicon Mac**, download and extract the macOS ARM64 emulator bundle and run `./launch-emulator.sh` from the extracted folder. It runs the released firmware and keeps its saved state on your Mac. [Downloads](https://github.com/pietrobolcato/tono/releases/latest)

## Inside Tono

Tono builds on Juan Jiant’s **JIANT FM**, Leo Kuroshita’s **Felucca** and Charles Vestal’s **fm1-amb**. The firmware is GPL-3.0-only, with separate notices for incorporated code, fonts and recordings. [Licensing and credits](LICENSING.md)

[Build from source](docs/tono/BUILD.md) · [Architecture](docs/tono/ARCHITECTURE.md) · [Release history](docs/tono/releases/)

If you report an issue, include your Tono version, the steps to reproduce it and whether you were playing on hardware or in the emulator.
