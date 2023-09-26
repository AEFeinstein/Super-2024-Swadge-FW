from rme_tiles import *


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

    def leftClickMap(self, x, y):
        self.isMapLeftClicked = True
        type = self.m.getSelectedTileType()
        if (type.value & OBJ) == OBJ or tileType.DELETE == type:
            self.m.setMapTileObj(x, y, type)
        else:
            self.m.setMapTileBg(x, y, type)

    def releaseClick(self):
        self.isMapLeftClicked = False
        self.isPaletteClicked = False

    def moveMouseMap(self, cellX, cellY):
        if self.isMapLeftClicked:
            self.leftClickMap(cellX, cellY)

    def moveMousePalette(self, cellX, cellY):
        if self.isPaletteClicked:
            self.clickPalette(cellX, cellY)

    def rightClickMap(self, x, y):
        self.m.setSelectedCell(x, y)
