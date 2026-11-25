# Field recordings

Kiel Harbour (8 seconds) follows the Kiel–Oslo ferry horn at two in the afternoon. Subway Voice (6.5 seconds) follows a New York subway announcement over the train rumble. Both are continuous excerpts, with their original timing. Choose Kiel Harbour directly under Texture → Sound. For Subway Voice, choose Texture → Sound → Voices, turn PRESETS to Subway Voice and press REC. Both recordings are also available under Sample → Source. They default to forward looping at the recorded speed. Loop direction, pitch, trim and normal layer effects remain available.

The Kiel recording was made by schaarsen. The subway recording was made by swelltoe77. Both are released under CC0. The manifest includes each original page, download URL and SHA-256. Source offsets are 0:00–0:08 and 0:01.25–0:07.75, respectively. The mono device renditions have a narrower bandwidth than the originals.

## Use personal audio in the two additional factory slots

With Python, NumPy, SciPy and ffmpeg installed, put the two original MP3 previews in a folder with the names in manifest.json, including the .mp3 suffix. From the firmware checkout, regenerate the factory header with:

```sh
python3 tools/field/encode.py /path/to/field-sources
```

To substitute your own recording in the second appended slot, use a WAV containing at least 6.5 seconds:

```sh
python3 tools/field/encode.py /path/to/field-sources --wav 2 /path/to/station.wav
```

Repeat --wav to supply both slots. Slot 1 takes the first 8 seconds. Slot 2 takes the first 6.5 seconds. Substitution preserves the two factory slot names and their playback rates. The source folder only needs the original files for slots you keep. The command only generates firmware source. Compile and check the resulting image size because input dynamics affect compressed size. User mic storage and microphone recording limits are independent.

## Abstract Rain

Abstract Rain is one Texture patch. PRESETS selects Chill Rain, Steady Downpour or Rain & Thunder. A four-second recording of woodland rain supplies the lower frequencies. Two upper bands follow the changing levels of nearly 24 seconds from the same shower. Two 64 ms recordings of rain on different surfaces and one 256 ms close water drop provide irregular foreground splashes. Rain & Thunder plays an independent eight-second thunder recording, with additional rolls at irregular intervals while held. The forest bed uses 4,096 bytes of two-bit samples, 512 bytes for local scales and 1,024 bytes for the upper bands. The drops and thunder use 1,024 bytes each, plus 128 bytes for the thunder's higher-frequency envelope.

All five recordings are CC0. Save the linked previews under the filenames below and run `python3 tools/field/rain.py /path/to/previews` to regenerate the data header. The generator verifies the source hashes and records the sample offsets.

| Local name | Recording | Preview |
| --- | --- | --- |
| courtyard.mp3 | [Medium rain in courtyard, xkeril](https://freesound.org/people/xkeril/sounds/658879/) | [HQ MP3](https://cdn.freesound.org/previews/658/658879_13504080-hq.mp3) |
| puddle.mp3 | [eau qui coule - water dripping, vfrattaroli](https://freesound.org/people/vfrattaroli/sounds/843505/) | [HQ MP3](https://cdn.freesound.org/previews/843/843505_18399564-hq.mp3) |
| thunder.mp3 | [Distant Thunder 3, Fission9](https://freesound.org/people/Fission9/sounds/581124/) | [HQ MP3](https://cdn.freesound.org/previews/581/581124_9395330-hq.mp3) |
| forest.mp3 | [ForestRainShower1a, AlanCat](https://freesound.org/people/AlanCat/sounds/383064/) | [HQ MP3](https://cdn.freesound.org/previews/383/383064_5486695-hq.mp3) |
| plop.mp3 | [Water Droplets, GammaGool](https://freesound.org/people/GammaGool/sounds/860797/) | [HQ MP3](https://cdn.freesound.org/previews/860/860797_8908475-hq.mp3) |

## True Rain

Chill Rain plays four continuous seconds of AlanCat’s **ForestRainShower1a** field recording (45.744–49.744 s). The additional bitstream captures the signed residual between the original 11,264 Hz mono waveform and the decoded low band already in the instrument. Each group of 128 residual frames retains its own amplitude. At the default sound settings all audible material comes from that field recording. Its last 62.5 ms crossfade into the first 62.5 ms, giving a repeating period of about 3.94 seconds. Raising DROPS adds separate recorded surface splashes.

Rain & Thunder keeps the same shower and plays the first eight seconds of Fission9’s **Distant Thunder 3** as a low roll. Its clock runs independently while a note is held. Both source recordings are CC0. Source pages and download links appear in the Abstract Rain table above. Rebuild the high-band header with `python3 tools/field/wet_surface.py /path/to/forest.mp3`. It checks the original recording hash and uses the actual factory low-band data from `amb_rain_drops.h`.
