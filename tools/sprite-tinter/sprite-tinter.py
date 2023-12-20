import os
from PIL import Image


def clamp(n, smallest, largest): return max(smallest, min(int(n), largest))


def tint(fname, postfix, additiveTint):

    with Image.open(fname, 'r') as bs:
        bs = bs.convert('RGBA')
        for y in range(bs.height):
            for x in range(bs.width):
                pixel = bs.getpixel((x, y))
                pixel = (
                    clamp(pixel[0] + additiveTint[0], 0x00, 0xFF),
                    clamp(pixel[1] + additiveTint[1], 0x00, 0xFF),
                    clamp(pixel[2] + additiveTint[2], 0x00, 0xFF),
                    pixel[3])
                bs.putpixel((x, y), pixel)

        newPath = os.path.splitext(fname)[0] + postfix + os.path.splitext(fname)[1]
        bs.save(newPath)


# assign directory
directory = 'bs'

# iterate over files in
# that directory
for filename in os.listdir(directory):
    f = os.path.join(directory, filename)
    # checking if it is a file
    if os.path.isfile(f):
        tint(f, '_M', [0x33, -0x33, -0x33])
        tint(f, '_X', [-0x33, 0x33, -0x33])
        tint(f, '_I', [-0x33, -0x33, 0x33])
