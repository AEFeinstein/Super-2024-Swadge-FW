
from enum import Enum


class tileType(Enum):
    EMPTY = 0
    BG_FLOOR = 1
    BG_WALL = 2
    BG_CEILING = 3
    BG_DOOR = 4
    OBJ_START_POINT = 5
    OBJ_ENEMY_DRAGON = 6
    OBJ_ENEMY_SKELETON = 7
    OBJ_ENEMY_KNIGHT = 8
    OBJ_ENEMY_GOLEM = 9
    OBJ_OBELISK = 10
    OBJ_GUN = 11
    OBJ_DELETE = 12  # Should be last


bgTiles: list[tileType] = [
    tileType.BG_FLOOR,
    tileType.BG_WALL,
    tileType.BG_CEILING,
    tileType.BG_DOOR
]

objTiles: list[tileType] = [
    tileType.OBJ_START_POINT,
    tileType.OBJ_ENEMY_DRAGON,
    tileType.OBJ_ENEMY_SKELETON,
    tileType.OBJ_ENEMY_KNIGHT,
    tileType.OBJ_ENEMY_GOLEM,
    tileType.OBJ_OBELISK,
    tileType.OBJ_GUN,
    tileType.OBJ_DELETE
]


class tile:
    def __init__(self):
        self.background: tileType = tileType.BG_FLOOR
        self.object: tileType = tileType.EMPTY
        self.objectId: int = -1

    def setBg(self, bg: tileType):
        self.background = bg

    def setObj(self, obj: tileType, id: int):
        self.object = obj
        self.objectId = id
