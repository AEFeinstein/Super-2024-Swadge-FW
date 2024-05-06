import sys
import os
from tmx_to_binary import convertTMX

def main():
    print("Starting soko tmx conversion")

    # check if output is real directory and create it if it does not exist.
    output = sys.argv[2]
    if not os.path.exists(output):
        os.makedirs(output)

  
    
    # todo: ensure output ends in a trailing slash.
    convertDir(sys.argv[1],output)
    print("completed soko tmx converstion")


def convertDir(dir,output):
    # todo: check file modification dates.
    # lol no
    for file in os.scandir(dir):
       # print("converting "+file)
        if os.path.isfile(file):
            name, ext = os.path.splitext(file)
            if ext == '.tmx':
                convertAndSave(file.path,output)
        elif os.path.isdir(file):
            convertDir(file,output)

def convertAndSave(filepath,output):
    rawbytes = convertTMX(filepath)
    fname = getNameFromPath(filepath)
    outfile_file = output+fname+".bin"
    print(filepath +" to "+outfile_file)
    with open(outfile_file,"wb") as binary_file:
            binary_file.write(rawbytes)

def getNameFromPath(p):
    base = os.path.basename(p)
    fp = base.split(".")
    fname = fp[len(fp)-2]
    return fname

main()