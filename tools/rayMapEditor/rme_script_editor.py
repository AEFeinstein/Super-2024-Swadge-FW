import re
from enum import Enum


class ifOpType(Enum):
    SHOOT_OBJS = 0,
    SHOOT_WALLS = 1,
    KILL = 2,
    ENTER = 3,
    GET = 4,
    TOUCH = 5,
    BUTTON_PRESSED = 6,
    TIME_ELAPSED = 7


class thenOpType(Enum):
    OPEN = 8,
    CLOSE = 9,
    SPAWN = 10,
    DESPAWN = 11,
    DIALOG = 12,
    WARP = 13,
    WIN = 14


class rme_script:
    def __init__(self) -> None:
        self.ifOp: ifOpType = None
        self.ifArgs = {}
        self.thenOp: thenOpType = None
        self.thenArgs = {}
        pass

    def toString(self) -> str:
        # TODO write toString()
        return 'IF ' + self.ifOp.name + '() THEN ' + self.thenOp.name + '()'

    def parseIdsOrder(self, if_args: str) -> bool:
        result = re.search(
            r'\[([\d, ]+)\]\s*,\s*(IN_ORDER|ANY_ORDER)', if_args)
        if result:
            ids: list[str] = re.split(r'[\s,]+', result.group(1))
            self.ifArgs['ids'] = [int(i) for i in ids]
            self.ifArgs['order'] = result.group(2)
            return True
        else:
            return False

    def fromString(self, if_op: str, if_args: str, then_op: str, then_args: str) -> bool:
        if ifOpType.SHOOT_OBJS.name == if_op:
            self.ifOp = ifOpType.SHOOT_OBJS
            if False == self.parseIdsOrder(if_args):
                return False

        elif ifOpType.SHOOT_WALLS.name == if_op:
            self.ifOp = ifOpType.SHOOT_WALLS
            result = re.search(
                r'\[([{\d, }]+)\]\s*,\s*(IN_ORDER|ANY_ORDER)', if_args)
            if result:
                cells: list[str] = re.split(r'[{}]+', result.group(1))
                self.ifArgs['cells'] = []
                for cell in cells:
                    try:
                        coordinates: list[str] = re.split(r'[\s,]+', cell)
                        self.ifArgs['cells'].append(
                            [int(coordinates[0]), int(coordinates[1])])
                    except:
                        pass
                self.ifArgs['order'] = result.group(2)
            else:
                return False

        elif ifOpType.KILL.name == if_op:
            self.ifOp = ifOpType.KILL
            if False == self.parseIdsOrder(if_args):
                return False

        elif ifOpType.ENTER.name == if_op:
            self.ifOp = ifOpType.ENTER

            result = re.search(
                r'{\s*(\d+)\s*,\s*(\d+)\s*}\s*,\s*\[([\d, ]+)\]', if_args)
            if result:
                self.ifArgs['cell'] = [
                    int(result.group(1)), int(result.group(2))]
                ids: list[str] = re.split(r'[\s,]+', result.group(3))
                self.ifArgs['ids'] = [int(i) for i in ids]
            else:
                return False

        elif ifOpType.GET.name == if_op:
            self.ifOp = ifOpType.GET
            result = re.search(r'\[([\d, ]+)\]', if_args)
            if result:
                ids: list[str] = re.split(r'[\s,]+', result.group(1))
                self.ifArgs['ids'] = [int(i) for i in ids]
            else:
                return False

        elif ifOpType.TOUCH.name == if_op:
            self.ifOp = ifOpType.TOUCH
            result = re.search(r'(\d+)', if_args)
            if result:
                self.ifArgs['ids'] = int(result.group(1))
            else:
                return False

        elif ifOpType.BUTTON_PRESSED.name == if_op:
            self.ifOp = ifOpType.BUTTON_PRESSED
            result = re.search(r'(\d+)', if_args)
            if result:
                self.ifArgs['btn'] = int(result.group(1))
            else:
                return False

        elif ifOpType.TIME_ELAPSED.name == if_op:
            self.ifOp = ifOpType.TIME_ELAPSED
            result = re.search(r'(\d+)', if_args)
            if result:
                self.ifArgs['tMs'] = int(result.group(1))
            else:
                return False

        else:
            return False

        if thenOpType.OPEN.name == then_op:
            # Set the operation
            self.thenOp = thenOpType.OPEN
            # Parse the arguments
            cells: list[str] = re.split(r'[{}]+', then_args)
            self.thenArgs['cells'] = []
            for cell in cells:
                try:
                    coordinates: list[str] = re.split(r'[\s,]+', cell)
                    self.thenArgs['cells'].append(
                        [int(coordinates[0]), int(coordinates[1])])
                except:
                    pass
            if 0 == len(self.thenArgs['cells']):
                return False

        elif thenOpType.CLOSE.name == then_op:
            # Set the operation
            self.thenOp = thenOpType.CLOSE
            # Parse the arguments
            cells: list[str] = re.split(r'[{}]+', then_args)
            self.thenArgs['cells'] = []
            for cell in cells:
                try:
                    coordinates: list[str] = re.split(r'[\s,]+', cell)
                    self.thenArgs['cells'].append(
                        [int(coordinates[0]), int(coordinates[1])])
                except:
                    pass
            if 0 == len(self.thenArgs['cells']):
                return False

        elif thenOpType.SPAWN.name == then_op:
            self.thenOp = thenOpType.SPAWN
            # TODO parse SPAWN

        elif thenOpType.DESPAWN.name == then_op:
            self.thenOp = thenOpType.DESPAWN
            # TODO parse DESPAWN

        elif thenOpType.DIALOG.name == then_op:
            # Set the operation
            self.thenOp = thenOpType.DIALOG
            # Parse the arguments
            if 0 == len(then_args):
                return False
            self.thenArgs['text'] = then_args

        elif thenOpType.WARP.name == then_op:
            # Set the operation
            self.thenOp = thenOpType.WARP
            # Parse the arguments
            cells: list[str] = re.split(r'[{}]+', then_args)
            self.thenArgs['cells'] = []
            for cell in cells:
                try:
                    coordinates: list[str] = re.split(r'[\s,]+', cell)
                    self.thenArgs['cell'] = [
                        int(coordinates[0]), int(coordinates[1])]
                    continue
                except:
                    pass
            if self.thenArgs['cells'] is None:
                return False

        elif thenOpType.WIN.name == then_op:
            # Set the operation
            self.thenOp = thenOpType.WIN
            # No arguments

        else:
            return False

        return True

    def toBytes(self) -> bytearray:
        # TODO write toBytes
        return None

    def fromBytes(self, bytes: bytearray):
        # TODO write fromBytes
        return


class scriptValidator:

    def __init__(self) -> None:
        argsRegex: str = '\(([A-Z0-9, \[\]\{\}_]*)\)'

        regex = "IF\s+("

        first = False
        for ifOp in ifOpType.__members__.items():
            if not first:
                first = True
            else:
                regex = regex + '|'
            regex = regex + ifOp[0]

        regex = regex + ")\s*"
        regex = regex + argsRegex
        regex = regex + '\s+THEN\s+('

        first = False
        for thenOp in thenOpType.__members__.items():
            if not first:
                first = True
            else:
                regex = regex + '|'
            regex = regex + thenOp[0]
        regex = regex + ')\s*'
        regex = regex + argsRegex

        self.pattern: re.Pattern = re.compile(regex, flags=re.IGNORECASE)
        pass

    def validateScript(self, scriptLine: str) -> bool:
        match = self.pattern.fullmatch(scriptLine)
        if None != match:
            if_op: str = match.group(1)
            if_args: str = match.group(2)
            then_op: str = match.group(3)
            then_args: str = match.group(4)

            script: rme_script = rme_script()
            return script.fromString(if_op, if_args, then_op, then_args)

        return False
