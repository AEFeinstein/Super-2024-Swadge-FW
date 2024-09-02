from svgelements import SVG
from svgelements import Group
from svgelements import Path
from svgelements import Point
from svgelements import Circle
from svgelements import Rect
from math import sqrt, pow
from enum import Enum

groups = []


def getIntGroupId(gId: str) -> int:
    try:
        if (gId.startswith('group_')):
            gInt = int(gId.split('_')[1])
            if gInt not in groups:
                groups.append(gInt)
            return gInt
        return 0
    except:
        return 0


def getIntId(id: str) -> int:
    try:
        return int(id)
    except:
        return 0


class LineType(Enum):
    JS_WALL = 0
    JS_SLINGSHOT = 1
    JS_DROP_TARGET = 2
    JS_STANDUP_TARGET = 3
    JS_SPINNER = 4

class CircleType(Enum):
    JS_BUMPER = 0
    JS_ROLLOVER = 1

class pbPoint:
    def __init__(self, p: Point = None, x: int = 0, y: int = 0) -> None:
        if (p is not None):
            self.x: int = int(p.x)
            self.y: int = int(p.y)
        else:
            self.x: int = int(x)
            self.y: int = int(y)

    def toBytes(self) -> bytearray:
        return bytearray([(self.x >> 8) & 0xFF, self.x & 0xFF, (self.y >> 8) & 0xFF, self.y & 0xFF])


class pbLine:
    def __init__(self, p1: pbPoint, p2: pbPoint, type: LineType, gId: str, id: str) -> None:
        self.p1 = p1
        self.p2 = p2
        self.type: int = type.value
        self.gId: int = getIntGroupId(gId)
        self.id: int = getIntId(id)

        match(type):
            case LineType.JS_WALL:
                self.isSolid = True
                self.pushVel = 0
                pass
            case LineType.JS_SLINGSHOT:
                self.isSolid = True
                self.pushVel = 80
                pass
            case LineType.JS_DROP_TARGET:
                self.isSolid = True
                self.pushVel = 40
                pass
            case LineType.JS_STANDUP_TARGET:
                self.isSolid = True
                self.pushVel = 0
                pass
            case LineType.JS_SPINNER:
                self.isSolid = False
                self.pushVel = 0
                pass

    def __str__(self) -> str:
        return "{.p1 = {.x = %d, .y = %d}, .p2 = {.x = %d, .y = %d}}," % (self.p1.x, self.p1.y, self.p2.x, self.p2.y)

    def toBytes(self) -> bytearray:
        b = bytearray([(self.id >> 8), self.id, self.gId])
        b.extend(self.p1.toBytes())
        b.extend(self.p2.toBytes())
        b.append(self.type)
        b.append(self.pushVel)
        b.append(self.isSolid)
        # print(' '.join(['%02X' % x for x in b]))
        return b


class pbCircle:
    def __init__(self, pos: pbPoint, radius: int, type: CircleType, pushVel: int, gId: str, id: str) -> None:
        self.position = pos
        self.radius = int(radius)
        self.type = type.value
        self.gId: int = getIntGroupId(gId)
        self.id: int = getIntId(id)
        self.pushVel = int(pushVel)

    def __str__(self) -> str:
        return '{.pos = {.x = %d, .y = %d}, .radius = %d},' % (self.position.x, self.position.y, self.radius)

    def toBytes(self) -> bytearray:
        b = bytearray([(self.id >> 8), self.id, self.gId])
        b.extend(self.position.toBytes())
        b.append(self.radius)
        b.append(self.type)
        b.append(self.pushVel)
        # print(' '.join(['%02X' % x for x in b]))
        return b


class pbRectangle:
    def __init__(self, position: pbPoint, size: pbPoint, gId: str, id: str) -> None:
        self.position = position
        self.size = size
        self.gId: int = getIntGroupId(gId)
        self.id: int = getIntId(id)

    def __str__(self) -> str:
        return '{.pos = {.x = %d, .y = %d}, .width = %d, .height = %d},' % (self.position.x, self.position.y, self.size.x, self.size.y)

    def toBytes(self) -> bytearray:
        b = bytearray([(self.id >> 8), self.id, self.gId])
        b.extend(self.position.toBytes())
        b.extend(self.size.toBytes())
        # print(' '.join(['%02X' % x for x in b]))
        return b


class pbFlipper:
    def __init__(self, pivot: pbPoint, radius: int, length: int, facingRight: bool) -> None:
        self.pivot = pivot
        self.radius = int(radius)
        self.length = int(length)
        self.facingRight = bool(facingRight)

    def __str__(self) -> str:
        return '{.cPivot = {.pos = {.x = %d, .y = %d}, .radius = %d}, .len = %d, .facingRight = %s},' % (
            self.pivot.x, self.pivot.y, self.radius, self.length, 'true' if self.facingRight else 'false')

    def toBytes(self) -> bytearray:
        b = bytearray()
        b.extend(self.pivot.toBytes())
        b.append(self.radius)
        b.append(self.length)
        b.append(self.facingRight)
        # print(' '.join(['%02X' % x for x in b]))
        return b


def extractCircles(gs: list, type: CircleType, gId: str) -> list[pbCircle]:
    """Recursively extract all circles from this list of SVG things

    Args:
        gs (list): A list that contains Group and Circle

    Returns:
        list[str]: A list of C strings for the circles
    """
    circles = []
    for g in gs:
        if isinstance(g, Circle):
            circles.append(pbCircle(pbPoint(x=g.cx, y=g.cy),
                           (g.rx + g.ry) / 2, type, 120, gId, g.id))
        elif isinstance(g, Group):
            circles.extend(extractCircles(g, type, g.id))
        else:
            print('Found ' + str(type(g)) + ' when extracting Circles')
    return circles


def extractRectangles(gs: list, gId: str) -> list[pbRectangle]:
    """Recursively extract all circles from this list of SVG things

    Args:
        gs (list): A list that contains Group and Circle

    Returns:
        list[str]: A list of C strings for the circles
    """
    rectangles = []
    for g in gs:
        if isinstance(g, Rect):
            rectangles.append(pbRectangle(
                pbPoint(x=g.x, y=g.y), pbPoint(x=g.width, y=g.height), gId, g.id))
        elif isinstance(g, Group):
            rectangles.extend(extractRectangles(g, g.id))
        else:
            print('Found ' + str(type(g)) + ' when extracting Rects')
    return rectangles


def extractPaths(gs: list, lineType: LineType, gId: str) -> list[pbLine]:
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
                    lines.append(pbLine(pbPoint(p=lastPoint),
                                 pbPoint(p=point), lineType, gId, g.id))
                lastPoint = point
        elif isinstance(g, Group):
            lines.extend(extractPaths(g, lineType, g.id))
        else:
            print('Found ' + str(type(g)) + ' when extracting Paths')
    return lines


def extractFlippers(gs: list, gId: str) -> list[pbFlipper]:
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
            lines.extend(extractFlippers(g, g.id))
        else:
            print('Found ' + str(type(g)) + ' when extracting Flippers')

    if 2 == len(flipperParts):
        if 'pivot' in flipperParts[0].id.lower():
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

        lines.append(pbFlipper(pbPoint(x=pivot.cx, y=pivot.cy),
                     pivot.rx, flipperLen, facingRight))

    return lines


def addLength(tableData: bytearray, array: int):
    length = len(array)
    b = [(length >> 8) & 0xFF, (length) & 0xFF]
    tableData.extend(b)
    # print(' '.join(['%02X' % x for x in b]))


def main():
    # Load the SVG
    g: Group = SVG().parse('pinball.svg')

    lines: list[pbLine] = []
    lines.extend(extractPaths(g.objects['Walls'], LineType.JS_WALL, None))
    lines.extend(extractPaths(
        g.objects['Slingshots'], LineType.JS_SLINGSHOT, None))
    lines.extend(extractPaths(
        g.objects['Drop_Targets'], LineType.JS_DROP_TARGET, None))
    lines.extend(extractPaths(
        g.objects['Standup_Targets'], LineType.JS_STANDUP_TARGET, None))

    circles: list[pbCircle] = []
    circles.extend(extractCircles(g.objects['Rollovers'], CircleType.JS_ROLLOVER, None))
    circles.extend(extractCircles(g.objects['Bumpers'], CircleType.JS_BUMPER, None))

    launchers = extractRectangles(g.objects['Launchers'], None)
    flippers = extractFlippers(g.objects['Flippers'], None)

    tableData: bytearray = bytearray()
    tableData.append(max(groups))

    addLength(tableData, lines)
    for line in lines:
        tableData.extend(line.toBytes())

    addLength(tableData, circles)
    for circle in circles:
        tableData.extend(circle.toBytes())

    addLength(tableData, launchers)
    for launcher in launchers:
        tableData.extend(launcher.toBytes())

    addLength(tableData, flippers)
    for flipper in flippers:
        tableData.extend(flipper.toBytes())

    with open('table.bin', 'wb') as outFile:
        outFile.write(tableData)


if __name__ == "__main__":
    main()
