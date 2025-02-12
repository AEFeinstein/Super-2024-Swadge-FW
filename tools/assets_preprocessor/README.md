# `assets_preprocessor`

## Usage
```
Usage:
  assets_preprocessor
    -i INPUT_DIRECTORY
    -o OUTPUT_DIRECTORY
```

All files with the extensions listed below are processed. All other files are ignored.

## Filetypes that are Processed

### `.bin`

No processing is done on `.bin` files. They are copied from the input directory to the output directory

### `.font.png`

Font files are black and white `.png` images where each ASCII character from `' '` to `'~'` is drawn in order in a single line, and each character is underlined with a one pixel black line. These files are sliced into characters and processed into a `.font` file. A `.font` file is:

```
Character height (one byte). All characters are the same height

for each character:
  Character width (one byte)
  Bitpacked bitmap, each bit is one pixel. Starts at top-left. The number of bytes is (int)(((width * height) + 7 ) / 8). The last byte may be padded with zero bits.
```

### `.png`

`.png` images are reduced to an 8-bit web-safe color palette, then compressed with [Heatshrink](https://github.com/atomicobject/heatshrink). This file format is called `.wsg` (web safe graphic).

```
TODO detail .wsg format
```

### `.json`

`.json` are compressed with [Heatshrink](https://github.com/atomicobject/heatshrink).

### `.txt`

No processing is done on `.txt` files. They are copied from the input directory to the output directory,

### `.mid`, `.midi`

The notes on the first track from the MIDI file are converted to buzzer format (frequency, duration), and compressed with Heatshrink compression. The buzzer is single channel, single voice, so if a note starts while another is playing, the first note is stopped.

### `.chart`

Generate Clone Hero charts [using this tool](https://efhiii.github.io/midi-ch/), see https://github.com/EFHIII/midi-ch.

[.chart file format spec](https://github.com/TheNathannator/GuitarGame_ChartFormats/blob/main/doc/FileFormats/.chart/Core%20Infrastructure.md).
