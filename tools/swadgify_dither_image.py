from PIL import Image

#plug your filename in here and it outputs a web-safe dithered output.
name = "credz.png"

BAYER_4x4 = [
    [ 0,  8,  2, 10],
    [12,  4, 14,  6],
    [ 3, 11,  1,  9],
    [15,  7, 13,  5],
]

BAYER_8x8 = [
    [ 0, 48, 12, 60,  3, 51, 15, 63],
    [32, 16, 44, 28, 35, 19, 47, 31],
    [ 8, 56,  4, 52, 11, 59,  7, 55],
    [40, 24, 36, 20, 43, 27, 39, 23],
    [ 2, 50, 14, 62,  1, 49, 13, 61],
    [34, 18, 46, 30, 33, 17, 45, 29],
    [10, 58,  6, 54,  9, 57,  5, 53],
    [42, 26, 38, 22, 41, 25, 37, 21],
]

def dither_channel(v, threshold, levels=6):
    """
    v: 0-255 channel value
    threshold: Bayer matrix value
    levels: number of palette levels (6 for web-safe)
    """
    step = 255 // (levels - 1)  # 51

    base = v // step
    rem  = v % step

    # Scale threshold to step range
    t = (threshold * step) // (levels * levels)

    if rem > t:
        base += 1

    if base < 0:
        base = 0
    elif base > levels - 1:
        base = levels - 1

    return base * step

def bayer_dither_4x4(img):
    img = img.convert("RGB")
    px = img.load()
    w, h = img.size

    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            t = BAYER_4x4[y & 3][x & 3]

            px[x, y] = (
                dither_channel(r, t),
                dither_channel(g, t),
                dither_channel(b, t),
            )

    return img

def bayer_dither_8x8(img):
    img = img.convert("RGB")
    px = img.load()
    w, h = img.size

    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            t = BAYER_8x8[y & 7][x & 7]

            px[x, y] = (
                dither_channel(r, t),
                dither_channel(g, t),
                dither_channel(b, t),
            )

    return img

im = Image.open(name)

dithered = bayer_dither_8x8(im)
dithered.save(f"swadge-{name}")
