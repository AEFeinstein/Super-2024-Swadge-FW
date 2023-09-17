import re
from enum import Enum
from rme_tiles import tileType

# Argument keys
kCell: str = 'cell'
kCells: str = 'cells'
kId: str = 'id'
kIds: str = 'ids'
kSpawns: str = 'spawns'
kOrder: str = 'order'
kBtn: str = 'btn'
kTms: str = 'tMs'
kText: str = 'text'


class ifOpType(Enum):
    SHOOT_OBJS = 0
    SHOOT_WALLS = 1
    KILL = 2
    ENTER = 3
    GET = 4
    TOUCH = 5
    BUTTON_PRESSED = 6
    TIME_ELAPSED = 7


class thenOpType(Enum):
    OPEN = 8
    CLOSE = 9
    SPAWN = 10
    DESPAWN = 11
    DIALOG = 12
    WARP = 13
    WIN = 14


class orderType(Enum):
    IN_ORDER = 0
    ANY_ORDER = 1


class spawn:
    def __init__(self, type: tileType, id: int, x: int, y: int) -> None:
        self.type = type
        self.id = id
        self.x = x
        self.y = y


class rme_scriptSplitter:

    def __init__(self) -> None:
        argsRegex: str = '(\([A-Z0-9_,;\.\-\[\]\{\}\s]*\))'

        regex = 'IF\s+('

        first = False
        for ifOp in ifOpType.__members__.items():
            if not first:
                first = True
            else:
                regex = regex + '|'
            regex = regex + ifOp[0]

        regex = regex + ')\s*'
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

    def splitScript(self, scriptLine: str) -> list[str]:
        match = self.pattern.fullmatch(scriptLine)
        if None != match:
            return [match.group(1), match.group(2), match.group(3), match.group(4)]
        return None


class rme_script:
    def __init__(self, bytes: bytearray = None, string: str = None, splitter: rme_scriptSplitter = None) -> None:
        # Add members
        self.resetScript()
        # Parse depending on what args we get
        if bytes is not None:
            self.fromBytes(bytes)
        elif string is not None and splitter is not None:
            parts = splitter.splitScript(string)
            if parts is not None and len(parts) == 4:
                self.fromString(parts[0], parts[1], parts[2], parts[3])
            else:
                # Failure, reset the script
                self.resetScript()
        pass

    def __parseArgs(self, args: str) -> list[str]:
        # Args are in the form (a;b;c)
        result = re.match(r'\s*\((.*)\)\s*', args)
        if result:
            return [s.strip() for s in re.split(r';', result.group(1).strip())]
        return None

    def __parseArray(self, array: str) -> list[str]:
        # Arrays are in the form [a,b,c]
        result = re.match(r'\s*\[(.*)\]\s*', array)
        if result:
            return [s.strip() for s in re.split(r',', result.group(1).strip())]
        return None

    def __parseInt(self, integer: str) -> int:
        # Integers are integers
        return int(integer.strip())

    def __parseOrder(self, order: str) -> orderType:
        # Order is either IN_ORDER or ANY_ORDER
        for type in orderType.__members__.items():
            if order == type[0]:
                return type[1]
        return None

    def __parseCell(self, cell: str) -> list[int]:
        # Cells are in the form {a.b}
        result = re.match(r'{\s*(\d+)\s*\.\s*(\d+)\s*}', cell.strip())
        if result:
            return [int(result.group(1)), int(result.group(2))]
        return None

    def __parseSpawn(self, spawnStr: str) -> spawn:
        result = re.match(
            r'{\s*([a-zA-Z_]+)\s*-\s*(\d+)\s*-\s*(\d+)\s*\.\s*(\d+)\s*}', spawnStr.strip())
        if result:
            type: tileType = None
            for a in tileType:
                if a.name == 'OBJ_' + result.group(1):
                    type = a
                    break
            if type is not None:
                return spawn(type, int(result.group(2)), int(result.group(3)), int(result.group(4)))
        return None

    def __parseText(self, text: str) -> str:
        # Text is a string, not quoted
        return text.strip()

    def __isListNotValid(self, array: list) -> bool:
        return (array is None) or (0 == len(array)) or (None in array)

    def toString(self) -> str:

        # Stringify the if args
        ifArgArray = []
        if kCell in self.ifArgs.keys():
            ifArgArray.append(
                '{' + str(self.ifArgs[kCell][0]) + '.' + str(self.ifArgs[kCell][1]) + '}')
        if kIds in self.ifArgs.keys():
            ifArgArray.append('[' + ', '.join(str(e)
                              for e in self.ifArgs[kIds]) + ']')
        if kCells in self.ifArgs.keys():
            ifArgArray.append(
                '[' + ', '.join('{' + str(e[0]) + '.' + str(e[1]) + '}' for e in self.ifArgs[kCells]) + ']')
        if kOrder in self.ifArgs.keys():
            ifArgArray.append(self.ifArgs[kOrder].name)
        if kId in self.ifArgs.keys():
            ifArgArray.append(str(self.ifArgs[kId]))
        if kBtn in self.ifArgs.keys():
            ifArgArray.append(str(self.ifArgs[kBtn]))
        if kTms in self.ifArgs.keys():
            ifArgArray.append(str(self.ifArgs[kTms]))

        # Stringify the then args
        thenArgArray = []
        if kCells in self.thenArgs.keys():
            thenArgArray.append(
                '[' + ', '.join('{' + str(e[0]) + '.' + str(e[1]) + '}' for e in self.thenArgs[kCells]) + ']')
        if kCell in self.thenArgs.keys():
            thenArgArray.append(
                '{' + str(self.thenArgs[kCell][0]) + '.' + str(self.thenArgs[kCell][1]) + '}')
        if kIds in self.thenArgs.keys():
            thenArgArray.append('[' + ', '.join(str(e)
                                for e in self.thenArgs[kIds]) + ']')
        if kText in self.thenArgs.keys():
            thenArgArray.append(self.thenArgs[kText])
        if kSpawns in self.thenArgs.keys():
            thenArgArray.append(
                '[' + ', '.join('{' + e.type.name.removeprefix('OBJ_') + '-' + str(e.id) + '-' + str(e.x) + '.' + str(e.y) + '}' for e in self.thenArgs[kSpawns]) + ']')

        # Stitch it all together
        return 'IF ' + self.ifOp.name + '(' + '; '.join(ifArgArray) + ') THEN ' + self.thenOp.name + '(' + '; '.join(thenArgArray) + ')'

    def fromString(self, if_op: str, if_args: str, then_op: str, then_args: str) -> bool:
        try:
            # Split the args
            argParts = self.__parseArgs(if_args)

            # Parse the IF part
            if ifOpType.SHOOT_OBJS.name == if_op:
                # Set the operation type
                self.ifOp = ifOpType.SHOOT_OBJS
                # Parse the args
                self.ifArgs[kIds] = [self.__parseInt(
                    s) for s in self.__parseArray(argParts[0])]
                self.ifArgs[kOrder] = self.__parseOrder(argParts[1])
                # Validate the args
                if self.__isListNotValid(self.ifArgs[kIds]) or self.ifArgs[kOrder] is None:
                    self.resetScript()
                    return False

            elif ifOpType.SHOOT_WALLS.name == if_op:
                self.ifOp = ifOpType.SHOOT_WALLS
                # Parse the args
                self.ifArgs[kCells] = [self.__parseCell(
                    s) for s in self.__parseArray(argParts[0])]
                self.ifArgs[kOrder] = self.__parseOrder(argParts[1])
                # Validate the args
                if self.__isListNotValid(self.ifArgs[kCells]) or self.ifArgs[kOrder] is None:
                    self.resetScript()
                    return False

            elif ifOpType.KILL.name == if_op:
                # Set the operation type
                self.ifOp = ifOpType.KILL
                # Parse the args
                self.ifArgs[kIds] = [self.__parseInt(
                    s) for s in self.__parseArray(argParts[0])]
                self.ifArgs[kOrder] = self.__parseOrder(argParts[1])
                # Validate the args
                if self.__isListNotValid(self.ifArgs[kIds]) or self.ifArgs[kOrder] is None:
                    self.resetScript()
                    return False

            elif ifOpType.ENTER.name == if_op:
                self.ifOp = ifOpType.ENTER
                # Parse the args
                self.ifArgs[kCell] = self.__parseCell(argParts[0])
                self.ifArgs[kIds] = [self.__parseInt(
                    s) for s in self.__parseArray(argParts[1])]
                # Validate the args
                if self.__isListNotValid(self.ifArgs[kIds]) or self.ifArgs[kCell] is None:
                    self.resetScript()
                    return False

            elif ifOpType.GET.name == if_op:
                self.ifOp = ifOpType.GET
                # Parse the args
                self.ifArgs[kIds] = [self.__parseInt(
                    s) for s in self.__parseArray(argParts[0])]
                # Validate the args
                if self.__isListNotValid(self.ifArgs[kIds]):
                    self.resetScript()
                    return False

            elif ifOpType.TOUCH.name == if_op:
                self.ifOp = ifOpType.TOUCH
                # Parse the args
                self.ifArgs[kId] = self.__parseInt(argParts[0])
                # Validate the args
                if self.ifArgs[kId] is None:
                    return False

            elif ifOpType.BUTTON_PRESSED.name == if_op:
                self.ifOp = ifOpType.BUTTON_PRESSED
                # Parse the args
                self.ifArgs[kBtn] = self.__parseInt(argParts[0])
                # Validate the args
                if self.ifArgs[kBtn] is None:
                    return False

            elif ifOpType.TIME_ELAPSED.name == if_op:
                self.ifOp = ifOpType.TIME_ELAPSED
                # Parse the arg
                self.ifArgs[kTms] = self.__parseInt(argParts[0])
                # Validate the args
                if self.ifArgs[kTms] is None:
                    return False
            else:
                self.resetScript()
                return False

            # Split the args
            argParts = self.__parseArgs(then_args)

            # Parse the THEN part
            if thenOpType.OPEN.name == then_op:
                # Set the operation
                self.thenOp = thenOpType.OPEN
                # Parse the args
                self.thenArgs[kCells] = [self.__parseCell(
                    s) for s in self.__parseArray(argParts[0])]
                # Validate the args
                if self.__isListNotValid(self.thenArgs[kCells]):
                    self.resetScript()
                    return False

            elif thenOpType.CLOSE.name == then_op:
                # Set the operation
                self.thenOp = thenOpType.CLOSE
                # Parse the args
                self.thenArgs[kCells] = [self.__parseCell(
                    s) for s in self.__parseArray(argParts[0])]
                # Validate the args
                if self.__isListNotValid(self.thenArgs[kCells]):
                    self.resetScript()
                    return False

            elif thenOpType.SPAWN.name == then_op:
                self.thenOp = thenOpType.SPAWN
                # Parse the args
                self.thenArgs[kSpawns] = [self.__parseSpawn(
                    s) for s in self.__parseArray(argParts[0])]
                # Validate the args
                if self.__isListNotValid(self.thenArgs[kSpawns]):
                    self.resetScript()
                    return False

            elif thenOpType.DESPAWN.name == then_op:
                self.thenOp = thenOpType.DESPAWN
                # Parse the args
                self.thenArgs[kIds] = [self.__parseInt(
                    s) for s in self.__parseArray(argParts[0])]
                # Validate the args
                if self.__isListNotValid(self.thenArgs[kIds]):
                    self.resetScript()
                    return False

            elif thenOpType.DIALOG.name == then_op:
                # Set the operation
                self.thenOp = thenOpType.DIALOG
                # Parse the args
                self.thenArgs[kText] = self.__parseText(argParts[0])
                # Validate the args
                if self.thenArgs[kText] is None:
                    return False

            elif thenOpType.WARP.name == then_op:
                # Set the operation
                self.thenOp = thenOpType.WARP
                # Parse the args
                self.thenArgs[kCell] = self.__parseCell(argParts[0])
                # Validate the args
                if self.thenArgs[kCell] is None:
                    return False

            elif thenOpType.WIN.name == then_op:
                # Set the operation
                self.thenOp = thenOpType.WIN
                # No arguments

            else:
                self.resetScript()
                return False

            return True
        except Exception as e:
            # print(e)
            self.resetScript()
            return False

    def toBytes(self) -> bytearray:
        bytes: bytearray = bytearray()

        # Append the IF operation
        bytes.append(self.ifOp.value)

        # Append the IF arguments, order matters
        if kCell in self.ifArgs.keys():
            bytes.append(self.ifArgs[kCell][0])
            bytes.append(self.ifArgs[kCell][1])
        if kIds in self.ifArgs.keys():
            bytes.append(len(self.ifArgs[kIds]))
            for id in self.ifArgs[kIds]:
                bytes.append(id)
        if kCells in self.ifArgs.keys():
            bytes.append(len(self.ifArgs[kCells]))
            for cell in self.ifArgs[kCells]:
                bytes.append(cell[0])
                bytes.append(cell[1])
        if kOrder in self.ifArgs.keys():
            if orderType.IN_ORDER == self.ifArgs[kOrder]:
                bytes.append(0)
            elif orderType.ANY_ORDER == self.ifArgs[kOrder]:
                bytes.append(1)
        if kId in self.ifArgs.keys():
            bytes.append(self.ifArgs[kId])
        if kBtn in self.ifArgs.keys():
            bytes.append((self.ifArgs[kBtn] >> 8) & 255)
            bytes.append((self.ifArgs[kBtn] >> 0) & 255)
        if kTms in self.ifArgs.keys():
            bytes.append((self.ifArgs[kTms] >> 24) & 255)
            bytes.append((self.ifArgs[kTms] >> 16) & 255)
            bytes.append((self.ifArgs[kTms] >> 8) & 255)
            bytes.append((self.ifArgs[kTms] >> 0) & 255)

        # Append the ELSE operation
        bytes.append(self.thenOp.value)

        # Append the ELSE arguments, order matters
        if kCells in self.thenArgs.keys():
            bytes.append(len(self.thenArgs[kCells]))
            for cell in self.thenArgs[kCells]:
                bytes.append(cell[0])
                bytes.append(cell[1])
        if kCell in self.thenArgs.keys():
            bytes.append(self.thenArgs[kCell][0])
            bytes.append(self.thenArgs[kCell][1])
        if kIds in self.thenArgs.keys():
            bytes.append(len(self.thenArgs[kIds]))
            for id in self.thenArgs[kIds]:
                bytes.append(id)
        if kText in self.thenArgs.keys():
            bytes.append(len(self.thenArgs[kText]))
            bytes.extend(self.thenArgs[kText].encode())
        if kSpawns in self.thenArgs.keys():
            bytes.append(len(self.thenArgs[kSpawns]))
            for spawnObj in self.thenArgs[kSpawns]:
                bytes.append(spawnObj.type.value)
                bytes.append(spawnObj.id)
                bytes.append(spawnObj.x)
                bytes.append(spawnObj.y)

        return bytes

    def fromBytes(self, bytes: bytearray):

        # Index to read bytes
        idx: int = 0

        # Read the if operation
        self.ifOp = ifOpType._value2member_map_[bytes[idx]]
        idx = idx + 1

        # Read the if args
        if self.ifOp == ifOpType.SHOOT_OBJS:
            # Read IDs
            numIds: int = bytes[idx]
            idx = idx + 1
            self.ifArgs[kIds] = []
            for id in range(numIds):
                self.ifArgs[kIds].append(bytes[idx])
                idx = idx + 1
            # Read order
            self.ifArgs[kOrder] = orderType._value2member_map_[bytes[idx]]
            idx = idx + 1
        elif self.ifOp == ifOpType.SHOOT_WALLS:
            # Read Cells
            numCells: int = bytes[idx]
            idx = idx + 1
            self.ifArgs[kCells] = []
            for id in range(numCells):
                self.ifArgs[kCells].append([bytes[idx], bytes[idx + 1]])
                idx = idx + 2
            # Read order
            self.ifArgs[kOrder] = orderType._value2member_map_[bytes[idx]]
            idx = idx + 1
            pass
        elif self.ifOp == ifOpType.KILL:
            # Read the IDs
            numIds: int = bytes[idx]
            idx = idx + 1
            self.ifArgs[kIds] = []
            for id in range(numIds):
                self.ifArgs[kIds].append(bytes[idx])
                idx = idx + 1
            # Read order
            self.ifArgs[kOrder] = orderType._value2member_map_[bytes[idx]]
            idx = idx + 1
        elif self.ifOp == ifOpType.ENTER:
            # Read the cell
            self.ifArgs[kCell] = [bytes[idx], bytes[idx + 1]]
            idx = idx + 2
            # Read IDs
            numIds: int = bytes[idx]
            idx = idx + 1
            self.ifArgs[kIds] = []
            for id in range(numIds):
                self.ifArgs[kIds].append(bytes[idx])
                idx = idx + 1
        elif self.ifOp == ifOpType.GET:
            # Read IDs
            numIds: int = bytes[idx]
            idx = idx + 1
            self.ifArgs[kIds] = []
            for id in range(numIds):
                self.ifArgs[kIds].append(bytes[idx])
                idx = idx + 1
        elif self.ifOp == ifOpType.TOUCH:
            self.ifArgs[kId] = bytes[idx]
            idx = idx + 1
        elif self.ifOp == ifOpType.BUTTON_PRESSED:
            self.ifArgs[kBtn] = \
                (bytes[idx + 0] << 8) + \
                (bytes[idx + 1])
            idx = idx + 2
        elif self.ifOp == ifOpType.TIME_ELAPSED:
            self.ifArgs[kTms] = \
                (bytes[idx + 0] << 24) + \
                (bytes[idx + 1] << 16) + \
                (bytes[idx + 2] << 8) + \
                (bytes[idx + 3])
            idx = idx + 4
        else:
            self.resetScript()
            return

        # Read the then operation
        self.thenOp = thenOpType._value2member_map_[bytes[idx]]
        idx = idx + 1

        # Read the then args
        if self.thenOp == thenOpType.OPEN:
            # Read Cells
            numCells: int = bytes[idx]
            idx = idx + 1
            self.thenArgs[kCells] = []
            for id in range(numCells):
                self.thenArgs[kCells].append([bytes[idx], bytes[idx + 1]])
                idx = idx + 2
        elif self.thenOp == thenOpType.CLOSE:
            # Read Cells
            numCells: int = bytes[idx]
            idx = idx + 1
            self.thenArgs[kCells] = []
            for id in range(numCells):
                self.thenArgs[kCells].append([bytes[idx], bytes[idx + 1]])
                idx = idx + 2
        elif self.thenOp == thenOpType.SPAWN:
            # Read spawns
            numSpawns: int = bytes[idx]
            idx = idx + 1
            self.thenArgs[kSpawns] = []
            for sp in range(numSpawns):
                self.thenArgs[kSpawns].append(spawn(tileType._value2member_map_[
                                              bytes[idx]], bytes[idx + 1], bytes[idx + 2], bytes[idx + 3]))
                idx = idx + 4
            pass
        elif self.thenOp == thenOpType.DESPAWN:
            # Read IDs
            numIds: int = bytes[idx]
            idx = idx + 1
            self.thenArgs[kIds] = []
            for id in range(numIds):
                self.thenArgs[kIds].append(bytes[idx])
                idx = idx + 1
        elif self.thenOp == thenOpType.DIALOG:
            # Read Text
            textLen: int = bytes[idx]
            idx = idx + 1
            self.thenArgs[kText] = str(bytes[idx:idx + textLen], 'ascii')
        elif self.thenOp == thenOpType.WARP:
            # Read the cell
            self.thenArgs[kCell] = [bytes[idx], bytes[idx + 1]]
            idx = idx + 2
        elif self.thenOp == thenOpType.WIN:
            # No args
            pass
        else:
            self.resetScript()
            return

        return

    def isValid(self) -> bool:
        return self.ifOp is not None and self.thenOp is not None

    def resetScript(self) -> None:
        self.ifOp: ifOpType = None
        self.ifArgs = {}
        self.thenOp: thenOpType = None
        self.thenArgs = {}
