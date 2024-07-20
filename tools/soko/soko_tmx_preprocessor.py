import sys
import os
from tmx_to_binary import convertTMX
count = 0
total = 0
raw_total = 0
comp_total = 0
def main():
    print("Starting soko tmx conversion")

    # check if output is real directory and create it if it does not exist.
    output = sys.argv[2]
    if not os.path.exists(output):
        os.makedirs(output)

    # todo: automatically check and move SK_LEVEL_LIST.txt, it doesn't update automatically.

    # todo: ensure output ends in a trailing slash.
    convertDir(sys.argv[1],output)
    print("Completed soko tmx converstion. "+str(count)+ "/"+str(total)+" tmx files converted. "+str(comp_total)+"/"+str(raw_total)+" - "+str(raw_total-comp_total)+" (of converted) bytes saved with compression.")


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
    fname = getNameFromPath(filepath)
    outfile_file = output+fname+".bin"
    with open(outfile_file,"wb") as binary_file:
            binary_file.write(rawbytes)

def getNameFromPath(p):
    base = os.path.basename(p)
    fp = base.split(".")
    fname = fp[len(fp)-2]
    return fname

main()