#!/bin/bash

mkdir frames
ffmpeg.exe -i '.\Touhou - Bad Apple.mp4' -s 140x120 -r 30 'frames/%04d.png'
