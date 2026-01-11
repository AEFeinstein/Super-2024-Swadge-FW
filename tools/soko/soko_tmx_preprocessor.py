import sys
import os
from tmx_to_binary import convertTMX
count = 0
total = 0
raw_total = 0
comp_total = 0


def main():
    convertAndSave(sys.argv[1], sys.argv[2])


def convertDir(dir,output):
    global count,total, raw_total, comp_total
    # todo: check file modification dates.
    # lol no
    for file in os.scandir(dir):
        if os.path.isfile(file):
            name, ext = os.path.splitext(file)
            if ext == '.tmx':
                lastMod = os.path.getmtime(file)
                fname = getNameFromPath(file)
                out_file = output+fname+".bin"
                if(os.path.isfile(out_file)):
                    lastOutMod = os.path.getmtime(out_file)
                    if(lastMod < lastOutMod):
                        #print("skipping "+fname)
                        total+=1
                        continue
                convertAndSave(file.path,output)
                count+=1
                total+=1
        elif os.path.isdir(file):
            convertDir(file,output)

def convertAndSave(filepath,output):
    global raw_total, comp_total
    rawbytes, r,c = convertTMX(filepath)
    raw_total += r
    comp_total += c
    #fname = getNameFromPath(filepath)
    #outfile_file = output+fname+".bin"
    with open(output,"wb") as binary_file:
            binary_file.write(rawbytes)

def getNameFromPath(p):
    base = os.path.basename(p)
    fp = base.split(".")
    fname = fp[len(fp)-2]
    return fname

main()