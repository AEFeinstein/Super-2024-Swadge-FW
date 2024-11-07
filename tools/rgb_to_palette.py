def roundToPalette(inColor: int) -> int:
    minDiff = 0xFF
    step = int(0xFF / 5)
    for val in range(0, 0x100, step):
        diff = abs(inColor - val)
        if diff < minDiff:
            minDiff = diff
            retVal = val
    return int(retVal / step)


ccs = [
    # ["Pango", "D78504", "F5C865", "14D7E7", "E1E8EC", "2F2F2F"],
    # ["Pixel", "FF487C", "FFB4CF", "FF3E4A", "E1E8EC", "14E78E"],
    # ["Poe", "A6A6A6", "58235C", "EF92B9", "E1E8EC", "212122", "F00E1D"],
    # ["Garbotnik", "F6B380", "FA6A63", "3C0D02", "E1E8EC", "252421", "8C33EE"],
    ['Gradient', 'cc9900ff', 'd6ae33ff', 'e0c266ff', 'ebd699ff', 'f5ebccff', 'ffffffff']
]
for cc in ccs:
    palette = []
    for color in cc[1:]:
        r = roundToPalette(int(color[0:2], 16))
        g = roundToPalette(int(color[2:4], 16))
        b = roundToPalette(int(color[4:6], 16))
        palette.append("c%d%d%d" % (r, g, b))
    print("%s: %s" % (cc[0], ", ".join(palette)))