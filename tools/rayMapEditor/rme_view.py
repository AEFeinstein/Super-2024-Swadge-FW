import tkinter as tk
from tkinter import simpledialog
from tkinter import Event
from tkinter.filedialog import askopenfile
from tkinter.filedialog import asksaveasfile
from PIL import Image, ImageTk
from io import TextIOWrapper
import os

from collections.abc import Mapping

from rme_tiles import *
from rme_script_editor import spawn

from enum import Enum


class clickMode(Enum):
    NORMAL_EDIT = 1
    SET_SPAWN_TRIGGER_ZONE = 2
    SET_ENEMIES = 3


class CustomText(tk.Text):
    '''
    This class is a tk.Text which can also report when the cursor moves
    '''

    def __init__(self, *args, **kwargs):
        tk.Text.__init__(self, *args, **kwargs)

        # create a proxy for the underlying widget
        self._orig = self._w + "_orig"
        self.tk.call("rename", self._w, self._orig)
        self.tk.createcommand(self._w, self._proxy)

    def _proxy(self, *args):
        cmd = (self._orig,) + args
        try:
            result = self.tk.call(cmd)

            # generate an event if something was added or deleted,
            # or the cursor position changed
            if (args[0] in ("insert", "delete") or
                    args[0:3] == ("mark", "set", "insert")):
                self.event_generate("<<CursorChange>>", when="tail")

            return result
        except:
            pass


class view:

    def __init__(self):

        self.currentFilePath: str = None

        self.paletteCellSize: int = 64
        self.mapCellSize: int = 32

        self.isMapMiddleClicked: bool = False
        self.isMapRightClicked: bool = False

        self.selRectX: int = 0
        self.selRectY: int = 0

        self.highlightRect: int = -1
        self.paletteHighlightRect: int = -1

        self.scriptRects = []

        frameBgColor: str = '#181818'
        elemBgColor: str = '#1F1F1F'
        borderColor: str = '#2A2A2A'
        borderHighlightColor: str = '#2B79D7'
        fontColor: str = '#CCCCCC'
        fontStyle = ('Courier New', 14)
        borderThickness: int = 2
        padding: int = 4

        self.buttonColor: str = '#2B79D7'
        self.buttonPressedColor: str = '#5191DE'
        self.buttonFontColor: str = '#FFFFFF'
        buttonWidth: int = 12

        # Setup the root and main frames
        self.root: tk.Tk = tk.Tk()
        self.root.title("Ray Map Editor")
        self.root.bind('<KeyPress>', self.key_press)
        content = tk.Frame(self.root, background=frameBgColor)
        frame = tk.Frame(content, background=frameBgColor)

        # Setup the button frame and buttons
        self.buttonFrame: tk.Frame = tk.Frame(content, height=0, background=elemBgColor,
                                              highlightthickness=borderThickness, highlightbackground=borderColor, padx=padding, pady=padding)
        self.loadButton: tk.Button = tk.Button(self.buttonFrame, height=1, width=buttonWidth, text="Load", font=fontStyle, background=self.buttonColor,
                                               foreground=self.buttonFontColor, activebackground=self.buttonPressedColor, activeforeground=self.buttonFontColor, bd=0,
                                               command=self.clickLoad)
        self.saveButton: tk.Button = tk.Button(self.buttonFrame, height=1, width=buttonWidth, text="Save", font=fontStyle, background=self.buttonColor,
                                               foreground=self.buttonFontColor, activebackground=self.buttonPressedColor, activeforeground=self.buttonFontColor, bd=0,
                                               command=self.clickSave)
        self.saveAsButton: tk.Button = tk.Button(self.buttonFrame, height=1, width=buttonWidth, text="Save As", font=fontStyle, background=self.buttonColor,
                                                 foreground=self.buttonFontColor, activebackground=self.buttonPressedColor, activeforeground=self.buttonFontColor, bd=0,
                                                 command=self.clickSaveAs)
        self.resizeMap: tk.Button = tk.Button(self.buttonFrame, height=1, width=buttonWidth, text="Resize Map", font=fontStyle, background=self.buttonColor,
                                              foreground=self.buttonFontColor, activebackground=self.buttonPressedColor, activeforeground=self.buttonFontColor, bd=0,
                                              command=self.clickResizeMap)
        self.scriptSpawn: tk.Button = tk.Button(self.buttonFrame, height=1, width=buttonWidth, text="Set E.Triggers", font=fontStyle, background=self.buttonColor,
                                                foreground=self.buttonFontColor, activebackground=self.buttonPressedColor, activeforeground=self.buttonFontColor, bd=0,
                                                command=self.clickScriptSpawn)
        self.scriptSpawnState: clickMode = clickMode.NORMAL_EDIT
        self.exitButton: tk.Button = tk.Button(self.buttonFrame, height=1, width=buttonWidth, text="Exit", font=fontStyle, background=self.buttonColor,
                                               foreground=self.buttonFontColor, activebackground=self.buttonPressedColor, activeforeground=self.buttonFontColor, bd=0,
                                               command=self.clickExit)

        # Set up the canvasses
        self.paletteCanvas: tk.Canvas = tk.Canvas(
            content, background=elemBgColor, width=self.paletteCellSize * 4, height=self.paletteCellSize * 8,
            highlightthickness=borderThickness, highlightbackground=borderColor)
        self.mapCanvas: tk.Canvas = tk.Canvas(
            content, background=elemBgColor, highlightthickness=borderThickness, highlightbackground=borderColor)

        # Set up the text
        self.cellMetaData: CustomText = CustomText(content, width=10, state="disabled",
                                                   undo=True, autoseparators=True, maxundo=-1,
                                                   background=elemBgColor, foreground=fontColor, insertbackground=fontColor, font=fontStyle,
                                                   highlightthickness=borderThickness, highlightbackground=borderColor,
                                                   highlightcolor=borderHighlightColor, borderwidth=0, bd=0)

        self.scriptTextEntry: CustomText = CustomText(content, height=10,
                                                      undo=True, autoseparators=True, maxundo=-1,
                                                      background=elemBgColor, foreground=fontColor, insertbackground=fontColor, font=fontStyle,
                                                      highlightthickness=borderThickness, highlightbackground=borderColor,
                                                      highlightcolor=borderHighlightColor, borderwidth=0, bd=0, wrap='none')
        self.scriptTextEntry.bind(
            "<<CursorChange>>", self.scriptTextCursorChange)

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
        self.scriptSpawn.grid(column=4, row=0, sticky=tk.NW,
                              padx=padding, pady=padding)
        # Place this button in the top right
        self.exitButton.grid(column=5, row=0, sticky=tk.NE,
                             padx=padding, pady=padding)

        # Configure button column weights
        self.buttonFrame.columnconfigure(0, weight=0)
        self.buttonFrame.columnconfigure(1, weight=0)
        self.buttonFrame.columnconfigure(2, weight=0)
        self.buttonFrame.columnconfigure(3, weight=0)
        self.buttonFrame.columnconfigure(4, weight=1)

        # Place the palette and bind events
        self.paletteCanvas.grid(column=0, row=1, sticky=(
            tk.NSEW), padx=padding, pady=padding)
        self.paletteCanvas.bind("<Button-1>", self.paletteLeftClick)
        self.paletteCanvas.bind('<ButtonRelease-1>', self.clickRelease)
        self.paletteCanvas.bind('<Motion>', self.paletteMouseMotion)

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
        self.mapCanvas.bind("<MouseWheel>", self.mapMouseWheel)
        self.mapCanvas.bind("<Button-4>", self.mapMouseWheel)
        self.mapCanvas.bind("<Button-5>", self.mapMouseWheel)

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

        self.loadAllTextures()

        # Start maximized
        self.root.wm_state('normal')  # 'zoomed' works for windows

    def loadAllTextures(self):

        # Empty these first
        self.texMapPalette: Mapping[tileType, ImageTk.PhotoImage] = {}
        self.texMapMap: Mapping[tileType, ImageTk.PhotoImage] = {}

        # Load all textures
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_FLOOR, '../../assets/ray/env/BASE/BG_BASE_FLOOR.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_FLOOR_WATER, '../../assets/ray/env/BG_FLOOR_WATER.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_FLOOR_LAVA, '../../assets/ray/env/BG_FLOOR_LAVA.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_WALL_1, '../../assets/ray/env/BASE/BG_BASE_WALL_1.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_WALL_2, '../../assets/ray/env/BASE/BG_BASE_WALL_2.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_WALL_3, '../../assets/ray/env/BASE/BG_BASE_WALL_3.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_WALL_4, '../../assets/ray/env/BASE/BG_BASE_WALL_4.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_WALL_5, '../../assets/ray/env/BASE/BG_BASE_WALL_5.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_DOOR, '../../assets/ray/doors/BG_DOOR.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_DOOR_CHARGE, '../../assets/ray/doors/BG_DOOR_CHARGE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_DOOR_MISSILE, '../../assets/ray/doors/BG_DOOR_MISSILE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_DOOR_ICE, '../../assets/ray/doors/BG_DOOR_ICE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_DOOR_XRAY, '../../assets/ray/doors/BG_DOOR_XRAY.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_DOOR_SCRIPT, '../../assets/ray/doors/BG_DOOR_SCRIPT.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_DOOR_KEY_A, '../../assets/ray/doors/BG_DOOR_KEY_A.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_DOOR_KEY_B, '../../assets/ray/doors/BG_DOOR_KEY_B.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_DOOR_KEY_C, '../../assets/ray/doors/BG_DOOR_KEY_C.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.BG_DOOR_ARTIFACT, '../../assets/ray/doors/BG_DOOR_ARTIFACT.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ENEMY_START_POINT, 'imgs/OBJ_ENEMY_START_POINT.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ENEMY_NORMAL, '../../assets/ray/enemies/NORMAL/E_NORMAL_WALK_0_0.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ENEMY_STRONG, '../../assets/ray/enemies/STRONG/E_STRONG_WALK_0_0.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ENEMY_ARMORED, '../../assets/ray/enemies/ARMORED/E_ARMORED_WALK_0_0.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ENEMY_FLAMING, '../../assets/ray/enemies/FLAMING/E_FLAMING_WALK_0_0.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ENEMY_HIDDEN, '../../assets/ray/enemies/HIDDEN/E_HIDDEN_WALK_0_0.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ENEMY_BOSS, '../../assets/ray/enemies/BOSS/E_BOSS_WALK_0_0.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ITEM_BEAM, '../../assets/ray/items/OBJ_ITEM_BEAM.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ITEM_CHARGE_BEAM, '../../assets/ray/items/OBJ_ITEM_CHARGE_BEAM.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ITEM_MISSILE, '../../assets/ray/items/OBJ_ITEM_MISSILE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ITEM_ICE, '../../assets/ray/items/OBJ_ITEM_ICE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ITEM_XRAY, '../../assets/ray/items/OBJ_ITEM_XRAY.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ITEM_SUIT_WATER, '../../assets/ray/items/OBJ_ITEM_SUIT_WATER.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ITEM_SUIT_LAVA, '../../assets/ray/items/OBJ_ITEM_SUIT_LAVA.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ITEM_ENERGY_TANK, '../../assets/ray/items/OBJ_ITEM_ENERGY_TANK.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ITEM_KEY_A, '../../assets/ray/items/OBJ_ITEM_KEY_A.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ITEM_KEY_B, '../../assets/ray/items/OBJ_ITEM_KEY_B.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ITEM_KEY_C, '../../assets/ray/items/OBJ_ITEM_KEY_C.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ITEM_ARTIFACT, '../../assets/ray/items/OBJ_ITEM_ARTIFACT.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_ITEM_PICKUP_ENERGY, '../../assets/ray/items/OBJ_ITEM_PICKUP_ENERGY.png')
        self.loadTexture(self.texMapPalette, self.texMapMap, tileType.OBJ_ITEM_PICKUP_MISSILE,
                         '../../assets/ray/items/OBJ_ITEM_PICKUP_MISSILE.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_SCENERY_TERMINAL, '../../assets/ray/scenery/OBJ_SCENERY_TERMINAL.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.OBJ_SCENERY_PORTAL, '../../assets/ray/scenery/OBJ_SCENERY_PORTAL.png')
        self.loadTexture(self.texMapPalette, self.texMapMap,
                         tileType.DELETE, 'imgs/DELETE.png')

    def loadTexture(self, pMap, mMap, key, texFile):
        img = Image.open(texFile)

        # Resize for the palette
        if (img.width < img.height):
            newHeight = self.paletteCellSize
            newWidth = img.width * (newHeight / img.height)
        else:
            newWidth = self.paletteCellSize
            newHeight = img.height * (newWidth / img.width)
        pResize = img.resize(
            size=(int(newWidth), int(newHeight)), resample=Image.LANCZOS)

        pMap[key] = ImageTk.PhotoImage(pResize)

        # Resize for the map
        if (img.width < img.height):
            newHeight = self.mapCellSize
            newWidth = img.width * (newHeight / img.height)
        else:
            newWidth = self.mapCellSize
            newHeight = img.height * (newWidth / img.width)

        mResize = img.resize(
            size=(int(newWidth), int(newHeight)), resample=Image.LANCZOS)
        mMap[key] = ImageTk.PhotoImage(mResize)

    def setController(self, c):
        from rme_controller import controller
        self.c: controller = c

    def setModel(self, m):
        from rme_model import model
        self.m: model = m

    def mainloop(self):
        self.root.mainloop()

    def setMapCellSize(self, newMapCellSize):
        # Bound the cell size between 8 and 64px
        if newMapCellSize > 64:
            newMapCellSize = 64
        elif newMapCellSize < 8:
            newMapCellSize = 8

        # Set the new cell size
        self.mapCellSize = newMapCellSize
        # Reload all textures
        self.loadAllTextures()
        # Redraw the UI
        self.redraw()

    def key_press(self, e: Event):
        # Keyboard shortcut, Ctrl+E is the same as the script button
        if (e.state == 20) and (e.keycode == 26):
            self.clickScriptSpawn()
        # ctrl+S saves
        if (e.state == 20) and (e.keycode == 39):
            self.clickSave()
        # ctrl+shift+S save as
        if (e.state == 21) and (e.keycode == 39):
            self.clickSaveAs()
        # ctrl+o opens
        if (e.state == 20) and (e.keycode == 32):
            self.clickLoad()
        # ctrl+r resize
        if (e.state == 20) and (e.keycode == 27):
            self.clickResizeMap()

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
                            int(y / self.mapCellSize), self.scriptSpawnState)

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
                                int(y / self.mapCellSize), self.scriptSpawnState)

    def mapMouseWheel(self, event: tk.Event):
        if (4 == event.num) or (event.delta < 0):
            self.setMapCellSize(self.mapCellSize + 4)
        elif (5 == event.num) or (event.delta > 0):
            self.setMapCellSize(self.mapCellSize - 4)
        pass

    def clickRelease(self, event: tk.Event):
        self.isMapMiddleClicked = False
        self.isMapRightClicked = False
        self.c.releaseClick()

    def scriptTextCursorChange(self, event):
        # Highlight script cells when the cursor changes
        self.highlightScriptCells()

    def highlightScriptCells(self):
        # Get the currently selected line
        line, char = self.scriptTextEntry.index("insert").split(".")
        scriptNum = int(line) - 1

        # Clear highlight from old cell
        while 0 != len(self.scriptRects):
            rect = self.scriptRects.pop()
            self.mapCanvas.delete(rect)

        # If there are scripts
        if scriptNum >= 0 and scriptNum < len(self.m.scripts):

            lineWidth = 4
            dashPattern = (8, 8)

            # Get all the if cells and highlight them yellow
            for cell in self.m.scripts[scriptNum].getIfCells():
                if cell is not None:
                    self.scriptRects.append(self.mapCanvas.create_rectangle(
                        (cell[0] * self.mapCellSize),
                        (cell[1] * self.mapCellSize),
                        ((cell[0] + 1) * self.mapCellSize),
                        ((cell[1] + 1) * self.mapCellSize),
                        outline='yellow', width=lineWidth, dash=dashPattern))

            # Get all object references and also highlight them yellow
            for id in self.m.scripts[scriptNum].getIfIds():
                for y in range(self.m.getMapHeight()):
                    for x in range(self.m.getMapWidth()):
                        if (id == self.m.tileMap[x][y].objectId):
                            self.scriptRects.append(self.mapCanvas.create_rectangle(
                                (x * self.mapCellSize),
                                (y * self.mapCellSize),
                                ((x + 1) * self.mapCellSize),
                                ((y + 1) * self.mapCellSize),
                                outline='yellow', width=lineWidth, dash=dashPattern))

            # Get all the then cells and highlight them DeepPink
            for cell in self.m.scripts[scriptNum].getThenCells():
                if cell is not None:
                    self.scriptRects.append(self.mapCanvas.create_rectangle(
                        (cell[0] * self.mapCellSize),
                        (cell[1] * self.mapCellSize),
                        ((cell[0] + 1) * self.mapCellSize),
                        ((cell[1] + 1) * self.mapCellSize),
                        outline='DeepPink', width=lineWidth, dash=dashPattern))

            # Get all object references and also highlight them DeepPink
            for id in self.m.scripts[scriptNum].getThenIds():
                for y in range(self.m.getMapHeight()):
                    for x in range(self.m.getMapWidth()):
                        if (id == self.m.tileMap[x][y].objectId):
                            self.scriptRects.append(self.mapCanvas.create_rectangle(
                                (x * self.mapCellSize),
                                (y * self.mapCellSize),
                                ((x + 1) * self.mapCellSize),
                                ((y + 1) * self.mapCellSize),
                                outline='DeepPink', width=lineWidth, dash=dashPattern))

            for spawn in self.m.scripts[scriptNum].getThenSpawns():
                imgWidth: int = self.texMapMap[spawn.type].width()
                hOffset = int((self.mapCellSize - imgWidth) / 2)
                self.scriptRects.append(self.mapCanvas.create_image(
                    (spawn.x * self.mapCellSize) + hOffset, (spawn.y * self.mapCellSize), image=self.texMapMap[spawn.type], anchor=tk.NW))

        # If the enemy script is being built
        if self.m.enemyScript is not None:
            # Highlight the trigger cells
            for cell in self.m.enemyScript.getIfCells():
                if cell is not None:
                    self.scriptRects.append(self.mapCanvas.create_rectangle(
                        (cell[0] * self.mapCellSize),
                        (cell[1] * self.mapCellSize),
                        ((cell[0] + 1) * self.mapCellSize),
                        ((cell[1] + 1) * self.mapCellSize),
                        outline='blue', width=4))
            # Draw the spawns
            for spawn in self.m.enemyScript.getThenSpawns():
                imgWidth: int = self.texMapMap[spawn.type].width()
                hOffset = int((self.mapCellSize - imgWidth) / 2)
                self.scriptRects.append(self.mapCanvas.create_image(
                    (spawn.x * self.mapCellSize) + hOffset, (spawn.y * self.mapCellSize), image=self.texMapMap[spawn.type], anchor=tk.NW))

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
        self.highlightScriptCells()

    def redraw(self):
        self.paletteCanvas.delete('all')
        self.mapCanvas.delete('all')

        # Draw backgrounds in the palette
        x: int = 0
        y: int = 0
        for col in bgTiles:
            for bg in col:
                if bg is not tileType.EMPTY:
                    self.paletteCanvas.create_image(
                        x*self.paletteCellSize, y*self.paletteCellSize, image=self.texMapPalette[bg], anchor=tk.NW)
                y = y+1
            x = x+1
            y = 0

        # Draw objects in the palette into two columns
        for col in objTiles:
            for obj in col:
                if obj is not tileType.EMPTY:

                    imgWidth: int = self.texMapPalette[obj].width()
                    hOffset = int((self.paletteCellSize - imgWidth) / 2)

                    self.paletteCanvas.create_image(
                        x*self.paletteCellSize + hOffset, y*self.paletteCellSize, image=self.texMapPalette[obj], anchor=tk.NW)
                y = y+1
            x = x+1
            y = 0

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
            imgWidth: int = self.texMapMap[t.background].width()
            hOffset = int((self.mapCellSize - imgWidth) / 2)

            self.mapCanvas.create_image(
                (x * self.mapCellSize) + hOffset, (y * self.mapCellSize), image=self.texMapMap[t.background], anchor=tk.NW)
        if (t.object is not tileType.EMPTY):
            imgWidth: int = self.texMapMap[t.object].width()
            hOffset = int((self.mapCellSize - imgWidth) / 2)

            self.mapCanvas.create_image(
                (x * self.mapCellSize) + hOffset, (y * self.mapCellSize), image=self.texMapMap[t.object], anchor=tk.NW)

    def drawSelectedTile(self, selectedTile: tileType, x, y):
        # Clear highlight from old cell
        self.paletteCanvas.delete(self.paletteHighlightRect)
        # Highlight new cell
        self.paletteHighlightRect = self.paletteCanvas.create_rectangle(
            (x * self.paletteCellSize), (y * self.paletteCellSize), ((x + 1) * self.paletteCellSize), ((y + 1) * self.paletteCellSize), outline='yellow', width=5)
        pass

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
        fileToLoad: TextIOWrapper = askopenfile(mode='rb', filetypes=fts)
        self.loadFile(fileToLoad)

    def loadFile(self, fileToLoad):
        if fileToLoad is not None:
            self.currentFilePath = os.path.abspath(fileToLoad.name)
            self.m.load(fileToLoad)

            # Redraw map
            self.redraw()

            # Redraw scripts
            self.reloadScriptText()

            self.root.title(os.path.basename(self.currentFilePath))

    def reloadScriptText(self):
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

    def clickScriptSpawn(self):
        if clickMode.NORMAL_EDIT == self.scriptSpawnState:
            self.scriptSpawnState = clickMode.SET_SPAWN_TRIGGER_ZONE
            self.scriptSpawn.config(text="Set Enemies")
            self.scriptSpawn.config(background='green')
            self.scriptSpawn.config(activebackground='green')
            self.m.startScriptCreation()
        elif clickMode.SET_SPAWN_TRIGGER_ZONE == self.scriptSpawnState:
            self.scriptSpawnState = clickMode.SET_ENEMIES
            self.scriptSpawn.config(text="Finish Script")
            self.scriptSpawn.config(background='red')
            self.scriptSpawn.config(activebackground='red')
        elif clickMode.SET_ENEMIES == self.scriptSpawnState:
            self.scriptSpawnState = clickMode.NORMAL_EDIT
            self.scriptSpawn.config(text="Set E.Triggers")
            self.scriptSpawn.config(background=self.buttonColor)
            self.scriptSpawn.config(activebackground=self.buttonPressedColor)
            self.m.finishScriptCreation()

    def clickExit(self):
        if self.currentFilePath is not None:
            self.clickSave()
        else:
            with open('autosave.rmd', 'wb') as outFile:
                self.m.save(outFile)
        self.root.destroy()
