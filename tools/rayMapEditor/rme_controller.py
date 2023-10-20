from rme_tiles import *
from rme_view import clickMode


class controller:

    def __init__(self):
        self.isMapLeftClicked: bool = False
        self.isPaletteClicked: bool = False

    def setModel(self, m):
        from rme_model import model
        self.m: model = m

    def clickPalette(self, x, y):
        self.isPaletteClicked = True
        type = self.m.getPaletteType(x, y)
        if type is not None:
            self.m.setSelectedTileType(type, x, y)

    def leftClickMap(self, x, y, mode: clickMode):
        type: tileType = self.m.getSelectedTileType()
        if None == type and clickMode.SET_SPAWN_TRIGGER_ZONE != mode:
            return
        self.isMapLeftClicked = True

        if clickMode.NORMAL_EDIT == mode:
            if (type.value & OBJ) == OBJ or tileType.DELETE == type:
                self.m.setMapTileObj(x, y, type)
            else:
                self.m.setMapTileBg(x, y, type)
        elif clickMode.SET_SPAWN_TRIGGER_ZONE == mode:
            self.m.addTileTriggerToScript(x, y, (tileType.DELETE == type))
        elif clickMode.SET_ENEMIES == mode:
            if ((type.value & 0xE0) == (OBJ | ENEMY)) or (tileType.DELETE == type):
                self.m.addEnemyToScript(x, y, type)

    def releaseClick(self):
        self.isMapLeftClicked = False
        self.isPaletteClicked = False

    def moveMouseMap(self, cellX, cellY, mode: clickMode):
        if self.isMapLeftClicked:
            self.leftClickMap(cellX, cellY, mode)

    def moveMousePalette(self, cellX, cellY):
        if self.isPaletteClicked:
            self.clickPalette(cellX, cellY)

    def rightClickMap(self, x, y):
        self.m.setSelectedCell(x, y)
