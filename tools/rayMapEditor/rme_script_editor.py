import re
from enum import Enum
from rme_tiles import tileType

# Argument keys
kMap: str = 'map'
kCell: str = 'cell'
kCells: str = 'cells'
kIds: str = 'ids'
kSpawns: str = 'spawns'
kOrder: str = 'order'
kTms: str = 'tMs'
kText: str = 'text'
kAndOr: str = 'andor'
kOneTime: str = 'onetime'


class ifOpType(Enum):
    SHOOT_OBJS = 0
    KILL = 1
    GET = 2
    TOUCH = 3
    SHOOT_WALLS = 4
    ENTER = 5
    TIME_ELAPSED = 6


class thenOpType(Enum):
    OPEN = 7
    CLOSE = 8
    SPAWN = 9
    DESPAWN = 10
    DIALOG = 11
    WARP = 12
    WIN = 13


class orderType(Enum):
    IN_ORDER = 0
    ANY_ORDER = 1


class andOrType(Enum):
    AND = 0
    OR = 1


class oneTimeType(Enum):
    ONCE = 0
    ALWAYS = 1


class spawn:
    def __init__(self, type: tileType, id: int, x: int, y: int) -> None:
        self.type = type
        self.id = id
        self.x = x
        self.y = y


class rme_scriptSplitter:

    def __init__(self) -> None:
        argsRegex: str = '(\([^\(\)]*\))'

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

    def __parseAndOr(self, order: str) -> andOrType:
        # Either AND or OR
        for type in andOrType.__members__.items():
            if order == type[0]:
                return type[1]
        return None

    def __parseOneTime(self, order: str) -> oneTimeType:
        # Either ONCE or ALWAYS
        for type in oneTimeType.__members__.items():
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
            try:
                type = tileType[result.group(1)]
                return spawn(type, int(result.group(2)), int(result.group(3)), int(result.group(4)))
            except:
                return None
        return None

    def __parseText(self, text: str) -> str:
        # Text is a string, not quoted
        return text.strip()

    def __isListNotValid(self, array: list) -> bool:
        return (array is None) or (0 == len(array)) or (None in array)

    def toString(self) -> str:

        # Stringify the if args
        ifArgArray = []
        if kAndOr in self.ifArgs.keys():
            ifArgArray.append(str(self.ifArgs[kAndOr].name))
        if kIds in self.ifArgs.keys():
            ifArgArray.append('[' + ', '.join(str(e)
                              for e in self.ifArgs[kIds]) + ']')
        if kCells in self.ifArgs.keys():
            ifArgArray.append(
                '[' + ', '.join('{' + str(e[0]) + '.' + str(e[1]) + '}' for e in self.ifArgs[kCells]) + ']')
        if kOrder in self.ifArgs.keys():
            ifArgArray.append(self.ifArgs[kOrder].name)
        if kOneTime in self.ifArgs.keys():
            ifArgArray.append(self.ifArgs[kOneTime].name)
        if kTms in self.ifArgs.keys():
            ifArgArray.append(str(self.ifArgs[kTms]))

        # Stringify the then args
        thenArgArray = []
        if kCells in self.thenArgs.keys():
            thenArgArray.append(
                '[' + ', '.join('{' + str(e[0]) + '.' + str(e[1]) + '}' for e in self.thenArgs[kCells]) + ']')
        if kSpawns in self.thenArgs.keys():
            thenArgArray.append('[' + ', '.join('{' + e.type.name + '-' + str(e.id) + '-' + str(
                e.x) + '.' + str(e.y) + '}' for e in self.thenArgs[kSpawns]) + ']')
        if kIds in self.thenArgs.keys():
            thenArgArray.append('[' + ', '.join(str(e)
                                for e in self.thenArgs[kIds]) + ']')
        if kText in self.thenArgs.keys():
            thenArgArray.append(self.thenArgs[kText])
        if kMap in self.thenArgs.keys():
            thenArgArray.append(str(self.thenArgs[kMap]))
        if kCell in self.thenArgs.keys():
            thenArgArray.append(
                '{' + str(self.thenArgs[kCell][0]) + '.' + str(self.thenArgs[kCell][1]) + '}')

        # Stitch it all together
        return 'IF ' + self.ifOp.name + '(' + '; '.join(ifArgArray) + ') THEN ' + self.thenOp.name + '(' + '; '.join(thenArgArray) + ')'

    def fromString(self, if_op: str, if_args: str, then_op: str, then_args: str) -> bool:
        try:
            # Split the args
            argParts = self.__parseArgs(if_args)

            # Set the operation type
            self.ifOp = ifOpType[if_op]
            if (ifOpType.SHOOT_OBJS == self.ifOp) or (ifOpType.KILL == self.ifOp) or \
                    (ifOpType.GET == self.ifOp) or (ifOpType.TOUCH == self.ifOp):
                # Parse the args
                self.ifArgs[kAndOr] = self.__parseAndOr(argParts[0])
                self.ifArgs[kIds] = [self.__parseInt(
                    s) for s in self.__parseArray(argParts[1])]
                self.ifArgs[kOrder] = self.__parseOrder(argParts[2])
                self.ifArgs[kOneTime] = self.__parseOneTime(argParts[3])
                # Validate the args
                if (self.ifArgs[kAndOr] is None) or self.__isListNotValid(self.ifArgs[kIds]) or \
                        (self.ifArgs[kOrder] is None) or (self.ifArgs[kOneTime] is None):
                    self.resetScript()
                    return False
            elif (ifOpType.SHOOT_WALLS == self.ifOp) or (ifOpType.ENTER == self.ifOp):
                # Parse the args
                self.ifArgs[kAndOr] = self.__parseAndOr(argParts[0])
                self.ifArgs[kCells] = [self.__parseCell(
                    s) for s in self.__parseArray(argParts[1])]
                self.ifArgs[kOrder] = self.__parseOrder(argParts[2])
                self.ifArgs[kOneTime] = self.__parseOneTime(argParts[3])
                # Validate the args
                if (self.ifArgs[kAndOr] is None) or self.__isListNotValid(self.ifArgs[kCells]) or \
                        (self.ifArgs[kOrder] is None) or (self.ifArgs[kOneTime] is None):
                    self.resetScript()
                    return False
            elif (ifOpType.TIME_ELAPSED == self.ifOp):
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
            self.thenOp = thenOpType[then_op]
            if (thenOpType.OPEN == self.thenOp) or (thenOpType.CLOSE == self.thenOp):
                # Parse the args
                self.thenArgs[kCells] = [self.__parseCell(
                    s) for s in self.__parseArray(argParts[0])]
                # Validate the args
                if self.__isListNotValid(self.thenArgs[kCells]):
                    self.resetScript()
                    return False

            elif thenOpType.SPAWN == self.thenOp:
                # Parse the args
                self.thenArgs[kSpawns] = [self.__parseSpawn(
                    s) for s in self.__parseArray(argParts[0])]
                # Validate the args
                if self.__isListNotValid(self.thenArgs[kSpawns]):
                    self.resetScript()
                    return False

            elif thenOpType.DESPAWN == self.thenOp:
                # Parse the args
                self.thenArgs[kIds] = [self.__parseInt(
                    s) for s in self.__parseArray(argParts[0])]
                # Validate the args
                if self.__isListNotValid(self.thenArgs[kIds]):
                    self.resetScript()
                    return False

            elif thenOpType.DIALOG == self.thenOp:
                # Parse the args
                self.thenArgs[kText] = self.__parseText(argParts[0])
                # Validate the args
                if self.thenArgs[kText] is None:
                    return False

            elif thenOpType.WARP == self.thenOp:
                # Parse the args
                self.thenArgs[kMap] = self.__parseInt(argParts[0])
                self.thenArgs[kCell] = self.__parseCell(argParts[1])
                # Validate the args
                if (self.thenArgs[kMap] is None) or (self.thenArgs[kCell] is None):
                    return False

            elif thenOpType.WIN == self.thenOp:
                # No arguments
                pass

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
        if kAndOr in self.ifArgs.keys():
            bytes.append(self.ifArgs[kAndOr].value)
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
            bytes.append(self.ifArgs[kOrder].value)
        if kOneTime in self.ifArgs.keys():
            bytes.append(self.ifArgs[kOneTime].value)
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
        if kSpawns in self.thenArgs.keys():
            bytes.append(len(self.thenArgs[kSpawns]))
            for spawnObj in self.thenArgs[kSpawns]:
                bytes.append(spawnObj.type.value)
                bytes.append(spawnObj.id)
                bytes.append(spawnObj.x)
                bytes.append(spawnObj.y)
        if kIds in self.thenArgs.keys():
            bytes.append(len(self.thenArgs[kIds]))
            for id in self.thenArgs[kIds]:
                bytes.append(id)
        if kText in self.thenArgs.keys():
            textLen = len(self.thenArgs[kText])
            bytes.append((textLen >> 8) & 255)
            bytes.append((textLen >> 0) & 255)
            bytes.extend(self.thenArgs[kText].encode())
        if kMap in self.thenArgs.keys():
            bytes.append(self.thenArgs[kMap])
        if kCell in self.thenArgs.keys():
            bytes.append(self.thenArgs[kCell][0])
            bytes.append(self.thenArgs[kCell][1])

        return bytes

    def fromBytes(self, bytes: bytearray):

        # Index to read bytes
        idx: int = 0

        # Read the if operation
        self.ifOp = ifOpType._value2member_map_[bytes[idx]]
        idx = idx + 1

        # Read the if args
        if (ifOpType.SHOOT_OBJS == self.ifOp) or (ifOpType.KILL == self.ifOp) or \
                (ifOpType.GET == self.ifOp) or (ifOpType.TOUCH == self.ifOp):
            # Read and/or
            self.ifArgs[kAndOr] = andOrType._value2member_map_[bytes[idx]]
            idx = idx + 1
            # Read number of IDs
            numIds: int = bytes[idx]
            idx = idx + 1
            # Read IDs
            self.ifArgs[kIds] = []
            for id in range(numIds):
                self.ifArgs[kIds].append(bytes[idx])
                idx = idx + 1
            # Read order
            self.ifArgs[kOrder] = orderType._value2member_map_[bytes[idx]]
            idx = idx + 1
            # Read one time
            self.ifArgs[kOneTime] = oneTimeType._value2member_map_[bytes[idx]]
            idx = idx + 1
        elif (ifOpType.SHOOT_WALLS == self.ifOp) or (ifOpType.ENTER == self.ifOp):
            # Read and/or
            self.ifArgs[kAndOr] = andOrType._value2member_map_[bytes[idx]]
            idx = idx + 1
            # Read number of cells
            numCells: int = bytes[idx]
            idx = idx + 1
            # Read cells
            self.ifArgs[kCells] = []
            for cell in range(numCells):
                self.ifArgs[kCells].append([bytes[idx], bytes[idx + 1]])
                idx = idx + 2
            # Read order
            self.ifArgs[kOrder] = orderType._value2member_map_[bytes[idx]]
            idx = idx + 1
            # Read one time
            self.ifArgs[kOneTime] = oneTimeType._value2member_map_[bytes[idx]]
            idx = idx + 1
        elif ifOpType.TIME_ELAPSED == self.ifOp:
            # Read the time
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
        if (thenOpType.OPEN == self.thenOp) or (thenOpType.CLOSE == self.thenOp):
            # Read Number of cells
            numCells: int = bytes[idx]
            idx = idx + 1
            # Read cells
            self.thenArgs[kCells] = []
            for cell in range(numCells):
                self.thenArgs[kCells].append([bytes[idx], bytes[idx + 1]])
                idx = idx + 2
        elif thenOpType.SPAWN == self.thenOp:
            # Read number of spawns
            numSpawns: int = bytes[idx]
            idx = idx + 1
            # Read spawns
            self.thenArgs[kSpawns] = []
            for sp in range(numSpawns):
                self.thenArgs[kSpawns].append(spawn(tileType._value2member_map_[
                                              bytes[idx]], bytes[idx + 1], bytes[idx + 2], bytes[idx + 3]))
                idx = idx + 4
            pass
        elif thenOpType.DESPAWN == self.thenOp:
            # Read number of IDs
            numIds: int = bytes[idx]
            idx = idx + 1
            # Read IDs
            self.thenArgs[kIds] = []
            for id in range(numIds):
                self.thenArgs[kIds].append(bytes[idx])
                idx = idx + 1
        elif thenOpType.DIALOG == self.thenOp:
            # Read length of text
            textLen: int = \
                (bytes[idx + 0] << 8) + \
                (bytes[idx + 1])
            idx = idx + 2
            # Read text
            self.thenArgs[kText] = str(bytes[idx:idx + textLen], 'ascii')
            idx = idx + textLen
        elif thenOpType.WARP == self.thenOp:
            # Read the map
            self.thenArgs[kMap] = bytes[idx]
            idx = idx + 1
            # Read the cell
            self.thenArgs[kCell] = [bytes[idx], bytes[idx + 1]]
            idx = idx + 2
        elif thenOpType.WIN == self.thenOp:
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
