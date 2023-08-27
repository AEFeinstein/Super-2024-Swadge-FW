import tkinter as tk
from tkinter import filedialog
from xml.dom.minidom import parse,parseString

SIG_BYTE_SIMPLE_SEQUENCE = 200 #This byte will capture long strings of bytes in compact form. Example [12][12][12]...[12] => [200][#][12] where # is number of 12 tiles

def insert_position(position, sourceList, insertionList):
    return sourceList[:position] + insertionList + sourceList[position:]

root = tk.Tk()
root.withdraw()

file_path = filedialog.askopenfilename()

print(file_path)

document = parse(file_path)

mapHeaderWidth = int(document.getElementsByTagName("map")[0].attributes.getNamedItem("width").nodeValue)
print(mapHeaderWidth)
mapHeaderHeight = int(document.getElementsByTagName("map")[0].attributes.getNamedItem("height").nodeValue)
print(mapHeaderHeight)
dataText = document.getElementsByTagName("data")[0].firstChild.nodeValue
print(dataText)
scrub = "".join(dataText.split()) #Remove all residual whitespace in data block
scrub2 = [int(i) for i in scrub.split(",")] #Convert all tileIDs to int.
print(scrub)
print(scrub2)
#scrub3 = list([mapHeaderWidth,mapHeaderHeight]) + scrub2
scrub3 = insert_position(0,scrub2,list([mapHeaderWidth,mapHeaderHeight]))
print(scrub3)
rawBytes = bytearray(scrub3)
rawBytesImmut = bytes(rawBytes)
outfile_path = "".join([file_path.split(".")[0],".bin"])
print(outfile_path)
with open(outfile_path,"wb") as binary_file:
    binary_file.write(rawBytesImmut)


