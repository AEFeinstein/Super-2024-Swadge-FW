from PIL import Image
import os
import heatshrink2
from pathlib import Path


def isBlack(px: tuple[int]) -> bool:
    """Return if a pixel is black or not

    Args:
        px (tuple[int]): An RGB pixel

    Returns:
        bool: true if the pixel is black, false if it is white
    """
    return (px[0] + px[1] + px[2]) < 128 * 3


def imgToBw(img: Image) -> bytearray:
    """Convert an image to a bit-packed black and white array

    Args:
        img (Image): The image to convert, area must be a multiple of 8

    Returns:
        bytearray: A 1D array where each bit is a black or white pixel
    """
    data = bytearray(int(img.width * img.height / 8))
    b = 0
    bitIdx = 0
    byteIdx = 0
    for y in range(img.height):
        for x in range(img.width):
            # Set white bits
            if not isBlack(img.getpixel((x, y))):
                b |= (1 << bitIdx)
            bitIdx += 1

            # If a full byte has been packed
            if 8 == bitIdx:
                # Append it to the output
                data[byteIdx] = b
                byteIdx += 1
                b = 0
                bitIdx = 0
    # Return the converted data
    return data


def rlcZeros(data: bytearray) -> bytearray:
    """Run length encode all the zeros in a byte array. This works well when the
    bytearray is sparse, i.e. mostly zeros relative to all other values.

    Runs of zeros are replaced with a zero followed by the number of zeros in
    the run. If there are more than 254 zeros in the run, additional length
    bytes are appended until there are fewer than 255 zeros left in the run.
    All other bytes are copied as-is

    Args:
        data (bytearray): The bytearray to encode

    Returns:
        bytearray: The encoded bytearray
    """
    rlcData = bytearray(0)

    zCount = 0
    for b in data:

        if 0 == b:
            zCount += 1
        else:
            if zCount > 0:
                rlcData.append(0)
                while zCount >= 0xFF:
                    zCount -= 0xFF
                    rlcData.append(0xFF)
                rlcData.append(zCount)
                zCount = 0
            rlcData.append(b)

    if zCount > 0:
        rlcData.append(0)
        while zCount >= 0xFF:
            zCount -= 0xFF
            rlcData.append(0xFF)
        rlcData.append(zCount)
    return rlcData


def main() -> None:
    """Convert frames which have been extracted by ffmpeg into Swadge data
    """

    # Setup variables
    frameIdx = 0
    priorBw: bytearray = None
    totalSizes = {}

    # For each image in the folder 'frames'
    for filename in sorted(os.listdir('frames')):

        # Read image and convert to black and white bits
        frameBw = imgToBw(Image.open('frames/' + filename))

        # If there is no prior frame
        if priorBw is None:
            # Make one filled with zeros
            priorBw = bytearray([0 for x in range(len(frameBw))])

        # Find the difference between the prior frame and the current one
        diff = bytearray(a ^ b for (a, b) in zip(priorBw, frameBw))

        # Set the prior frame to the current frame
        priorBw = frameBw

        # Run-length encode the difference
        encoded = rlcZeros(diff)

        # For a range of different compression options. (7,3) was tested to be most efficient
        for window in range(7, 8):  # (4 -> 16)
            for lookahead in range(3, 4):  # (3, window)

                # Pick the output folder based on the compression options
                dir = 'shrink_%d_%d' % (window, lookahead)
                Path(dir).mkdir(parents=True, exist_ok=True)

                # Construct the output filename
                outFileName = ('%s/ba%s' % (dir, filename)
                               ).replace('png', 'bin')

                # Write the file using heatshrink compression
                with heatshrink2.open(outFileName, 'wb', window_sz2=window, lookahead_sz2=lookahead) as fOut:
                    fOut.write(encoded)

                # Record the size
                frameSize = os.path.getsize(outFileName)
                key = '%d_%d' % (window, lookahead)
                if key not in totalSizes:
                    totalSizes[key] = 0
                totalSizes[key] += frameSize

        # Increment the frame index
        frameIdx += 1

        # Print a note every 100 frames
        if (0 == frameIdx % 100):
            print('%4d frames' % (frameIdx))

    # After all encoding is done, print all sizes
    for key in totalSizes:
        print('%s -> %d' % (key, totalSizes[key]))


if __name__ == "__main__":
    main()
