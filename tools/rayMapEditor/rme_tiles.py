
from enum import Enum


class tileType(Enum):
    # Special empty type
    EMPTY = 0
    # Background tiles
    BG_FLOOR = 1
    BG_FLOOR_WATER = 2
    BG_FLOOR_LAVA = 3
    BG_WALL_1 = 4
    BG_WALL_2 = 5
    BG_WALL_3 = 6
    BG_DOOR = 7
    BG_DOOR_CHARGE = 8
    BG_DOOR_MISSILE = 9
    BG_DOOR_ICE = 10
    BG_DOOR_XRAY = 11
    # Meta tile, starting point
    OBJ_START_POINT = 12
    # Enemies
    OBJ_ENEMY_BEAM = 13
    OBJ_ENEMY_CHARGE = 14
    OBJ_ENEMY_MISSILE = 15
    OBJ_ENEMY_ICE = 16
    OBJ_ENEMY_XRAY = 17
    # Power-ups
    OBJ_ITEM_BEAM = 18
    OBJ_ITEM_CHARGE_BEAM = 19
    OBJ_ITEM_MISSILE = 20
    OBJ_ITEM_ICE = 21
    OBJ_ITEM_XRAY = 22
    OBJ_ITEM_SUIT_WATER = 23
    OBJ_ITEM_SUIT_LAVA = 24
    OBJ_ITEM_ENERGY_TANK = 25
    # Permanent non-power-items
    OBJ_ITEM_KEY = 26
    OBJ_ITEM_ARTIFACT = 27
    # Transient items
    OBJ_ITEM_PICKUP_ENERGY = 28
    OBJ_ITEM_PICKUP_MISSILE = 29
    # Scenery
    OBJ_SCENERY_TERMINAL = 30
    # Special delete tile
    OBJ_DELETE = 31

bgTiles: list[tileType] = [
    tileType.BG_FLOOR,
    tileType.BG_FLOOR_WATER,
    tileType.BG_FLOOR_LAVA,
    tileType.BG_WALL_1,
    tileType.BG_WALL_2,
    tileType.BG_WALL_3,
    tileType.BG_DOOR,
    tileType.BG_DOOR_CHARGE,
    tileType.BG_DOOR_MISSILE,
    tileType.BG_DOOR_ICE,
    tileType.BG_DOOR_XRAY
]

objTiles: list[tileType] = [
    tileType.OBJ_START_POINT,
    tileType.OBJ_ENEMY_BEAM,
    tileType.OBJ_ENEMY_CHARGE,
    tileType.OBJ_ENEMY_MISSILE,
    tileType.OBJ_ENEMY_ICE,
    tileType.OBJ_ENEMY_XRAY,
    tileType.OBJ_ITEM_BEAM,
    tileType.OBJ_ITEM_CHARGE_BEAM,
    tileType.OBJ_ITEM_MISSILE,
    tileType.OBJ_ITEM_ICE,
    tileType.OBJ_ITEM_XRAY,
    tileType.OBJ_ITEM_SUIT_WATER,
    tileType.OBJ_ITEM_SUIT_LAVA,
    tileType.OBJ_ITEM_ENERGY_TANK,
    tileType.OBJ_ITEM_KEY,
    tileType.OBJ_ITEM_ARTIFACT,
    tileType.OBJ_ITEM_PICKUP_ENERGY,
    tileType.OBJ_ITEM_PICKUP_MISSILE,
    tileType.OBJ_SCENERY_TERMINAL,
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
