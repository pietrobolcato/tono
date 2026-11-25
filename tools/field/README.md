# Field recordings

Kiel Harbour (8 seconds) follows the Kiel–Oslo ferry horn at two in the afternoon. Subway Voice (6.5 seconds) follows a New York subway announcement over the train rumble. Both are continuous excerpts, with their original timing. Find them in Texture → Sound or Sample → Source. They default to forward looping at the recorded speed. Loop direction, pitch, trim and normal layer effects remain available.

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
