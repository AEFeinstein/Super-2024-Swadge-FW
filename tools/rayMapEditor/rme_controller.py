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
            self.m.setSelectedTileType(type)

    def leftClickMap(self, x, y):
        self.isMapLeftClicked = True
        if self.m.getSelectedTileType() in bgTiles:
            self.m.setMapTileBg(x, y, self.m.selectedTileType)
        elif self.m.getSelectedTileType() in objTiles:
            self.m.setMapTileObj(x, y, self.m.selectedTileType)

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
