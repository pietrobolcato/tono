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
