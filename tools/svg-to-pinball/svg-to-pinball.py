from svgelements import SVG
from svgelements import Group
from svgelements import Path
from svgelements import Point
from svgelements import Circle
from svgelements import Rect
from math import sqrt, pow


def extractCircles(gs: list) -> list[str]:
    """Recursively extract all circles from this list of SVG things

    Args:
        gs (list): A list that contains Group and Circle

    Returns:
        list[str]: A list of C strings for the circles
    """
    lines = []
    for g in gs:
        if isinstance(g, Circle):
            lines.append('  {.pos = {.x = %d, .y = %d}, .radius = %d},' % (
                g.cx, g.cy, (g.rx + g.ry) / 2))
        elif isinstance(g, Group):
            lines.extend(extractCircles(g))
        else:
            print('Found ' + type(g) + ' when extracting Circles')
    return lines


def extractRectangles(gs: list) -> list[str]:
    """Recursively extract all circles from this list of SVG things

    Args:
        gs (list): A list that contains Group and Circle

    Returns:
        list[str]: A list of C strings for the circles
    """
    lines = []
    for g in gs:
        if isinstance(g, Rect):
            lines.append('  {.pos = {.x = %d, .y = %d}, .width = %d, .height = %d},' % (
                g.x, g.y, g.width, g.height))
        elif isinstance(g, Group):
            lines.extend(extractRectangles(g))
        else:
            print('Found ' + str(type(g)) + ' when extracting Rects')
    return lines


def extractPaths(gs: list) -> list[str]:
    """Recursively extract all paths from this list of SVG things

    Args:
        gs (list): A list that contains Group and Path

    Returns:
        list[str]: A list of C strings for the path segments
    """
    lines = []
    for g in gs:
        if isinstance(g, Path):
            lastPoint: Point = None
            point: Point
            for point in g.as_points():
                if lastPoint is not None and lastPoint != point:
                    lines.append('  {.p1 = {.x = %d, .y = %d}, .p2 = {.x = %d, .y = %d}},' % (
                        lastPoint.x, lastPoint.y, point.x, point.y))
                lastPoint = point
        elif isinstance(g, Group):
            lines.extend(extractPaths(g))
        else:
            print('Found ' + str(type(g)) + ' when extracting Paths')
    return lines


def extractFlippers(gs: list) -> list[str]:
    """Recursively extract all flippers (groups of circles and paths) from this list of SVG things

    Args:
        gs (list): A list that contains stuff

    Returns:
        list[str]: A list of C strings for the path segments
    """
    lines = []
    flipperParts: list[Circle] = []
    for g in gs:
        if isinstance(g, Circle):
            flipperParts.append(g)
        elif isinstance(g, Path):
            pass
        elif isinstance(g, Group):
            lines.extend(extractFlippers(g))
        else:
            print('Found ' + str(type(g)) + ' when extracting Flippers')

    if 2 == len(flipperParts):
        if flipperParts[0].rx > flipperParts[1].rx:
            pivot = flipperParts[0]
            tip = flipperParts[1]
        else:
            pivot = flipperParts[1]
            tip = flipperParts[0]

        if pivot.cx < tip.cx:
            facingRight = True
        else:
            facingRight = False

        flipperLen = sqrt(pow(pivot.cx - tip.cx, 2) +
                          pow(pivot.cy - tip.cy, 2))

        lines.append('  {.cPivot = {.pos = {.x = %d, .y = %d}, .radius = %d}, .tRadius = %d, .len = %d, .facingRight = %s},' % (
            pivot.cx, pivot.cy, pivot.rx, tip.rx, flipperLen, 'true' if facingRight else 'false'))

    return lines


def main():
    # Load the SVG
    g: Group = SVG().parse('pinball.svg')

    # Start a string
    cstr = ''

    # Extract walls
    cstr += 'static const lineFl_t constWalls[] = {\n'
    cstr += '\n'.join(extractPaths(g.objects['Walls']))
    cstr += '\n};\n\n'

    # Extract bumpers
    cstr += 'static const circleFl_t constBumpers[] = {\n'
    cstr += '\n'.join(extractCircles(g.objects['Bumpers']))
    cstr += '\n};\n\n'

    # Extract launchers
    cstr += 'static const rectangleFl_t constLaunchers[] = {\n'
    cstr += '\n'.join(extractRectangles(g.objects['Launchers']))
    cstr += '\n};\n\n'

    # Extract flippers
    cstr += 'static const flipperFl_t constFlippers[] = {\n'
    cstr += '\n'.join(extractFlippers(g.objects['Flippers']))
    cstr += '\n};'

    # Print the result
    print(cstr)


if __name__ == "__main__":
    main()
