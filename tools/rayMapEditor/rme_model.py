from rme_tiles import *
from rme_script_editor import *


class model:

    def __init__(self, width: int, height: int):
        self.tileMap: list[list[tile]] = [
            [tile() for y in range(height)] for x in range(width)]
        self.selectedTileType: tileType = None
        self.mapWidth = width
        self.mapHeight = height
        self.scripts: list[rme_script] = []
        self.splitter: rme_scriptSplitter = rme_scriptSplitter()
        self.currentId: int = 0
        self.usedIds: list[int] = []

    def setView(self, v):
        from rme_view import view
        self.v: view = v

    def getMapWidth(self):
        return len(self.tileMap)

    def getMapHeight(self):
        return len(self.tileMap[0])

    def setMapTileBg(self, x: int, y: int, bg: tileType):
        if (0 <= x and x < len(self.tileMap)):
            if (0 <= y and y < len(self.tileMap[x])):
                if not self.tileMap[x][y].background == bg:
                    self.tileMap[x][y].setBg(bg)
                    self.v.drawMapCell(x, y)

    def setMapTileObj(self, x: int, y: int, obj: tileType):
        if (0 <= x and x < len(self.tileMap)):
            if (0 <= y and y < len(self.tileMap[x])):
                if tileType.OBJ_DELETE == obj:
                    if tileType.EMPTY != self.tileMap[x][y].object:
                        self.usedIds.remove(self.tileMap[x][y].objectId)
                        self.tileMap[x][y].setObj(tileType.EMPTY, -1)
                        self.v.drawMapCell(x, y)
                elif not self.tileMap[x][y].object == obj:
                    objId = self.getNextId()
                    if -1 != objId:
                        self.tileMap[x][y].setObj(obj, objId)
                        self.v.drawMapCell(x, y)

    def getNextId(self):
        if len(self.usedIds) == 256:
            return -1
        while True:
            if self.currentId not in self.usedIds:
                self.usedIds.append(self.currentId)
                return self.currentId
            else:
                self.currentId = (self.currentId + 1) % 256

    def getPaletteType(self, x, y):
        if 0 == x:
            # backgrounds in the first column
            if 0 <= y and y < len(bgTiles):
                return bgTiles[y]
        elif 1 == x:
            # objects i the second column
            if 0 <= y and y < len(objTiles):
                return objTiles[y]
        return None

    def setSelectedTileType(self, type):
        self.selectedTileType = type
        self.v.drawSelectedTile(self.selectedTileType)

    def getSelectedTileType(self):
        return self.selectedTileType

    def setSelectedCell(self, x, y):
        if (0 <= x and x < len(self.tileMap)):
            if (0 <= y and y < len(self.tileMap[x])):
                self.v.selectCell(x, y, self.tileMap[x][y].objectId)

    def setScripts(self, scripts: list[str]) -> None:
        self.scripts: list[rme_script] = []
        for script in scripts:
            self.scripts.append(rme_script(
                string=script, splitter=self.splitter))
