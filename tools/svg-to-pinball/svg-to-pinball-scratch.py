import re
import svg_plain
import json

warningsPrinted = []


def printWarning(warnStr: str):
    if warnStr not in warningsPrinted:
        warningsPrinted.append(warnStr)
        print('WARNING: ' + warnStr)


class PinPoint:
    def __init__(self, x: float = 0, y: float = 0, p=None) -> None:
        if p is not None:
            self.x = p.x
            self.y = p.y
        else:
            self.x = x
            self.y = y


class PinLine:
    def __init__(self, start: PinPoint, end: PinPoint) -> None:
        self.start: PinPoint = start
        self.end: PinPoint = end

    def __str__(self) -> str:
        return "[(%f, %f), (%f, %f)]" % (self.start.x, self.start.y, self.end.x, self.end.y)


class PinCircle:
    def __init__(self, cx: int, cy: int, r: int) -> None:
        self.cx = cx
        self.cy = cy
        self.r = r


class PinballTable:
    def __init__(self) -> None:
        self.walls: list[PinLine] = []
        self.bumpers: list[PinCircle] = []
        # self.flippers = []

    def toCStr(self) -> str:
        cstr = ''

        cstr += 'lineFl_t walls[%d] = {\n' % len(self.walls)
        for wall in self.walls:
            cstr += '  {.p1 = {.x = %d, .y = %d}, .p2 = {.x = %d, .y = %d}},\n' % (
                wall.start.x, wall.start.y, wall.end.x, wall.end.y)
        cstr += '};\n\n'

        cstr += 'circleFl_t bumpers[%d] = {\n' % len(self.bumpers)
        for bumper in self.bumpers:
            cstr += '  {.pos = {.x = %d, .y = %d}, .radius = %d},\n' % (
                bumper.cx, bumper.cy, bumper.r)
        cstr += '};\n\n'

        return cstr

    def parseSvgGroup(self, gList: list[svg_plain.g], parentIds: str):
        # TODO apply group transforms

        for gData in gList:
            id = parentIds + ':' + str.lower(gData.get_id())

            ellipse: svg_plain.ellipseType
            for ellipse in gData.get_ellipse():
                if 'bumpers' in id:
                    cx = float(ellipse.get_cx())
                    cy = float(ellipse.get_cy())
                    cr = float(ellipse.get_rx() +
                               float(ellipse.get_ry()) / 2)
                    self.bumpers.append(PinCircle(cx, cy, cr))

            circle: svg_plain.circleType
            for circle in gData.get_circle():
                if 'bumpers' in id:
                    cx = float(circle.get_cx())
                    cy = float(circle.get_cy())
                    cr = float(circle.get_r())
                    self.bumpers.append(PinCircle(cx, cy, cr))

            path: svg_plain.path
            for path in gData.get_path():
                if 'walls' in id:
                    self.walls.extend(svgPathParser().parsePath(path.get_d()))

            rect: svg_plain.rectType
            for rect in gData.get_rect():
                printWarning('Rect not handled')
                pass

            text: svg_plain.textType
            for text in gData.get_text():
                printWarning('Text not handled')
                pass

            # Recurse
            self.parseSvgGroup(gData.get_g(), id)


class svgPathParser:
    def __init__(self) -> None:
        self.currentPoint: PinPoint = None
        self.lines = []
        self.lineStarted = False
        self.startSubpaths = []

    def __str__(self) -> str:
        return ', '.join([str(x) for x in self.lines])

    def parseLineToCommand(self, parts: list[str], isMoveTo: bool, isAbsolute: bool) -> list[str]:
        moved: bool = False
        # Consume coordinates from list
        while 0 < len(parts) and 1 < len(parts[0]):
            # Pop the point from the list
            newPoint = PinPoint(x=float(parts.pop(0)), y=float(parts.pop(0)))

            # Find the new point, relative or absolute
            if isMoveTo:
                if not moved:
                    # Treat first move as absolute, always
                    moved = True
                    # Append it to the subpath stack
                    self.startSubpaths.append(newPoint)
                elif not isAbsolute:
                    newPoint.x += self.currentPoint.x
                    newPoint.y += self.currentPoint.y
            elif not isAbsolute:
                newPoint.x += self.currentPoint.x
                newPoint.y += self.currentPoint.y

            # Add a line segment if there are two points
            if self.currentPoint is not None:
                self.lines.append(PinLine(self.currentPoint, newPoint))

            # Set the current point to the new point
            self.currentPoint = newPoint

        # Return what's left
        return parts

    def parse1DLineToToCommand(self, parts: list[str], isAbsolute: bool, isHorizontal: bool) -> list[str]:
        # Consume floats from list
        while 0 < len(parts) and 1 < len(parts[0]):
            # Pop the point from the list
            newNum = float(parts.pop(0))

            # Find the new point, relative or absolute
            newPoint = PinPoint(p=self.currentPoint)
            if isHorizontal:
                if isAbsolute:
                    newPoint.x = newNum
                else:
                    newPoint.x += newNum
            else:
                if isAbsolute:
                    newPoint.y = newNum
                else:
                    newPoint.y += newNum

            # Add a line segment if there are two points
            if self.currentPoint is not None:
                self.lines.append(PinLine(self.currentPoint, newPoint))

            # Set the current point to the new point
            self.currentPoint = newPoint

        # Return what's left
        return parts

    def parseClosePathCommand(self, parts: list[str]) -> list[str]:
        self.lines.append(PinLine(self.currentPoint, self.startSubpaths.pop()))
        self.currentPoint = None
        return parts

    def parseUnsupportedCommand(self, parts: list[str], name: str) -> list[str]:

        printWarning(name + ' not supported')

        # Consume coordinates from list
        while 0 < len(parts) and 1 < len(parts[0]):
            # Pop the point from the list
            PinPoint(x=float(parts.pop(0)), y=float(parts.pop(0)))

        return parts

    def parsePath(self, pathData: str) -> list[PinLine]:
        # https://www.w3.org/TR/SVG/paths.html#DProperty
        parts = re.split(r'[ ,]+', pathData)
        while 0 < len(parts):
            match parts[0]:
                # Line commands
                case 'M' | 'm':
                    # Move To is basically the same as Line To, but resets the point first
                    parts = self.parseLineToCommand(
                        parts[1:], True, parts[0].isupper())
                case 'L' | 'l':
                    parts = self.parseLineToCommand(
                        parts[1:], False, parts[0].isupper())
                case 'H' | 'h':
                    parts = self.parse1DLineToToCommand(
                        parts[1:], parts[0].isupper(), True)
                case 'V' | 'v':
                    parts = self.parse1DLineToToCommand(
                        parts[1:], parts[0].isupper(), False)
                # Cubic Bezier commands
                case 'C' | 'c':
                    parts = self.parseUnsupportedCommand(
                        parts[1:], 'Curve')
                case 'S' | 's':
                    parts = self.parseUnsupportedCommand(
                        parts[1:], 'Smooth Curve')
                # Quadratic Bezier commands
                case 'Q' | 'q':
                    parts = self.parseUnsupportedCommand(
                        parts[1:], 'Quadratic Curve')
                case 'T' | 't':
                    parts = self.parseUnsupportedCommand(
                        parts[1:], 'Smooth Quadratic Curve')
                # Elliptical Arc commands
                case 'A' | 'a':
                    parts = self.parseUnsupportedCommand(
                        parts[1:], 'Elliptic Arc')
                # Close command
                case 'Z' | 'z':
                    parts = self.parseClosePathCommand(parts[1:])
                # Unknown
                case _: pass

        return self.lines


def main():
    with open('pinball.svg', 'r') as svgFile:
        data: svg_plain.svg = svg_plain.parse(svgFile, True, True)
        pinballTable: PinballTable = PinballTable()
        pinballTable.parseSvgGroup(data.get_g(), '')
        print(pinballTable.toCStr())


# Using the special variable
# __name__
if __name__ == "__main__":
    main()
