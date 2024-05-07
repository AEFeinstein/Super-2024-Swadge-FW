import tkinter as tk
import math
from tkinter import filedialog
from xml.dom.minidom import parse,parseString
import os

SIG_BYTE_SIMPLE_SEQUENCE = 200 #This byte will capture long strings of bytes in compact form. Example [12][12][12]...[12] => [200][#][12] where # is number of 12 tiles

OBJECT_START_BYTE = 201
SKB_EMPTY             = 0
SKB_WALL              = 1
SKB_FLOOR             = 2
SKB_GOAL              = 3
SKB_NO_WALK           = 4
SKB_OBJSTART          = 201
SKB_COMPRESS          = 202
SKB_PLAYER            = 203
SKB_CRATE             = 204
SKB_WARPINTERNAL      = 205
SKB_WARPINTERNALEXIT  = 206
SKB_WARPEXTERNAL      = 207
SKB_BUTTON            = 208
SKB_LASEREMITTER      = 209
SKB_LASERRECEIVEROMNI = 210
SKB_LASERRECEIVER     = 211
SKB_LASER90ROTATE     = 212
SKB_GHOSTBLOCK        = 213
SKB_OBJEND            = 230

classToID = {
    "wall": SKB_WALL,
    "wal": SKB_WALL,
    "block": SKB_WALL,
    "floor": SKB_FLOOR,
    "ground": SKB_FLOOR,
    "goal":SKB_GOAL,
    "floornowalk":SKB_NO_WALK,
    "nowalk":SKB_NO_WALK,
    "nowalkfloor":SKB_NO_WALK,
    "empty":SKB_EMPTY,
    "nothing":SKB_EMPTY,
}

def insert_position(position, sourceList, insertionList):
    return sourceList[:position] + insertionList + sourceList[position:]

# root = tk.Tk()
# root.withdraw()

def convertTMX(file_path):
    document = parse(file_path)
    entities = {}
    mapHeaderWidth = int(document.getElementsByTagName("map")[0].attributes.getNamedItem("width").nodeValue)
    mapHeaderHeight = int(document.getElementsByTagName("map")[0].attributes.getNamedItem("height").nodeValue)
    mode = "SOKO_UNDEFINED"
    
    
    allProps = document.getElementsByTagName("property")
    for prop in allProps:
        if(prop.getAttribute("name") == "gamemode"):
            mode=prop.getAttribute("value")
            break
    if(mode == "SOKO_UNDEFINED"):
        print("Preprocessor Warning. "+file_path+" has no properly set gamemode. setting gamemode to SOKO_CLASSIC")
        mode = "SOKO_CLASSIC"
    modeInt = getModeInt(mode)
    # populate entities dictionary

    # loop through entities and add values.
    entityContainer = document.getElementsByTagName("objectgroup")[0]
    if(entityContainer.getAttribute("name") != "entities"): #todo: get length of container. number children, i guess?
        print("Warning, object layer not called 'entities' or there is more than one object layer. there should be just one, called entities.")
    
    for entity in entityContainer.getElementsByTagName("object"):
        ebytes = getEntityBytesFromEntity(entity)
        x = int(float(entity.getAttribute("x"))/16)
        y = int(float(entity.getAttribute("y"))/16)
        entities[str(x)+","+str(y)] = ebytes
    
    #get firstgid of tilesheet
    tileLookups = {} #offset from the tilesheet
    tilesets = document.getElementsByTagName("tileset")
    for tileset in tilesets:
        x =(int((tileset.getAttribute("firstgid"))))
        source = tileset.getAttribute("source")
        if(source != ""):
            current_dir = os.path.dirname(file_path)
            tpath = os.path.normpath(current_dir+"/"+source)
            if(os.path.splitext(tpath)[1] == ".tsx"):
                doc = parse(tpath)
                tileLookups[x] = loadTilesetLookup(doc)
        else:
            tileLookups[x] = loadTilesetLookup(tileset)

    dataText = document.getElementsByTagName("data")[0].firstChild.nodeValue
    scrub = "".join(dataText.split()) #Remove all residual whitespace in data block
    scrub = [(int(i)) for i in scrub.split(",")] #Convert all tileIDs to int.

    # fisrt, our HEADER data: width, height, modeint
    output = [mapHeaderWidth,mapHeaderHeight,modeInt]
    for i in range(len(scrub)):
        x = (i-1)%mapHeaderWidth
        y = ((i-1)//mapHeaderWidth)+1 #todo: figure out why this is +1 ????
        key = str(x)+","+str(y)
        #print("playing with "+key)
        if(key in entities):
            for b in entities[key]:
                output.append(b)
        output.append(int(getTile(scrub[i],tileLookups)))

    # output now is a list of tiles. 

    # add header information at start of tiles list.
    rawBytes = bytearray(output)
    rawBytesImmut = bytes(rawBytes)
    print("Converted "+file_path+" ("+str(mode)+")")
    return rawBytesImmut
    # outfile_path = "".join([file_path.split(".")[0],".bin"])
    # with open(outfile_path,"wb") as binary_file:
    #     binary_file.write(rawBytesImmut)


# These need to match the enum int casts in soko.h
def getModeInt(mode):
    mode = mode.upper()
    if(mode == "SOKO_OVERWORLD" or mode == "OVERWORLD"):
        return 0
    elif mode == "SOKO_CLASSIC" or mode == "CLASSIC":
        return 1
    elif mode == "SOKO_EULER" or mode == "EULER":
        return 2
    elif mode == "SOKO_LASER" or mode == "LASER" or mode == "SOKO_LASERBOUNCE" or mode == "LASERBOUNCE":
        return 3


def getEntityBytesFromEntity(entity):
    #todo: look up data in the tsx. which we have loaded? I think?
    #SKB_OBJSTART, SKB_[Object Type], [Data Bytes] , SKB_OBJEND
    gid = int(entity.getAttribute("gid"))
    otype = 0
    if(gid == 1):
        return [SKB_OBJSTART,SKB_PLAYER,SKB_OBJEND]
    elif(gid == 4):
        # index of destination or x,y?
        return [SKB_OBJSTART,SKB_WARPEXTERNAL,0,0,SKB_OBJEND]
    elif(gid == 2):
        flag = 0
        # bit 0 is sticky ob01
        # bit 1 is trail ob10
        # flag = sticky&trail (00,01,10, or 11)
        return [SKB_OBJSTART,SKB_CRATE,flag,SKB_OBJEND]
    # etc
    return []
    return [SKB_OBJSTART,SKB_CRATE,SKB_OBJEND]

def getTile(i,lookups):
    if(i == 0):
        # empty from tiled
        return 0 # whatever our empty is.
    for k,v in lookups.items():
        ix = i-k
        if ix in v:
            s = v[ix]
            if s in classToID:
                x = classToID[s]
                return x
    print("uh oh"+str(i)+"-"+str(k)+"-"+str(lookups))
    return i

def loadTilesetLookup(doc):
    # turn root object into dictonary of id's->classnames.
    tiles = doc.getElementsByTagName("tile")
    lookup = {}
    for tile in tiles:
        lookup[int(tile.getAttribute("id"))] = tile.getAttribute("type")

    return lookup


    