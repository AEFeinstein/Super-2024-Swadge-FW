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

class script:
    def __init__(self) -> None:
        self.ifOp: ifOpType = None
        self.ifArgs = []
        self.thenOp: thenOpType = None
        self.thenArgs = []
        pass

    def toString(self) -> str:
        # TODO
        return ''
    
    def fromString(self, str)-> None:
        # TODO
        return
    
    def toBytes(self) -> bytearray:
        # TODO
        return None
    
    def fromBytes(self, bytes: bytearray):
        # TODO
        return

class scriptValidator:

    def __init__(self) -> None:
        argsRegex: str = '(\([A-Z0-9, \[\]\{\}_]*\))'

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

            if ifOpType.SHOOT_OBJS.name == if_op:
                x = 0
            elif ifOpType.SHOOT_WALLS.name == if_op:
                x = 0
            elif ifOpType.KILL.name == if_op:
                x = 0
            elif ifOpType.ENTER.name == if_op:
                x = 0
            elif ifOpType.GET.name == if_op:
                x = 0
            elif ifOpType.TOUCH.name == if_op:
                x = 0
            elif ifOpType.BUTTON_PRESSED.name == if_op:
                x = 0
            elif ifOpType.TIME_ELAPSED.name == if_op:
                x = 0
            else:
                return False

            if thenOpType.OPEN.name == then_op:
                x = 0
            elif thenOpType.CLOSE.name == then_op:
                x = 0
            elif thenOpType.SPAWN.name == then_op:
                x = 0
            elif thenOpType.DESPAWN.name == then_op:
                x = 0
            elif thenOpType.DIALOG.name == then_op:
                x = 0
            elif thenOpType.WARP.name == then_op:
                x = 0
            elif thenOpType.WIN.name == then_op:
                x = 0
            else:
                return False
            return True
        return False
