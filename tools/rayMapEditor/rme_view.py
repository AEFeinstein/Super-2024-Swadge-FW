import tkinter as tk
from tkinter import simpledialog
from tkinter.filedialog import askopenfile
from tkinter.filedialog import asksaveasfile
from PIL import Image, ImageTk
from io import TextIOWrapper
import os

from collections.abc import Mapping

from rme_tiles import *
from rme_script_editor import rme_scriptSplitter
from rme_script_editor import rme_script


class view:

    def __init__(self):

        self.splitter: rme_scriptSplitter = rme_scriptSplitter()
        self.currentFilePath: str = None

        self.paletteCellSize: int = 64
        self.mapCellSize: int = 32

        self.isMapMiddleClicked: bool = False
        self.isMapRightClicked: bool = False

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
        buttonWidth: int = 12

        # Setup the root and main frames
        self.root: tk.Tk = tk.Tk()
        self.root.title("Ray Map Editor")
        content = tk.Frame(self.root, background=frameBgColor)
        frame = tk.Frame(content, background=frameBgColor)

        # Setup the button frame and buttons
        self.buttonFrame: tk.Frame = tk.Frame(content, height=0, background=elemBgColor,
                                              highlightthickness=borderThickness, highlightbackground=borderColor, padx=padding, pady=padding)
        self.loadButton: tk.Button = tk.Button(self.buttonFrame, height=1, width=buttonWidth, text="Load", font=fontStyle, background=buttonColor,
                                               foreground=buttonFontColor, activebackground=buttonPressedColor, activeforeground=buttonFontColor, bd=0,
                                               command=self.clickLoad)
        self.saveButton: tk.Button = tk.Button(self.buttonFrame, height=1, width=buttonWidth, text="Save", font=fontStyle, background=buttonColor,
                                               foreground=buttonFontColor, activebackground=buttonPressedColor, activeforeground=buttonFontColor, bd=0,
                                               command=self.clickSave)
        self.saveAsButton: tk.Button = tk.Button(self.buttonFrame, height=1, width=buttonWidth, text="Save As", font=fontStyle, background=buttonColor,
                                                 foreground=buttonFontColor, activebackground=buttonPressedColor, activeforeground=buttonFontColor, bd=0,
                                                 command=self.clickSaveAs)
        self.resizeMap: tk.Button = tk.Button(self.buttonFrame, height=1, width=buttonWidth, text="Resize Map", font=fontStyle, background=buttonColor,
                                              foreground=buttonFontColor, activebackground=buttonPressedColor, activeforeground=buttonFontColor, bd=0,
                                              command=self.clickResizeMap)
        self.exitButton: tk.Button = tk.Button(self.buttonFrame, height=1, width=buttonWidth, text="Exit", font=fontStyle, background=buttonColor,
                                               foreground=buttonFontColor, activebackground=buttonPressedColor, activeforeground=buttonFontColor, bd=0,
                                               command=self.clickExit)

        # Set up the canvasses
        self.paletteCanvas: tk.Canvas = tk.Canvas(
            content, background=elemBgColor, width=self.paletteCellSize * 3, height=self.paletteCellSize * 8,
            highlightthickness=borderThickness, highlightbackground=borderColor)
        self.paletteSelected: tk.Canvas = tk.Canvas(
            content, background=elemBgColor, width=self.paletteCellSize * 3, height=self.paletteCellSize * 2,
            highlightthickness=borderThickness, highlightbackground=borderColor)
        self.mapCanvas: tk.Canvas = tk.Canvas(
            content, background=elemBgColor, highlightthickness=borderThickness, highlightbackground=borderColor)

        # Set up the text
        self.cellMetaData: tk.Text = tk.Text(content, width=40, state="disabled",
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
        self.buttonFrame.grid(column=0, row=0, columnspan=4,
                              rowspan=1, sticky=tk.NSEW, padx=padding, pady=padding)

        # Place the buttons in the button bar
        self.loadButton.grid(column=0, row=0, sticky=tk.NW,
                             padx=padding, pady=padding)
        self.saveButton.grid(column=1, row=0, sticky=tk.NW,
                             padx=padding, pady=padding)
        self.saveAsButton.grid(column=2, row=0, sticky=tk.NW,
                               padx=padding, pady=padding)
        self.resizeMap.grid(column=3, row=0, sticky=tk.NW,
                            padx=padding, pady=padding)
        # Place this button in the top right
        self.exitButton.grid(column=4, row=0, sticky=tk.NE,
                             padx=padding, pady=padding)

        # Configure button column weights
        self.buttonFrame.columnconfigure(0, weight=0)
        self.buttonFrame.columnconfigure(1, weight=0)
        self.buttonFrame.columnconfigure(2, weight=0)
        self.buttonFrame.columnconfigure(3, weight=1)

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

        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.BG_FLOOR, 'imgs/BG_FLOOR.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.BG_FLOOR_WATER, 'imgs/BG_FLOOR_WATER.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.BG_FLOOR_LAVA, 'imgs/BG_FLOOR_LAVA.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.BG_WALL_1, 'imgs/BG_WALL_1.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.BG_WALL_2, 'imgs/BG_WALL_2.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.BG_WALL_3, 'imgs/BG_WALL_3.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.BG_DOOR, 'imgs/BG_DOOR.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.BG_DOOR_CHARGE, 'imgs/BG_DOOR_CHARGE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.BG_DOOR_MISSILE, 'imgs/BG_DOOR_MISSILE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.BG_DOOR_ICE, 'imgs/BG_DOOR_ICE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.BG_DOOR_XRAY, 'imgs/BG_DOOR_XRAY.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_START_POINT, 'imgs/OBJ_START_POINT.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ENEMY_BEAM, 'imgs/OBJ_ENEMY_BEAM.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ENEMY_CHARGE, 'imgs/OBJ_ENEMY_CHARGE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ENEMY_MISSILE, 'imgs/OBJ_ENEMY_MISSILE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ENEMY_ICE, 'imgs/OBJ_ENEMY_ICE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ENEMY_XRAY, 'imgs/OBJ_ENEMY_XRAY.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ITEM_BEAM, 'imgs/OBJ_ITEM_BEAM.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ITEM_CHARGE_BEAM, 'imgs/OBJ_ITEM_CHARGE_BEAM.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ITEM_MISSILE, 'imgs/OBJ_ITEM_MISSILE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ITEM_ICE, 'imgs/OBJ_ITEM_ICE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ITEM_XRAY, 'imgs/OBJ_ITEM_XRAY.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ITEM_SUIT_WATER, 'imgs/OBJ_ITEM_SUIT_WATER.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ITEM_SUIT_LAVA, 'imgs/OBJ_ITEM_SUIT_LAVA.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ITEM_ENERGY_TANK, 'imgs/OBJ_ITEM_ENERGY_TANK.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ITEM_KEY, 'imgs/OBJ_ITEM_KEY.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ITEM_ARTIFACT, 'imgs/OBJ_ITEM_ARTIFACT.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ITEM_PICKUP_ENERGY, 'imgs/OBJ_ITEM_PICKUP_ENERGY.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ITEM_PICKUP_MISSILE, 'imgs/OBJ_ITEM_PICKUP_MISSILE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_SCENERY_TERMINAL, 'imgs/OBJ_SCENERY_TERMINAL.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_DELETE, 'imgs/OBJ_DELETE.png')

        # Start maximized
        self.root.state('zoomed')

    def loadTexture(self, pMap, mMap, key, texFile):
        img = Image.open(texFile)
        pResize = img.resize(
            size=(self.paletteCellSize, self.paletteCellSize), resample=Image.LANCZOS)
        pMap[key] = ImageTk.PhotoImage(pResize)
        mResize = img.resize(
            size=(self.mapCellSize, self.mapCellSize), resample=Image.LANCZOS)
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
        x: int = self.paletteCanvas.canvasx(event.x)
        y: int = self.paletteCanvas.canvasy(event.y)
        self.c.clickPalette(int(x / self.paletteCellSize),
                            int(y / self.paletteCellSize))

    def paletteMouseMotion(self, event: tk.Event):
        x: int = self.paletteCanvas.canvasx(event.x)
        y: int = self.paletteCanvas.canvasy(event.y)
        self.c.moveMousePalette(int(x / self.paletteCellSize),
                                int(y / self.paletteCellSize))

    def mapLeftClick(self, event: tk.Event):
        x: int = self.mapCanvas.canvasx(event.x)
        y: int = self.mapCanvas.canvasy(event.y)
        self.c.leftClickMap(int(x / self.mapCellSize),
                            int(y / self.mapCellSize))

    def mapRightClick(self, event: tk.Event):
        self.isMapRightClicked = True
        x: int = self.mapCanvas.canvasx(event.x)
        y: int = self.mapCanvas.canvasy(event.y)
        self.c.rightClickMap(int(x / self.mapCellSize),
                             int(y / self.mapCellSize))

    def mapMiddleClick(self, event: tk.Event):
        self.isMapMiddleClicked = True
        self.mapCanvas.scan_mark(event.x, event.y)

    def mapMouseMotion(self, event: tk.Event):
        if self.isMapMiddleClicked:
            self.mapCanvas.scan_dragto(event.x, event.y, gain=1)
        elif self.isMapRightClicked:
            self.mapRightClick(event)
        else:
            x: int = self.mapCanvas.canvasx(event.x)
            y: int = self.mapCanvas.canvasy(event.y)
            self.c.moveMouseMap(int(x / self.mapCellSize),
                                int(y / self.mapCellSize))

    def clickRelease(self, event: tk.Event):
        self.isMapMiddleClicked = False
        self.isMapRightClicked = False
        self.c.releaseClick()

    def scriptTextChanged(self, event: tk.Event):

        self.m.setScripts(self.scriptTextEntry.get(
            "1.0", tk.END).splitlines(keepends=False))

        line = 1
        for script in self.m.scripts:
            tag: str = 'highlight' + str(line)
            self.scriptTextEntry.tag_remove(
                tag, str(line) + '.0', str(line) + '.0 lineend')
            self.scriptTextEntry.tag_add(
                tag, str(line) + '.0', str(line) + '.0 lineend')
            if script.isValid():
                self.scriptTextEntry.tag_configure(
                    tag, background="green", foreground="black")
            else:
                self.scriptTextEntry.tag_configure(
                    tag, background="red", foreground="black")
            line = line + 1

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

        # Draw objects in the palette into two columns
        x: int = 1
        y: int = 0
        for obj in objTiles:
            if obj is not tileType.EMPTY:
                self.paletteCanvas.create_image(
                    x*self.paletteCellSize, y*self.paletteCellSize, image=self.texMapPalette[obj], anchor=tk.NW)
            y = y+1
            if 11 == y:
                y = 0
                x = x+1

        # Draw the map
        for x in range(self.m.getMapWidth()):
            for y in range(self.m.getMapHeight()):
                self.drawMapCell(x, y)

        # Clear highlight from old cell
        self.mapCanvas.delete(self.highlightRect)
        # Highlight new cell
        self.highlightRect = self.mapCanvas.create_rectangle(
            (self.selRectX * self.mapCellSize), (self.selRectY * self.mapCellSize), ((self.selRectX + 1) * self.mapCellSize), ((self.selRectY + 1) * self.mapCellSize), outline='yellow')

    def drawMapCell(self, x, y):
        t: tile = self.m.tileMap[x][y]
        if (t.background is not tileType.EMPTY):
            self.mapCanvas.create_image(
                (x * self.mapCellSize), (y * self.mapCellSize), image=self.texMapMap[t.background], anchor=tk.NW)
        if (t.object is not tileType.EMPTY):
            self.mapCanvas.create_image(
                (x * self.mapCellSize), (y * self.mapCellSize), image=self.texMapMap[t.object], anchor=tk.NW)

    def drawSelectedTile(self, selectedTile: tileType):
        self.paletteSelected.delete('all')
        self.paletteSelected.create_image(self.paletteSelected.winfo_width(
        ) / 2, self.paletteSelected.winfo_height() / 2, image=self.texMapPalette[selectedTile], anchor=tk.CENTER)

    def selectCell(self, x, y, objId):
        self.selRectX = x
        self.selRectY = y

        # Clear highlight from old cell
        self.mapCanvas.delete(self.highlightRect)
        # Highlight new cell
        self.highlightRect = self.mapCanvas.create_rectangle(
            (x * self.mapCellSize), (y * self.mapCellSize), ((x + 1) * self.mapCellSize), ((y + 1) * self.mapCellSize), outline='yellow')

        # Enable the text for writing
        self.cellMetaData.configure(state='normal')

        # Delete is going to erase anything
        # in the range of 0 and end of file,
        # The respective range given here
        self.cellMetaData.delete('1.0', 'end')

        # Insert method inserts the text at
        # specified position, Here it is the
        # beginning
        self.cellMetaData.insert(
            '1.0', "{" + str(x) + "." + str(y) + "}")
        if objId >= 0:
            self.cellMetaData.insert(
                '2.0', "\nID: " + str(objId))

        # Disable the text for writing
        self.cellMetaData.configure(state='normal')

    def clickSave(self):
        if self.currentFilePath is None:
            self.clickSaveAs()
        else:
            with open(self.currentFilePath, 'wb') as saveFile:
                self.m.save(saveFile)

    def clickSaveAs(self):
        fts = (
            ('Ray Map Data', '*.rmd'),
            ('All files', '*.*')
        )
        saveFile: TextIOWrapper = asksaveasfile(
            mode='wb', filetypes=fts, defaultextension='rmd')
        if saveFile is not None:
            self.currentFilePath = os.path.abspath(saveFile.name)
            self.m.save(saveFile)

    def clickLoad(self):
        fts = (
            ('Ray Map Data', '*.rmd'),
            ('All files', '*.*')
        )
        loadFile: TextIOWrapper = askopenfile(mode='rb', filetypes=fts)
        if loadFile is not None:
            self.currentFilePath = os.path.abspath(loadFile.name)
            self.m.load(loadFile)

            # Redraw map
            self.redraw()

            # Clear and set script text
            self.scriptTextEntry.delete('1.0', tk.END)
            for script in self.m.scripts:
                self.scriptTextEntry.insert(tk.END, script.toString() + '\n')
            # Highlight text
            self.scriptTextChanged(None)

    def clickResizeMap(self):
        inputStr: str = str(self.m.getMapWidth()) + 'x' + \
            str(self.m.getMapHeight())
        validInput: bool = True
        while True:
            if validInput:
                inputStr = simpledialog.askstring(
                    'Map Size', 'Enter the map size (w x h)', initialvalue=inputStr)
            else:
                inputStr = simpledialog.askstring(
                    'Map Size', 'Invalid value\nEnter the map size (w x h)', initialvalue=inputStr)

            if None is inputStr:
                # Cancel pressed
                return

            # Validate value
            try:
                # Pick out the dimensions
                parts: list[str] = inputStr.split('x')
                newW: int = int(parts[0].strip())
                newH: int = int(parts[1].strip())
                if 0 < newW and newW < 256 and 0 < newH and newH < 256:
                    # Resize the map
                    self.m.setMapSize(newW, newH)
                    # Redraw map and revalidate scripts
                    self.redraw()
                    self.scriptTextChanged(None)
                    return
                else:
                    validInput = False
            except:
                # Validation failed, try again
                validInput = False

    def clickExit(self):
        if self.currentFilePath is not None:
            self.clickSave()
        else:
            with open('autosave.rmd', 'wb') as outFile:
                self.m.save(outFile)
        self.root.destroy()
