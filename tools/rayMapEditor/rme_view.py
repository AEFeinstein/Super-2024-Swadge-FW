import tkinter as tk
from PIL import Image, ImageTk

from collections.abc import Mapping

from rme_tiles import *


class view:

    def __init__(self):
        self.paletteCellSize: int = 64
        self.mapCellSize: int = 32

        self.isMapMiddleClicked: bool = False
        self.lastScrollX: int = -1
        self.lastScrollY: int = -1
        self.mapOffsetX: int = 0
        self.mapOffsetY: int = 0

        self.selRectX: int = 0
        self.selRectY: int = 0

        self.highlightRect: int = -1

        frameBgColor: str = '#181818'
        elemBgColor: str = '#1F1F1F'
        borderColor: str = '#2A2A2A'
        borderHighlightColor: str = '#2B79D7'
        fontColor: str = '#CCCCCC'
        fontStyle = ('Courier New', 14)
        borderThickness: int = 2
        padding: int = 4

        buttonColor: str = '#2B79D7'
        buttonPressedColor: str = '#5191DE'
        buttonFontColor: str = '#FFFFFF'

        # Setup the root and main frames
        self.root: tk.Tk = tk.Tk()
        self.root.title("Ray Map Editor")
        content = tk.Frame(self.root, background=frameBgColor)
        frame = tk.Frame(content, background=frameBgColor)

        # Setup the button frame and buttons
        self.buttonFrame: tk.Frame = tk.Frame(content, height=0, background=elemBgColor,
                                              highlightthickness=borderThickness, highlightbackground=borderColor, padx=padding, pady=padding)
        self.loadButton: tk.Button = tk.Button(self.buttonFrame, height=1, width=6, text="Load", font=fontStyle, background=buttonColor,
                                               foreground=buttonFontColor, activebackground=buttonPressedColor, activeforeground=buttonFontColor, bd=0)
        self.saveButton: tk.Button = tk.Button(self.buttonFrame, height=1, width=6, text="Save", font=fontStyle, background=buttonColor,
                                               foreground=buttonFontColor, activebackground=buttonPressedColor, activeforeground=buttonFontColor, bd=0)
        self.exitButton: tk.Button = tk.Button(self.buttonFrame, height=1, width=6, text="Exit", font=fontStyle, background=buttonColor,
                                               foreground=buttonFontColor, activebackground=buttonPressedColor, activeforeground=buttonFontColor, bd=0)

        # Set up the canvasses
        self.paletteCanvas: tk.Canvas = tk.Canvas(
            content, background=elemBgColor, width=self.paletteCellSize * 2, height=self.paletteCellSize * 8,
            highlightthickness=borderThickness, highlightbackground=borderColor)
        self.paletteSelected: tk.Canvas = tk.Canvas(
            content, background=elemBgColor, width=self.paletteCellSize * 2, height=self.paletteCellSize * 2,
            highlightthickness=borderThickness, highlightbackground=borderColor)
        self.mapCanvas: tk.Canvas = tk.Canvas(
            content, background=elemBgColor, highlightthickness=borderThickness, highlightbackground=borderColor)

        # Set up the text
        self.cellMetaData: tk.Text = tk.Text(content, width=40,
                                             undo=True, autoseparators=True, maxundo=-1,
                                             background=elemBgColor, foreground=fontColor, insertbackground=fontColor, font=fontStyle,
                                             highlightthickness=borderThickness, highlightbackground=borderColor,
                                             highlightcolor=borderHighlightColor, borderwidth=0, bd=0)
        self.scriptTextEntry: tk.Text = tk.Text(content, height=10,
                                                undo=True, autoseparators=True, maxundo=-1,
                                                background=elemBgColor, foreground=fontColor, insertbackground=fontColor, font=fontStyle,
                                                highlightthickness=borderThickness, highlightbackground=borderColor,
                                                highlightcolor=borderHighlightColor, borderwidth=0, bd=0)

        # Configure the main frame
        content.grid(column=0, row=0, sticky=(tk.NSEW))
        frame.grid(column=0, row=0, columnspan=3, rowspan=4, sticky=(tk.NSEW))

        # Place the button bar
        self.buttonFrame.grid(column=0, row=0, columnspan=3,
                              rowspan=1, sticky=tk.NSEW, padx=padding, pady=padding)

        # Place the buttons in the button bar
        self.loadButton.grid(column=0, row=0, sticky=tk.NSEW,
                             padx=padding, pady=padding)
        self.saveButton.grid(column=1, row=0, sticky=tk.NSEW,
                             padx=padding, pady=padding)
        self.exitButton.grid(column=2, row=0, sticky=tk.NSEW,
                             padx=padding, pady=padding)

        # Place the palette and bind events
        self.paletteCanvas.grid(column=0, row=1, sticky=(
            tk.NSEW), padx=padding, pady=padding)
        self.paletteCanvas.bind("<Button-1>", self.paletteLeftClick)
        self.paletteCanvas.bind('<ButtonRelease-1>', self.clickRelease)
        self.paletteCanvas.bind('<Motion>', self.paletteMouseMotion)

        # Place the palette selection
        self.paletteSelected.grid(column=0, row=2, sticky=(
            tk.NSEW), padx=padding, pady=padding)

        # Place the map and bind events
        self.mapCanvas.grid(column=1, row=1, rowspan=2, sticky=(
            tk.NSEW), padx=padding, pady=padding)
        self.mapCanvas.bind("<Button-1>", self.mapLeftClick)
        self.mapCanvas.bind("<Button-2>", self.mapMiddleClick)
        self.mapCanvas.bind("<Button-3>", self.mapRightClick)
        self.mapCanvas.bind('<ButtonRelease-1>', self.clickRelease)
        self.mapCanvas.bind('<ButtonRelease-2>', self.clickRelease)
        self.mapCanvas.bind('<ButtonRelease-3>', self.clickRelease)
        self.mapCanvas.bind('<Motion>', self.mapMouseMotion)

        # Place the cell metadata text window
        self.cellMetaData.grid(column=2, row=1, rowspan=2, sticky=(
            tk.NSEW), padx=padding, pady=padding)

        # Place the script editor text window
        self.scriptTextEntry.grid(column=0, row=3, columnspan=3, sticky=(
            tk.NSEW), padx=padding, pady=padding)
        self.scriptTextEntry.bind("<KeyRelease>", self.scriptTextChanged)

        # Set root weights so the UI scales
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)

        # Set row and column weights so the UI scales
        content.columnconfigure(0, weight=0)
        content.columnconfigure(1, weight=1)
        content.columnconfigure(2, weight=0)
        content.rowconfigure(0, weight=0)
        content.rowconfigure(1, weight=1)
        content.rowconfigure(2, weight=0)
        content.rowconfigure(3, weight=0)

        self.texMapPalette: Mapping[tileType, ImageTk.PhotoImage] = {}
        self.texMapMap: Mapping[tileType, ImageTk.PhotoImage] = {}

        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_FLOOR, 'imgs/floor.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_WALL, 'imgs/wall.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_CEILING, 'imgs/ceiling.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_DOOR, 'imgs/door.png')

        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ENEMY, 'imgs/enemy.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_OBELISK, 'imgs/obelisk.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_START_POINT, 'imgs/start.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_GUN, 'imgs/item.png')

        # Start maximized
        self.root.state('zoomed')

    def loadTexture(self, pMap, mMap, key, texFile):
        img = Image.open(texFile)
        pResize = img.resize(
            size=[self.paletteCellSize, self.paletteCellSize], resample=Image.LANCZOS)
        pMap[key] = ImageTk.PhotoImage(pResize)
        mResize = img.resize(
            size=[self.mapCellSize, self.mapCellSize], resample=Image.LANCZOS)
        mMap[key] = ImageTk.PhotoImage(mResize)

    def setController(self, c):
        from rme_controller import controller
        self.c: controller = c

    def setModel(self, m):
        from rme_model import model
        self.m: model = m

    def mainloop(self):
        self.root.mainloop()

    def paletteLeftClick(self, event: tk.Event):
        self.c.clickPalette(int(event.x / self.paletteCellSize),
                            int(event.y / self.paletteCellSize))

    def mapLeftClick(self, event: tk.Event):
        self.c.leftClickMap(int((event.x - self.mapOffsetX) / self.mapCellSize),
                            int((event.y - self.mapOffsetY) / self.mapCellSize))

    def mapRightClick(self, event: tk.Event):
        self.c.rightClickMap(int((event.x - self.mapOffsetX) / self.mapCellSize),
                             int((event.y - self.mapOffsetY) / self.mapCellSize))

    def mapMiddleClick(self, event: tk.Event):
        self.isMapMiddleClicked = True
        self.lastScrollX = event.x
        self.lastScrollY = event.y

    def mapMouseMotion(self, event: tk.Event):
        if self.isMapMiddleClicked:
            self.mapOffsetX = self.mapOffsetX - (self.lastScrollX - event.x)
            self.mapOffsetY = self.mapOffsetY - (self.lastScrollY - event.y)
            self.lastScrollX = event.x
            self.lastScrollY = event.y
            self.redraw()
        else:
            self.c.moveMouseMap(int((event.x - self.mapOffsetX) / self.mapCellSize),
                                int((event.y - self.mapOffsetY) / self.mapCellSize))

    def paletteMouseMotion(self, event: tk.Event):
        self.c.moveMousePalette(int((event.x) / self.paletteCellSize),
                                int((event.y) / self.paletteCellSize))

    def clickRelease(self, event: tk.Event):
        self.isMapMiddleClicked = False
        self.c.releaseClick()

    def scriptTextChanged(self, event: tk.Event):
        self.scriptTextEntry.tag_remove('highlight', '1.0', 'end')
        self.scriptTextEntry.tag_add(
            "highlight", "insert linestart", "insert lineend")
        self.scriptTextEntry.tag_configure(
            "highlight", background="OliveDrab1", foreground="black")
        print(self.scriptTextEntry.get("insert linestart", "insert lineend"))

    def redraw(self):
        self.paletteCanvas.delete('all')
        self.mapCanvas.delete('all')

        # Draw backgrounds in the palette
        y: int = 0
        for bg in bgTiles:
            if bg is not tileType.EMPTY:
                self.paletteCanvas.create_image(
                    0, y*self.paletteCellSize, image=self.texMapPalette[bg], anchor=tk.NW)
            y = y+1

        # Draw objects in the palette
        y: int = 0
        for obj in objTiles:
            if obj is not tileType.EMPTY:
                self.paletteCanvas.create_image(
                    self.paletteCellSize, y*self.paletteCellSize, image=self.texMapPalette[obj], anchor=tk.NW)
            y = y+1

        # Draw the map
        for x in range(self.m.getMapWidth()):
            for y in range(self.m.getMapHeight()):
                self.drawMapCell(x, y)

        # Clear highlight from old cell
        self.mapCanvas.delete(self.highlightRect)
        # Highlight new cell
        self.highlightRect = self.mapCanvas.create_rectangle(
            self.mapOffsetX + (self.selRectX * self.mapCellSize), self.mapOffsetY + (self.selRectY * self.mapCellSize), self.mapOffsetX + ((self.selRectX + 1) * self.mapCellSize), self.mapOffsetY + ((self.selRectY + 1) * self.mapCellSize), outline='yellow')

    def drawMapCell(self, x, y):
        t: tile = self.m.tileMap[x][y]
        if (t.background is not tileType.EMPTY):
            self.mapCanvas.create_image(
                self.mapOffsetX + (x * self.mapCellSize), self.mapOffsetY + (y * self.mapCellSize), image=self.texMapMap[t.background], anchor=tk.NW)
        if (t.object is not tileType.EMPTY):
            self.mapCanvas.create_image(
                self.mapOffsetX + (x * self.mapCellSize), self.mapOffsetY + (y * self.mapCellSize), image=self.texMapMap[t.object], anchor=tk.NW)

    def drawSelectedTile(self, selectedTile: tileType):
        self.paletteSelected.delete('all')
        self.paletteSelected.create_image(self.paletteSelected.winfo_width(
        ) / 2, self.paletteSelected.winfo_height() / 2, image=self.texMapPalette[selectedTile], anchor=tk.CENTER)

    def selectCell(self, x, y):
        self.selRectX = x
        self.selRectY = y

        # Clear highlight from old cell
        self.mapCanvas.delete(self.highlightRect)
        # Highlight new cell
        self.highlightRect = self.mapCanvas.create_rectangle(
            self.mapOffsetX + (x * self.mapCellSize), self.mapOffsetY + (y * self.mapCellSize), self.mapOffsetX + ((x + 1) * self.mapCellSize), self.mapOffsetY + ((y + 1) * self.mapCellSize), outline='yellow')

        # Delete is going to erase anything
        # in the range of 0 and end of file,
        # The respective range given here
        self.cellMetaData.delete('1.0', 'end')

        # Insert method inserts the text at
        # specified position, Here it is the
        # beginning
        self.cellMetaData.insert('1.0', "[" + str(x) + ", " + str(y) + "]")
