# Bad Apple Gen

A tool to convert videos to monochrome and compress them down to fit on a Swadge

## Requirements

```bash
sudo apt install ffmpeg python
pip install -r requirements.txt
```

## How to use

1. Get a copy of "Touhou - Bad Apple.mp4" from https://archive.org/details/TouhouBadApple
1. Run `extract_frames.sh` to get 140x120 sized frames at 30fps in PNG form in the `frames` folder.
1. Run `badapplegen.py` to compress the images in the `frames` folder into BIN form (frame differences -> run length encoding -> heatshrink compression)
1. I haven't been able to find a MIDI file that syncs nicely with the linked video, so find your own