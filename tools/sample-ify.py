#!/usr/bin/env python3

from PIL import Image
import math
import random
import sys

SQUARE_HALFPERIOD = 4

def is_pixel_set(px):
    return (px[0] < 128 and px[1] < 128 and px[2] < 128) or px[3] < 128

def convert_image(path, mode="noise", raw_out=None):
    with Image.open(path) as img:
        w, h = img.size

        scaled_img = img

        if w != 256 or h != 256:
            scaled_img = img.resize((256, 256), resample=Image.Resampling.NEAREST)

        print(f"Converting {w}x{h} image to a 256-sample wave using method: {mode}")
        if mode == "square":
            print("Squarewave frequency is", (32768 / (2 * SQUARE_HALFPERIOD)), "Hz")

        wavedata = []

        for x in range(256):
            top_pixel = 0
            bottom_pixel = 255

            wave_val = x % 16

            sample = 128

            for y in range(256):
                px = scaled_img.getpixel((x, y))

                if is_pixel_set(px):
                    if y > top_pixel:
                        top_pixel = y
                    if y < bottom_pixel:
                        bottom_pixel = y

            if mode == "noise":
                if top_pixel == bottom_pixel:
                    sample = 255 - top_pixel
                else:
                    sample = 255 - random.randint(bottom_pixel, top_pixel)
            elif mode == "max" or (mode == "alt" and (x % 2) == 1):
                sample = 255 - top_pixel
            elif mode == "min" or (mode == "alt" and (x % 2) == 0):
                sample = 255 - bottom_pixel
            elif mode == "avg":
                sample = 255 - ((bottom_pixel + top_pixel) // 2)
            elif mode == "square":
                squareval = (x % (SQUARE_HALFPERIOD * 2)) < SQUARE_HALFPERIOD
                sample = 255 - (top_pixel if squareval else bottom_pixel)
            elif mode == "sine":
                sample = 128 + int(round(128 * math.cos(wave_val * 2 * math.pi / 16), 0)) * ((bottom_pixel + top_pixel) // 2) // 128

            wavedata.append(sample)

        print("const int8_t sample[] = {")
        for n in range(16):
            sub_bytes = wavedata[n*16:(n+1)*16]
            print(", ".join((f"{b-128:4d}" for b in sub_bytes)), end=",\n")
        print("};")

        if raw_out:
            with open(f"{raw_out}", "wb") as outf:
                outf.write(bytes(wavedata))



if __name__ == "__main__":
    if len(sys.argv) > 3:
        convert_image(sys.argv[1], mode=sys.argv[2], raw_out=sys.argv[3])
    elif len(sys.argv) > 2:
        convert_image(sys.argv[1], mode=sys.argv[2])
    elif len(sys.argv) > 1:
        convert_image(sys.argv[1])
    else:
        print(f"Usage: {sys.argv[0]} <image-file> [noise|max|min|alt|avg|sine] [raw-out-filename]")
