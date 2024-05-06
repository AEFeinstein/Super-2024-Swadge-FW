import tkinter as tk
import math
from tkinter import filedialog
from xml.dom.minidom import parse,parseString

SIG_BYTE_SIMPLE_SEQUENCE = 200 #This byte will capture long strings of bytes in compact form. Example [12][12][12]...[12] => [200][#][12] where # is number of 12 tiles

OBJECT_START_BYTE = 201
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

def insert_position(position, sourceList, insertionList):
    return sourceList[:position] + insertionList + sourceList[position:]

root = tk.Tk()
root.withdraw()

#file_path = filedialog.askopenfilename()
#print(file_path)

def convertTMX(file_path):
    document = parse(file_path)
    entities = {}
    mapHeaderWidth = int(document.getElementsByTagName("map")[0].attributes.getNamedItem("width").nodeValue)
    #print(mapHeaderWidth)
    mapHeaderHeight = int(document.getElementsByTagName("map")[0].attributes.getNamedItem("height").nodeValue)
    mode = "SOKO_UNDEFINED"
    
    #get mode
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
    firstgid = 0 #offset from the tilesheet
    ## todo: how od we know what this thing is called? we just want local id's always? but we don't know which tilesheet the user drew with.
    # not sure how the tmx format links tilesets to the layer/objectgroups. 
    tilesets = document.getElementsByTagName("tileset")
    for tileset in tilesets:
        if(tileset.getAttribute("name") == "tilesheet"):
            firstgid = int((tileset.getAttribute("firstgid")))
            break
    dataText = document.getElementsByTagName("data")[0].firstChild.nodeValue
    scrub = "".join(dataText.split()) #Remove all residual whitespace in data block
    scrub2 = [((int(i)-firstgid)) for i in scrub.split(",")] #Convert all tileIDs to int.
    #print(scrub)
    output = []
    for i in range(len(scrub2)):
        x = (i-1)%mapHeaderWidth
        y = ((i-1)//mapHeaderWidth)+1 #todo: figure out why this is +1 ????
        key = str(x)+","+str(y)
        print("playing with "+key)
        if(key in entities):
            for b in entities[key]:
                output.append(b)
        output.append(getTile(scrub2[i]))


    #scrub3 = list([mapHeaderWidth,mapHeaderHeight]) + scrub2
    scrub3 = insert_position(0,output,list([mapHeaderWidth,mapHeaderHeight,modeInt]))
    print(scrub3)

    rawBytes = bytearray(scrub3)
    rawBytesImmut = bytes(rawBytes)
    return rawBytesImmut
    # outfile_path = "".join([file_path.split(".")[0],".bin"])
    # print(outfile_path)
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

def getTile(i):
    #0 is wall? i guess?
    return i