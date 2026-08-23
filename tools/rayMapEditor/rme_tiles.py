
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
    EMPTY  = (BG | META | 0)
    # Special delete tile
    DELETE = (BG | META | 1)
    # Floor tiles
    BG_FLOOR_HOLE = (BG | FLOOR | 0)
    BG_FLOOR_1 = (BG | FLOOR | 1)
    BG_FLOOR_2 = (BG | FLOOR | 2)
    BG_FLOOR_3 = (BG | FLOOR | 3)
    BG_FLOOR_4 = (BG | FLOOR | 4)
    BG_FLOOR_5 = (BG | FLOOR | 5)
    BG_FLOOR_6 = (BG | FLOOR | 6)
    BG_FLOOR_7 = (BG | FLOOR | 7)
    BG_FLOOR_8 = (BG | FLOOR | 8)
    BG_FLOOR_9 = (BG | FLOOR | 9)
    BG_FLOOR_10 = (BG | FLOOR | 10)
    BG_FLOOR_11 = (BG | FLOOR | 11)
    BG_FLOOR_12 = (BG | FLOOR | 12)
    BG_FLOOR_13 = (BG | FLOOR | 13)
    BG_FLOOR_14 = (BG | FLOOR | 14)
    BG_FLOOR_15 = (BG | FLOOR | 15)
    BG_FLOOR_16 = (BG | FLOOR | 16)
    BG_FLOOR_17 = (BG | FLOOR | 17)
    BG_FLOOR_18 = (BG | FLOOR | 18)
    BG_FLOOR_19 = (BG | FLOOR | 19)
    BG_FLOOR_20 = (BG | FLOOR | 20)
    BG_FLOOR_21 = (BG | FLOOR | 21)
    BG_FLOOR_22 = (BG | FLOOR | 22)
    BG_FLOOR_23 = (BG | FLOOR | 23)
    BG_FLOOR_24 = (BG | FLOOR | 24)
    BG_FLOOR_25 = (BG | FLOOR | 25)
    BG_FLOOR_26 = (BG | FLOOR | 26)
    BG_FLOOR_27 = (BG | FLOOR | 27)
    BG_FLOOR_28 = (BG | FLOOR | 28)
    BG_FLOOR_29 = (BG | FLOOR | 29)
    BG_FLOOR_30 = (BG | FLOOR | 30)
    BG_FLOOR_31 = (BG | FLOOR | 31)
    # Wall tiles
    BG_WALL_0 = (BG | WALL | 0)
    BG_WALL_1 = (BG | WALL | 1)
    BG_WALL_2 = (BG | WALL | 2)
    BG_WALL_3 = (BG | WALL | 3)
    BG_WALL_4 = (BG | WALL | 4)
    BG_WALL_5 = (BG | WALL | 5)
    BG_WALL_6 = (BG | WALL | 6)
    BG_WALL_7 = (BG | WALL | 7)
    BG_WALL_8 = (BG | WALL | 8)
    BG_WALL_9 = (BG | WALL | 9)
    BG_WALL_10 = (BG | WALL | 10)
    BG_WALL_11 = (BG | WALL | 11)
    BG_WALL_12 = (BG | WALL | 12)
    BG_WALL_13 = (BG | WALL | 13)
    BG_WALL_14 = (BG | WALL | 14)
    BG_WALL_15 = (BG | WALL | 15)
    BG_WALL_16 = (BG | WALL | 16)
    BG_WALL_17 = (BG | WALL | 17)
    BG_WALL_18 = (BG | WALL | 18)
    BG_WALL_19 = (BG | WALL | 19)
    BG_WALL_20 = (BG | WALL | 20)
    BG_WALL_21 = (BG | WALL | 21)
    BG_WALL_22 = (BG | WALL | 22)
    BG_WALL_23 = (BG | WALL | 23)
    BG_WALL_24 = (BG | WALL | 24)
    BG_WALL_25 = (BG | WALL | 25)
    BG_WALL_26 = (BG | WALL | 26)
    BG_WALL_27 = (BG | WALL | 27)
    BG_WALL_28 = (BG | WALL | 28)
    BG_WALL_29 = (BG | WALL | 29)
    BG_WALL_30 = (BG | WALL | 30)
    BG_WALL_31 = (BG | WALL | 31)
    # Door tiles
    BG_DOOR_BUSH    = (BG | DOOR | 0)
    BG_DOOR_CRACK_H = (BG | DOOR | 1)
    BG_DOOR_CRACK_V = (BG | DOOR | 2)
    BG_DOOR_3 = (BG | DOOR | 3)
    BG_DOOR_4 = (BG | DOOR | 4)
    BG_DOOR_5 = (BG | DOOR | 5)
    BG_DOOR_6 = (BG | DOOR | 6)
    BG_DOOR_7 = (BG | DOOR | 7)
    BG_DOOR_8 = (BG | DOOR | 8)
    BG_DOOR_9 = (BG | DOOR | 9)
    BG_DOOR_10 = (BG | DOOR | 10)
    BG_DOOR_11 = (BG | DOOR | 11)
    BG_DOOR_12 = (BG | DOOR | 12)
    BG_DOOR_13 = (BG | DOOR | 13)
    BG_DOOR_14 = (BG | DOOR | 14)
    BG_DOOR_15 = (BG | DOOR | 15)
    BG_DOOR_16 = (BG | DOOR | 16)
    BG_DOOR_17 = (BG | DOOR | 17)
    BG_DOOR_18 = (BG | DOOR | 18)
    BG_DOOR_19 = (BG | DOOR | 19)
    BG_DOOR_20 = (BG | DOOR | 20)
    BG_DOOR_21 = (BG | DOOR | 21)
    BG_DOOR_22 = (BG | DOOR | 22)
    BG_DOOR_23 = (BG | DOOR | 23)
    BG_DOOR_24 = (BG | DOOR | 24)
    BG_DOOR_25 = (BG | DOOR | 25)
    BG_DOOR_26 = (BG | DOOR | 26)
    BG_DOOR_27 = (BG | DOOR | 27)
    BG_DOOR_28 = (BG | DOOR | 28)
    BG_DOOR_29 = (BG | DOOR | 29)
    BG_DOOR_30 = (BG | DOOR | 30)
    BG_DOOR_31 = (BG | DOOR | 31)
    # Self and Enemies
    OBJ_ENEMY_START_POINT = (OBJ | ENEMY | 0)
    OBJ_ENEMY_BOX  = (OBJ | ENEMY | 1)
    OBJ_ENEMY_2 = (OBJ | ENEMY | 2)
    OBJ_ENEMY_3 = (OBJ | ENEMY | 3)
    OBJ_ENEMY_4 = (OBJ | ENEMY | 4)
    OBJ_ENEMY_5 = (OBJ | ENEMY | 5)
    OBJ_ENEMY_6 = (OBJ | ENEMY | 6)
    OBJ_ENEMY_7 = (OBJ | ENEMY | 7)
    OBJ_ENEMY_8 = (OBJ | ENEMY | 8)
    OBJ_ENEMY_9 = (OBJ | ENEMY | 9)
    OBJ_ENEMY_10 = (OBJ | ENEMY | 10)
    OBJ_ENEMY_11 = (OBJ | ENEMY | 11)
    OBJ_ENEMY_12 = (OBJ | ENEMY | 12)
    OBJ_ENEMY_13 = (OBJ | ENEMY | 13)
    OBJ_ENEMY_14 = (OBJ | ENEMY | 14)
    OBJ_ENEMY_15 = (OBJ | ENEMY | 15)
    OBJ_ENEMY_16 = (OBJ | ENEMY | 16)
    OBJ_ENEMY_17 = (OBJ | ENEMY | 17)
    OBJ_ENEMY_18 = (OBJ | ENEMY | 18)
    OBJ_ENEMY_19 = (OBJ | ENEMY | 19)
    OBJ_ENEMY_20 = (OBJ | ENEMY | 20)
    OBJ_ENEMY_21 = (OBJ | ENEMY | 21)
    OBJ_ENEMY_22 = (OBJ | ENEMY | 22)
    OBJ_ENEMY_23 = (OBJ | ENEMY | 23)
    OBJ_ENEMY_24 = (OBJ | ENEMY | 24)
    OBJ_ENEMY_25 = (OBJ | ENEMY | 25)
    OBJ_ENEMY_26 = (OBJ | ENEMY | 26)
    OBJ_ENEMY_27 = (OBJ | ENEMY | 27)
    OBJ_ENEMY_28 = (OBJ | ENEMY | 28)
    OBJ_ENEMY_29 = (OBJ | ENEMY | 29)
    OBJ_ENEMY_30 = (OBJ | ENEMY | 30)
    OBJ_ENEMY_31 = (OBJ | ENEMY | 31)
    # Item pickups
    OBJ_ITEM_EWI        = (OBJ | ITEM | 0)
    OBJ_ITEM_BOMB       = (OBJ | ITEM | 1)
    OBJ_ITEM_BOOTS      = (OBJ | ITEM | 2)
    OBJ_ITEM_SHIELD     = (OBJ | ITEM | 3)
    OBJ_ITEM_BOW        = (OBJ | ITEM | 4)
    OBJ_ITEM_BOOMERANG  = (OBJ | ITEM | 5)
    OBJ_ITEM_TURNTABLES = (OBJ | ITEM | 6)
    OBJ_ITEM_LULLABY    = (OBJ | ITEM | 7)
    OBJ_ITEM_HEART      = (OBJ | ITEM | 8)
    OBJ_ITEM_MPOINT_1   = (OBJ | ITEM | 9)
    OBJ_ITEM_MPOINT_5   = (OBJ | ITEM | 10)
    OBJ_ITEM_MPOINT_10  = (OBJ | ITEM | 11)
    OBJ_ITEM_MPOINT_20  = (OBJ | ITEM | 12)
    OBJ_ITEM_13 = (OBJ | ITEM | 13)
    OBJ_ITEM_14 = (OBJ | ITEM | 14)
    OBJ_ITEM_15 = (OBJ | ITEM | 15)
    OBJ_ITEM_16 = (OBJ | ITEM | 16)
    OBJ_ITEM_17 = (OBJ | ITEM | 17)
    OBJ_ITEM_18 = (OBJ | ITEM | 18)
    OBJ_ITEM_19 = (OBJ | ITEM | 19)
    OBJ_ITEM_20 = (OBJ | ITEM | 20)
    OBJ_ITEM_21 = (OBJ | ITEM | 21)
    OBJ_ITEM_22 = (OBJ | ITEM | 22)
    OBJ_ITEM_23 = (OBJ | ITEM | 23)
    OBJ_ITEM_24 = (OBJ | ITEM | 24)
    OBJ_ITEM_25 = (OBJ | ITEM | 25)
    OBJ_ITEM_26 = (OBJ | ITEM | 26)
    OBJ_ITEM_27 = (OBJ | ITEM | 27)
    OBJ_ITEM_28 = (OBJ | ITEM | 28)
    OBJ_ITEM_29 = (OBJ | ITEM | 29)
    OBJ_ITEM_30 = (OBJ | ITEM | 30)
    OBJ_ITEM_31 = (OBJ | ITEM | 31)
    # Bullets
    OBJ_BULLET_0 = (OBJ | BULLET | 0)
    OBJ_BULLET_1 = (OBJ | BULLET | 1)
    OBJ_BULLET_2 = (OBJ | BULLET | 2)
    OBJ_BULLET_3 = (OBJ | BULLET | 3)
    OBJ_BULLET_4 = (OBJ | BULLET | 4)
    OBJ_BULLET_5 = (OBJ | BULLET | 5)
    OBJ_BULLET_6 = (OBJ | BULLET | 6)
    OBJ_BULLET_7 = (OBJ | BULLET | 7)
    OBJ_BULLET_8 = (OBJ | BULLET | 8)
    OBJ_BULLET_9 = (OBJ | BULLET | 9)
    OBJ_BULLET_10 = (OBJ | BULLET | 10)
    OBJ_BULLET_11 = (OBJ | BULLET | 11)
    OBJ_BULLET_12 = (OBJ | BULLET | 12)
    OBJ_BULLET_13 = (OBJ | BULLET | 13)
    OBJ_BULLET_14 = (OBJ | BULLET | 14)
    OBJ_BULLET_15 = (OBJ | BULLET | 15)
    OBJ_BULLET_16 = (OBJ | BULLET | 16)
    OBJ_BULLET_17 = (OBJ | BULLET | 17)
    OBJ_BULLET_18 = (OBJ | BULLET | 18)
    OBJ_BULLET_19 = (OBJ | BULLET | 19)
    OBJ_BULLET_20 = (OBJ | BULLET | 20)
    OBJ_BULLET_21 = (OBJ | BULLET | 21)
    OBJ_BULLET_22 = (OBJ | BULLET | 22)
    OBJ_BULLET_23 = (OBJ | BULLET | 23)
    OBJ_BULLET_24 = (OBJ | BULLET | 24)
    OBJ_BULLET_25 = (OBJ | BULLET | 25)
    OBJ_BULLET_26 = (OBJ | BULLET | 26)
    OBJ_BULLET_27 = (OBJ | BULLET | 27)
    OBJ_BULLET_28 = (OBJ | BULLET | 28)
    OBJ_BULLET_29 = (OBJ | BULLET | 29)
    OBJ_BULLET_30 = (OBJ | BULLET | 30)
    OBJ_BULLET_31 = (OBJ | BULLET | 31)
    # Scenery
    OBJ_SCENERY_SHOP_BOMB = (OBJ | SCENERY | 0)
    OBJ_SCENERY_1 = (OBJ | SCENERY | 1)
    OBJ_SCENERY_2 = (OBJ | SCENERY | 2)
    OBJ_SCENERY_3 = (OBJ | SCENERY | 3)
    OBJ_SCENERY_4 = (OBJ | SCENERY | 4)
    OBJ_SCENERY_5 = (OBJ | SCENERY | 5)
    OBJ_SCENERY_6 = (OBJ | SCENERY | 6)
    OBJ_SCENERY_7 = (OBJ | SCENERY | 7)
    OBJ_SCENERY_8 = (OBJ | SCENERY | 8)
    OBJ_SCENERY_9 = (OBJ | SCENERY | 9)
    OBJ_SCENERY_10 = (OBJ | SCENERY | 10)
    OBJ_SCENERY_11 = (OBJ | SCENERY | 11)
    OBJ_SCENERY_12 = (OBJ | SCENERY | 12)
    OBJ_SCENERY_13 = (OBJ | SCENERY | 13)
    OBJ_SCENERY_14 = (OBJ | SCENERY | 14)
    OBJ_SCENERY_15 = (OBJ | SCENERY | 15)
    OBJ_SCENERY_16 = (OBJ | SCENERY | 16)
    OBJ_SCENERY_17 = (OBJ | SCENERY | 17)
    OBJ_SCENERY_18 = (OBJ | SCENERY | 18)
    OBJ_SCENERY_19 = (OBJ | SCENERY | 19)
    OBJ_SCENERY_20 = (OBJ | SCENERY | 20)
    OBJ_SCENERY_21 = (OBJ | SCENERY | 21)
    OBJ_SCENERY_22 = (OBJ | SCENERY | 22)
    OBJ_SCENERY_23 = (OBJ | SCENERY | 23)
    OBJ_SCENERY_24 = (OBJ | SCENERY | 24)
    OBJ_SCENERY_25 = (OBJ | SCENERY | 25)
    OBJ_SCENERY_26 = (OBJ | SCENERY | 26)
    OBJ_SCENERY_27 = (OBJ | SCENERY | 27)
    OBJ_SCENERY_28 = (OBJ | SCENERY | 28)
    OBJ_SCENERY_29 = (OBJ | SCENERY | 29)
    OBJ_SCENERY_30 = (OBJ | SCENERY | 30)
    OBJ_SCENERY_31 = (OBJ | SCENERY | 31)


bgTiles: list[list[tileType]] = [
    [
        tileType.BG_FLOOR_HOLE,
        tileType.BG_FLOOR_1,
        tileType.BG_FLOOR_2,
        tileType.BG_FLOOR_3,
        tileType.BG_FLOOR_4,
        tileType.BG_FLOOR_5,
        tileType.BG_FLOOR_6,
        tileType.BG_FLOOR_7,
        tileType.BG_FLOOR_8,
        tileType.BG_FLOOR_9,
        tileType.BG_FLOOR_10,
        tileType.BG_FLOOR_11,
        tileType.BG_FLOOR_12,
        tileType.BG_FLOOR_13,
        tileType.BG_FLOOR_14,
        tileType.BG_FLOOR_15,
        tileType.BG_FLOOR_16,
        tileType.BG_FLOOR_17,
        tileType.BG_FLOOR_18,
        tileType.BG_FLOOR_19,
        tileType.BG_FLOOR_20,
        tileType.BG_FLOOR_21,
        tileType.BG_FLOOR_22,
        tileType.BG_FLOOR_23,
        tileType.BG_FLOOR_24,
        tileType.BG_FLOOR_25,
        tileType.BG_FLOOR_26,
        tileType.BG_FLOOR_27,
        tileType.BG_FLOOR_28,
        tileType.BG_FLOOR_29,
        tileType.BG_FLOOR_30,
        tileType.BG_FLOOR_31,
    ],
    [
        tileType.BG_WALL_0,
        tileType.BG_WALL_1,
        tileType.BG_WALL_2,
        tileType.BG_WALL_3,
        tileType.BG_WALL_4,
        tileType.BG_WALL_5,
        tileType.BG_WALL_6,
        tileType.BG_WALL_7,
        tileType.BG_WALL_8,
        tileType.BG_WALL_9,
        tileType.BG_WALL_10,
        tileType.BG_WALL_11,
        tileType.BG_WALL_12,
        tileType.BG_WALL_13,
        tileType.BG_WALL_14,
        tileType.BG_WALL_15,
        tileType.BG_WALL_16,
        tileType.BG_WALL_17,
        tileType.BG_WALL_18,
        tileType.BG_WALL_19,
        tileType.BG_WALL_20,
        tileType.BG_WALL_21,
        tileType.BG_WALL_22,
        tileType.BG_WALL_23,
        tileType.BG_WALL_24,
        tileType.BG_WALL_25,
        tileType.BG_WALL_26,
        tileType.BG_WALL_27,
        tileType.BG_WALL_28,
        tileType.BG_WALL_29,
        tileType.BG_WALL_30,
        tileType.BG_WALL_31,
    ],
    [
        tileType.BG_DOOR_BUSH,
        tileType.BG_DOOR_CRACK_H,
        tileType.BG_DOOR_CRACK_V,
        tileType.BG_DOOR_3,
        tileType.BG_DOOR_4,
        tileType.BG_DOOR_5,
        tileType.BG_DOOR_6,
        tileType.BG_DOOR_7,
        tileType.BG_DOOR_8,
        tileType.BG_DOOR_9,
        tileType.BG_DOOR_10,
        tileType.BG_DOOR_11,
        tileType.BG_DOOR_12,
        tileType.BG_DOOR_13,
        tileType.BG_DOOR_14,
        tileType.BG_DOOR_15,
        tileType.BG_DOOR_16,
        tileType.BG_DOOR_17,
        tileType.BG_DOOR_18,
        tileType.BG_DOOR_19,
        tileType.BG_DOOR_20,
        tileType.BG_DOOR_21,
        tileType.BG_DOOR_22,
        tileType.BG_DOOR_23,
        tileType.BG_DOOR_24,
        tileType.BG_DOOR_25,
        tileType.BG_DOOR_26,
        tileType.BG_DOOR_27,
        tileType.BG_DOOR_28,
        tileType.BG_DOOR_29,
        tileType.BG_DOOR_30,
        tileType.BG_DOOR_31,
        tileType.DELETE,
        ]
]

objTiles: list[list[tileType]] = [
    [
        tileType.OBJ_ITEM_EWI,
        tileType.OBJ_ITEM_BOMB,
        tileType.OBJ_ITEM_BOOTS,
        tileType.OBJ_ITEM_SHIELD,
        tileType.OBJ_ITEM_BOW,
        tileType.OBJ_ITEM_BOOMERANG,
        tileType.OBJ_ITEM_TURNTABLES,
        tileType.OBJ_ITEM_LULLABY,
        tileType.OBJ_ITEM_HEART,
        tileType.OBJ_ITEM_MPOINT_1,
        tileType.OBJ_ITEM_MPOINT_5,
        tileType.OBJ_ITEM_MPOINT_10,
        tileType.OBJ_ITEM_MPOINT_20,
        tileType.OBJ_ITEM_13,
        tileType.OBJ_ITEM_14,
        tileType.OBJ_ITEM_15,
        tileType.OBJ_ITEM_16,
        tileType.OBJ_ITEM_17,
        tileType.OBJ_ITEM_18,
        tileType.OBJ_ITEM_19,
        tileType.OBJ_ITEM_20,
        tileType.OBJ_ITEM_21,
        tileType.OBJ_ITEM_22,
        tileType.OBJ_ITEM_23,
        tileType.OBJ_ITEM_24,
        tileType.OBJ_ITEM_25,
        tileType.OBJ_ITEM_26,
        tileType.OBJ_ITEM_27,
        tileType.OBJ_ITEM_28,
        tileType.OBJ_ITEM_29,
        tileType.OBJ_ITEM_30,
        tileType.OBJ_ITEM_31,
    ],
    [
        tileType.OBJ_ENEMY_START_POINT,
        tileType.OBJ_ENEMY_BOX,
        tileType.OBJ_ENEMY_2,
        tileType.OBJ_ENEMY_3,
        tileType.OBJ_ENEMY_4,
        tileType.OBJ_ENEMY_5,
        tileType.OBJ_ENEMY_6,
        tileType.OBJ_ENEMY_7,
        tileType.OBJ_ENEMY_8,
        tileType.OBJ_ENEMY_9,
        tileType.OBJ_ENEMY_10,
        tileType.OBJ_ENEMY_11,
        tileType.OBJ_ENEMY_12,
        tileType.OBJ_ENEMY_13,
        tileType.OBJ_ENEMY_14,
        tileType.OBJ_ENEMY_15,
        tileType.OBJ_ENEMY_16,
        tileType.OBJ_ENEMY_17,
        tileType.OBJ_ENEMY_18,
        tileType.OBJ_ENEMY_19,
        tileType.OBJ_ENEMY_20,
        tileType.OBJ_ENEMY_21,
        tileType.OBJ_ENEMY_22,
        tileType.OBJ_ENEMY_23,
        tileType.OBJ_ENEMY_24,
        tileType.OBJ_ENEMY_25,
        tileType.OBJ_ENEMY_26,
        tileType.OBJ_ENEMY_27,
        tileType.OBJ_ENEMY_28,
        tileType.OBJ_ENEMY_29,
        tileType.OBJ_ENEMY_30,
        tileType.OBJ_ENEMY_31,
    ],
    [
        tileType.OBJ_SCENERY_SHOP_BOMB,
        tileType.OBJ_SCENERY_1,
        tileType.OBJ_SCENERY_2,
        tileType.OBJ_SCENERY_3,
        tileType.OBJ_SCENERY_4,
        tileType.OBJ_SCENERY_5,
        tileType.OBJ_SCENERY_6,
        tileType.OBJ_SCENERY_7,
        tileType.OBJ_SCENERY_8,
        tileType.OBJ_SCENERY_9,
        tileType.OBJ_SCENERY_10,
        tileType.OBJ_SCENERY_11,
        tileType.OBJ_SCENERY_12,
        tileType.OBJ_SCENERY_13,
        tileType.OBJ_SCENERY_14,
        tileType.OBJ_SCENERY_15,
        tileType.OBJ_SCENERY_16,
        tileType.OBJ_SCENERY_17,
        tileType.OBJ_SCENERY_18,
        tileType.OBJ_SCENERY_19,
        tileType.OBJ_SCENERY_20,
        tileType.OBJ_SCENERY_21,
        tileType.OBJ_SCENERY_22,
        tileType.OBJ_SCENERY_23,
        tileType.OBJ_SCENERY_24,
        tileType.OBJ_SCENERY_25,
        tileType.OBJ_SCENERY_26,
        tileType.OBJ_SCENERY_27,
        tileType.OBJ_SCENERY_28,
        tileType.OBJ_SCENERY_29,
        tileType.OBJ_SCENERY_30,
        tileType.OBJ_SCENERY_31,
    ]
]


class tile:
    def __init__(self):
        self.background: tileType = tileType.BG_FLOOR_31
        self.object: tileType = tileType.EMPTY
        self.objectId: int = -1

    def setBg(self, bg: tileType):
        self.background = bg

    def setObj(self, obj: tileType, id: int):
        self.object = obj
        self.objectId = id
