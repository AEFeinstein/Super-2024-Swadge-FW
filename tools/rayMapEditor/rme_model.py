from rme_tiles import *
from rme_script_editor import *
from rme_view import NUM_PALETTE_ROWS
from io import TextIOWrapper


class model:

    def __init__(self, width: int, height: int):
        self.tileMap: list[list[tile]] = [
            [tile() for y in range(height)] for x in range(width)]
        self.selectedTileType: tileType = None
        self.scripts: list[rme_script] = []
        self.splitter: rme_scriptSplitter = rme_scriptSplitter()
        self.currentId: int = 0
        self.usedIds: list[int] = []

    def setView(self, v):
        from rme_view import view
        self.v: view = v

    def setMapSize(self, newWidth: int, newHeight: int):
        print(str(newWidth) + ' x ' + str(newHeight))

        # Make a new map
        newTileMap: list[list[tile]] = [
            [tile() for y in range(newHeight)] for x in range(newWidth)]

        # Copy as much as one can from old to new
        minWidth: int = min(newWidth, self.getMapWidth())
        minHeight: int = min(newHeight, self.getMapHeight())
        for y in range(minHeight):
            for x in range(minWidth):
                newTileMap[x][y] = self.tileMap[x][y]

        # Set the new map
        self.tileMap = newTileMap

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
                if tileType.DELETE == obj:
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
        else:
            # objects i the other columns
            y = y + NUM_PALETTE_ROWS * (x - 1)
            if 0 <= y and y < len(objTiles):
                return objTiles[y]
        return None

    def setSelectedTileType(self, type, x, y):
        self.selectedTileType = type
        self.v.drawSelectedTile(self.selectedTileType, x, y)

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

    def save(self, outFile: TextIOWrapper) -> bool:
        # Construct file bytes
        fileBytes: bytearray = bytearray()

        # Write map dimensions
        if self.getMapHeight() > 255 or self.getMapWidth() > 255:
            # TODO display error
            print("MAP TOO BIG!!")
            return False
        fileBytes.append(self.getMapWidth())
        fileBytes.append(self.getMapHeight())

        # Write map tiles
        for y in range(self.getMapHeight()):
            for x in range(self.getMapWidth()):
                fileBytes.append(self.tileMap[x][y].background.value)
                fileBytes.append(self.tileMap[x][y].object.value)
                # Only append IDs if there is an object
                if tileType.EMPTY is not self.tileMap[x][y].object:
                    fileBytes.append(self.tileMap[x][y].objectId)

        # Write number of scripts
        numScripts: int = sum(x is not None and x.isValid()
                              for x in self.scripts)
        if numScripts > 255:
            # TODO display error
            print("TOO MANY SCRIPTS!!")
            return False
        fileBytes.append(numScripts)

        # Write script data
        for script in self.scripts:
            if script is not None and script.isValid():
                sb = script.toBytes()
                if (len(sb) > 255):
                    # TODO display error
                    print("SCRIPT TOO BIG!!")
                    return False
                fileBytes.append(len(sb))
                fileBytes.extend(sb)
            else:
                # TODO display warning
                print("INVALID SCRIPT!!")

        # Write bytes to file
        return len(fileBytes) == outFile.write(fileBytes)

    def load(self, inFile: TextIOWrapper) -> bool:
        data: bytes = inFile.read()
        idx: int = 0

        # Read width and height
        mapWidth: int = data[idx]
        idx = idx + 1
        mapHeight: int = data[idx]
        idx = idx + 1

        # Make empty tiles and used IDs
        self.tileMap = [
            [tile() for y in range(mapHeight)] for x in range(mapWidth)]
        self.usedIds = []
        self.currentId = 0

        # Nothing selected yet
        self.selectedTileType = None

        # Read map tiles
        for y in range(self.getMapHeight()):
            for x in range(self.getMapWidth()):
                # Read background
                self.tileMap[x][y].background = tileType._value2member_map_[
                    data[idx]]
                idx = idx + 1
                # Read Object
                self.tileMap[x][y].object = tileType._value2member_map_[
                    data[idx]]
                idx = idx + 1
                # Read optional object ID
                if tileType.EMPTY is not self.tileMap[x][y].object:
                    self.tileMap[x][y].objectId = data[idx]
                    idx = idx + 1
                    # Note this ID is used
                    self.usedIds.append(self.tileMap[x][y].objectId)

        # Read number of scripts
        numScripts: int = data[idx]
        idx = idx + 1

        # Empty the list of scripts
        self.scripts = []

        # Read each script
        for si in range(numScripts):
            # Read script length
            sLen: int = data[idx]
            idx = idx + 1
            # Read script
            self.scripts.append(rme_script(bytes=data[idx: idx + sLen]))
            idx = idx + sLen

        # Everything loaded
        return True
