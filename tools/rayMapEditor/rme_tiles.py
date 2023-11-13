
from enum import Enum

# Bits used for tile type construction, topmost bit
BG      = 0x00
OBJ     = 0x80
# Types of background, next two top bits
META    = 0x00
FLOOR   = 0x20
WALL    = 0x40
DOOR    = 0x60
# Types of objects, next two top bits
ITEM    = 0x00
ENEMY   = 0x20
BULLET  = 0x40
SCENERY = 0x60
# Bottom five bits are used for uniqueness

class tileType(Enum):
    # Special empty type
    EMPTY           = (BG | META | 0)
    # Special delete tile
    DELETE          = (BG | META | 1)
    # Background tiles
    BG_FLOOR        = (BG | FLOOR | 1)
    BG_FLOOR_WATER  = (BG | FLOOR | 2)
    BG_FLOOR_LAVA   = (BG | FLOOR | 3)
    BG_CEILING      = (BG | FLOOR | 4)
    BG_FLOOR_HEAL   = (BG | FLOOR | 5)
    BG_WALL_1       = (BG | WALL  | 1)
    BG_WALL_2       = (BG | WALL  | 2)
    BG_WALL_3       = (BG | WALL  | 3)
    BG_WALL_4       = (BG | WALL  | 4)
    BG_WALL_5       = (BG | WALL  | 5)
    BG_DOOR         = (BG | DOOR  | 1)
    BG_DOOR_CHARGE  = (BG | DOOR  | 2)
    BG_DOOR_MISSILE = (BG | DOOR  | 3)
    BG_DOOR_ICE     = (BG | DOOR  | 4)
    BG_DOOR_XRAY    = (BG | DOOR  | 5)
    BG_DOOR_SCRIPT  = (BG | DOOR  | 6)
    BG_DOOR_KEY_A   = (BG | DOOR  | 7)
    BG_DOOR_KEY_B   = (BG | DOOR  | 8)
    BG_DOOR_KEY_C   = (BG | DOOR  | 9)
    BG_DOOR_ARTIFACT = (BG | DOOR | 10)
    # Self and Enemies
    OBJ_ENEMY_START_POINT = (OBJ | ENEMY | 1)
    OBJ_ENEMY_NORMAL      = (OBJ | ENEMY | 2)
    OBJ_ENEMY_STRONG      = (OBJ | ENEMY | 3)
    OBJ_ENEMY_ARMORED     = (OBJ | ENEMY | 4)
    OBJ_ENEMY_FLAMING     = (OBJ | ENEMY | 5)
    OBJ_ENEMY_HIDDEN      = (OBJ | ENEMY | 6)
    OBJ_ENEMY_BOSS        = (OBJ | ENEMY | 7)
    # Power-ups
    OBJ_ITEM_BEAM        = (OBJ | ITEM | 1)
    OBJ_ITEM_CHARGE_BEAM = (OBJ | ITEM | 2)
    OBJ_ITEM_MISSILE     = (OBJ | ITEM | 3)
    OBJ_ITEM_ICE         = (OBJ | ITEM | 4)
    OBJ_ITEM_XRAY        = (OBJ | ITEM | 5)
    OBJ_ITEM_SUIT_WATER  = (OBJ | ITEM | 6)
    OBJ_ITEM_SUIT_LAVA   = (OBJ | ITEM | 7)
    OBJ_ITEM_ENERGY_TANK = (OBJ | ITEM | 8)
    # Permanent non-power-items
    OBJ_ITEM_KEY_A       = (OBJ | ITEM |  9)
    OBJ_ITEM_KEY_B       = (OBJ | ITEM | 10)
    OBJ_ITEM_KEY_C       = (OBJ | ITEM | 11)
    OBJ_ITEM_ARTIFACT    = (OBJ | ITEM | 12)
    # Transient items
    OBJ_ITEM_PICKUP_ENERGY  = (OBJ | ITEM | 13)
    OBJ_ITEM_PICKUP_MISSILE = (OBJ | ITEM | 14)
    # Bullets
    OBJ_BULLET_NORMAL    = (OBJ | BULLET | 1)
    OBJ_BULLET_CHARGE    = (OBJ | BULLET | 2)
    OBJ_BULLET_ICE       = (OBJ | BULLET | 3)
    OBJ_BULLET_MISSILE   = (OBJ | BULLET | 4)
    OBJ_BULLET_XRAY      = (OBJ | BULLET | 5)
    OBJ_BULLET_E_NORMAL  = (OBJ | BULLET | 6)
    OBJ_BULLET_E_STRONG  = (OBJ | BULLET | 7)
    OBJ_BULLET_E_ARMOR   = (OBJ | BULLET | 8)
    OBJ_BULLET_E_FLAMING = (OBJ | BULLET | 9)
    OBJ_BULLET_E_HIDDEN  = (OBJ | BULLET | 10)
    # Scenery
    OBJ_SCENERY_TERMINAL = (OBJ | SCENERY | 1)
    OBJ_SCENERY_PORTAL   = (OBJ | SCENERY | 2)
    OBJ_SCENERY_F1       = (OBJ | SCENERY | 3)
    OBJ_SCENERY_F2       = (OBJ | SCENERY | 4)
    OBJ_SCENERY_F3       = (OBJ | SCENERY | 5)
    OBJ_SCENERY_F4       = (OBJ | SCENERY | 6)
    OBJ_SCENERY_F5       = (OBJ | SCENERY | 7)
    OBJ_SCENERY_F6       = (OBJ | SCENERY | 8)
    OBJ_SCENERY_F7       = (OBJ | SCENERY | 9)

bgTiles: list[list[tileType]] = [
    [
        tileType.BG_FLOOR,
        tileType.BG_FLOOR_WATER,
        tileType.BG_FLOOR_LAVA,
        tileType.BG_FLOOR_HEAL,
        tileType.BG_WALL_1,
        tileType.BG_WALL_2,
        tileType.BG_WALL_3,
        tileType.BG_WALL_4,
        tileType.BG_WALL_5
    ],
    [
        tileType.BG_DOOR,
        tileType.BG_DOOR_CHARGE,
        tileType.BG_DOOR_MISSILE,
        tileType.BG_DOOR_ICE,
        tileType.BG_DOOR_XRAY,
        tileType.BG_DOOR_SCRIPT,
        tileType.BG_DOOR_KEY_A,
        tileType.BG_DOOR_KEY_B,
        tileType.BG_DOOR_KEY_C,
        tileType.BG_DOOR_ARTIFACT,
    ]
]

objTiles: list[list[tileType]] = [
    [
        tileType.OBJ_ENEMY_START_POINT,
        tileType.OBJ_ENEMY_NORMAL,
        tileType.OBJ_ENEMY_STRONG,
        tileType.OBJ_ENEMY_ARMORED,
        tileType.OBJ_ENEMY_FLAMING,
        tileType.OBJ_ENEMY_HIDDEN,
        tileType.OBJ_ENEMY_BOSS,
        tileType.OBJ_ITEM_PICKUP_ENERGY,
        tileType.OBJ_ITEM_PICKUP_MISSILE,
        tileType.OBJ_ITEM_KEY_A,
        tileType.OBJ_ITEM_KEY_B,
        tileType.OBJ_ITEM_KEY_C,
    ],
    [
        tileType.OBJ_ITEM_BEAM,
        tileType.OBJ_ITEM_CHARGE_BEAM,
        tileType.OBJ_ITEM_MISSILE,
        tileType.OBJ_ITEM_ICE,
        tileType.OBJ_ITEM_XRAY,
        tileType.OBJ_ITEM_SUIT_WATER,
        tileType.OBJ_ITEM_SUIT_LAVA,
        tileType.OBJ_ITEM_ENERGY_TANK,
        tileType.OBJ_ITEM_ARTIFACT,
    ],
    [
        tileType.OBJ_SCENERY_TERMINAL,
        tileType.OBJ_SCENERY_PORTAL,
        tileType.OBJ_SCENERY_F1,
        tileType.OBJ_SCENERY_F2,
        tileType.OBJ_SCENERY_F3,
        tileType.OBJ_SCENERY_F4,
        tileType.OBJ_SCENERY_F5,
        tileType.OBJ_SCENERY_F6,
        tileType.OBJ_SCENERY_F7,
        tileType.DELETE
    ]
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
