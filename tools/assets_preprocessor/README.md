# `assets_preprocessor`

## Usage
```
Usage:
  assets_preprocessor
    -i INPUT_DIRECTORY
    -o OUTPUT_DIRECTORY
    [-c CONFIG_FILE]
    [-t TIMESTAMP_FILE]
    [-v] [-h]
```

All files with the extensions listed below are processed. All other files are ignored.

## Config File

The asset processor [config file](../../assets.conf) can be used to map new asset file
extensions to be handled by existing asset processors, and to define new command-based
processors that use a shell command to process assets. Note that the global config file
does not allow configuring options below. Each extension mapping is defined as a single
section (marked by a `[section-name]` line) that contains an input extension, an output
extension, and the name of an asset processor function or a shell command. The available
asset processor functions can be listed by running `asset_processor -h`.

### Example
This config file will setup the asset processor to use the `heatshrink` function for
processing `.big` assets, and to run a simple `sed` command to process `.game` assets.
Note that the section name will be used as `inExt` if one is not explicitly provided.
Any leading `.` in the section name, `inExt`, or `outExt` will be ignored.

```config.ini
[big]
outExt=smol
func=heatshrink

[.game]
outExt=gbin
exec=sed 's/[aoeui]/y/g' "%i" > "%o"
```

This config is functionally equivalent to the previous one, but with more readable
formatting and explicitly set input and output file extensions.

```config.ini
; This processor shrinks big files for My Game with heatshrink
[my-big-game-files]
inExt = .big
outExt = .smol
func = heatshrink

; This processor handles small binary game files for My Game
[my-binary-game-files]
inExt = .game
outExt = .gbin

; %i will be the input file
; %o will be the output file
exec = sed 's/[aoeui]/y/g' "%i" > "%o"
```

### Exec Processor Placeholders

Exec processors support several placeholders which can be used to insert the input and
output file paths into the command being executed. These are listed in the table below:

| Placeholder | Replacement                                |
|-------------|--------------------------------------------|
| `%i`        | Full path to the input file                |
| `%f`        | Filename portion of the input file path    |
| `%o`        | Full path to the output file               |
| `%a`        | Input file extension, without leading '.'  |
| `%b`        | Output file extension, without leading '.' |
| `%%`        | Literal `%` character                      |

## <a name="options-files">Options Files</a>

Some asset processors support the use of options files, which can be used to configure
how a specific asset or directory of assets are processed. To set the options for a single
asset, such as `myImage.png`, create a text file with the same name as the asset file but
with the extension `.opts`, such as `myImage.opts`. This file is formatted as an [INI file][ini],
with each section (marked by a `[section-name]` line) noting the options for that processor.
For example, to configure the WSG image processor to use dithering for `myImage.png`, create
`myImage.opts` with the following content:

```myImage.opts
[wsg]
dither=yes
```

To configure asset processing for an entire directory tree, create a text file with
the name `.opts` inside the directory. The options defined in that file will be applied
to all assets within the same directory, including inside subdirectories of that
directory. However, files with a file-specific `<filename>.opts` file and subdirectories
with their own `.opts` files will still use those files instead of a `.opts` file in the
parent directory. Also note that options files are not merged, so _only_ the closest options
file found to the input file will be used. To clear any options set by a `.opts` file in
a parent directory, create an empty `.opts` file or `<filename>.opts` file. For an example
of how the options file is located, given the following file structure:

```
assets/
+-- sprite.png
+-- myGame/
    +-- .opts
    +-- bg.png
    +-- bg.opts
    +-- gameConfig.json
    +-- test/
        +-- data.json
        +-- images/
            +-- .opts [wsg]
            +-- something.png
```

The file `sprite.png` would be processed with no options, and `myGame/bg.png` would be
processed with options from `myGame/bg.opts`. Both `myGame/gameConfig.json` and
`myGame/test/data.json` would be processed with the options from `myGame/.opts`.
And `myGame/test/images/something.png` would be processed with the options from
`myGame/test/images/.opts`.

In other words, for an asset file with the name `assets/a/b/c/d/image.png`,
the options file used will be the first in this list that exists:
* `assets/a/b/c/d/image.opts`
* `assets/a/b/c/d/.opts`
* `assets/a/b/c/.opts`
* `assets/a/b/.opts`
* `assets/a/.opts`
* `assets/.opts`

## File types that are Processed

### `.bin`

No processing is done on `.bin` files. They are copied from the input directory to the output directory.

### `.raw`

`.raw` files are processed only with heatshrink compression, and are otherwise unmodified.

### `.font.png`

Font files are black and white `.png` images where each ASCII character from `' '` to `'~'` is drawn in order in a single line, and each character is underlined with a one pixel black line. These files are sliced into characters and processed into a `.font` file. A `.font` file is:

```
Character height (one byte). All characters are the same height

for each character:
  Character width (one byte)
  Bitpacked bitmap, each bit is one pixel. Starts at top-left. The number of bytes is (int)(((width * height) + 7 ) / 8). The last byte may be padded with zero bits.
```

### `.png`

`.png` images are reduced to an 8-bit web-safe color [palette][paletteColor_t], then compressed with [Heatshrink][heatshrink]. This file format is called `.wsg` (web safe graphic). After decompressing, the WSG data format is:

```
Image Width (two bytes, big-endian)
Image Height (two bytes, big-endian)

Image Data (<Image Width> * <Image Height> bytes)

Each byte of image data represents one pixel, with its value corresponding to the color's index in the WSG palette.
```

#### Options

The WSG processor supports one boolean option `dither`, which can be set to `yes` to force
the use of dithering when processing an image with colors outside of the supported
[palette][paletteColor_t]. This may improve the appearance of larger and less-detailed
images. See the [options instructions][processorOptions] for more information.

### `.json`

`.json` files are validated for proper syntax, minified, and then by default are compressed with [Heatshrink][heatshrink].

#### Options

The JSON processor supports one option, `compress`, which can be set to `no` to disable
compression. See the [options instructions][processorOptions] for more information.

### `.txt`

No processing is done on `.txt` files. They are copied from the input directory to the output directory,

### `.mid`, `.midi`, `.kar`

MIDI files are processed with [Heatshrink][heatshrink] compression only, as the swadge can
play them in their native format.

### `.rmd`

RMD files are maps for the [raycast FPS mode](../../attic/modes/ray/mode_ray.h) created with
the [ray map editor](../rayMapEditor/). They are processed with [Heatshrink][heatshrink]
compression only.

### `.chart`

Generate Clone Hero charts [using this tool](https://efhiii.github.io/midi-ch/), see https://github.com/EFHIII/midi-ch.

[.chart file format spec](https://github.com/TheNathannator/GuitarGame_ChartFormats/blob/main/doc/FileFormats/.chart/Core%20Infrastructure.md).

[heatshrink]: https://github.com/atomicobject/heatshrink
[paletteColor_t]: https://adam.feinste.in/Super-2024-Swadge-FW/palette_8h.html#aed8c673902cb720e5754e04d1cd66f97
[processorOptions]: #options-files
