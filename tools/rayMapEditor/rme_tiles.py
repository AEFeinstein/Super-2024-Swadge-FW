
from enum import Enum


class tileType(Enum):
    EMPTY = 0
    BG_FLOOR = 1
    BG_WALL = 2
    BG_CEILING = 3
    BG_DOOR = 4
    OBJ_START_POINT = 5
    OBJ_ENEMY = 6
    OBJ_OBELISK = 7
    OBJ_GUN = 9


bgTiles: list[tileType] = [
    tileType.BG_FLOOR,
    tileType.BG_WALL,
    tileType.BG_CEILING,
    tileType.BG_DOOR
]

objTiles: list[tileType] = [
    tileType.OBJ_START_POINT,
    tileType.OBJ_ENEMY,
    tileType.OBJ_OBELISK,
    tileType.OBJ_GUN,
]


class tile:
    def __init__(self):
        self.background: tileType = tileType.BG_FLOOR
        self.object: tileType = tileType.EMPTY
        self.objectId: int = 0

    def setBg(self, bg: tileType):
        self.background = bg

    def setObj(self, obj: tileType):
        self.object = obj
