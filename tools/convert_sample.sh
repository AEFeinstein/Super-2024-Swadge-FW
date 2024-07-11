#!/bin/bash

if [ "$#" -eq "1" ]; then
    TRIM=""
elif [ "$#" -eq "2" ]; then
    TRIM=" trim $2"
elif [ "$#" -eq "3" ]; then
    TRIM=" trim $2 $3"
else
    echo "Usage: $0 <input-file> [<start-time> [<end-time>]]"
    echo
    echo "Converts any audio or video file to the appropriate format for the Swadge DAC"
    echo " Note: Requires ffmpeg / libavcodec and sox"
    exit 0
fi

FFMPEG="ffmpeg"

if ! ${FFMPEG} -version >/dev/null 2>&1; then
    FFMPEG="avconv"

    if ! $FFMPEG -version >/dev/null 2>&1; then
        echo "Err: ffmpeg or avconv binary not found!"
        exit 1
    fi
fi

if ! sox --version >/dev/null 2>&1; then
    echo "Err: sox binary not found!"
    exit 1
fi

OUTFILE="${1%.*}.raw"

if $FFMPEG -hide_banner -loglevel warning -i "$1" -f wav - | sox -t wav - -t raw -r 32768 -b 8 -c 1 -e unsigned-integer "$OUTFILE" $TRIM ; then
    echo "Converted output saved to $OUTFILE"
else
    CODE=$?
    echo "Err: Failed to convert!"
    exit $CODE
fi
# To use sox directly on an audio file:
# sox "$1" -t raw -r 32768 -b 8 -c 1 -e unsigned-integer "$OUTFILE" $TRIM
