import re
import sys
import random

def shuffle(string):
    split = re.split(',', string)
    random.shuffle(split)
    outString = ""
    for i in split:
        if i != "":
            outString += "\"" + i + "\","
    return outString

if __name__ == "__main__":
    string = ""
    for i in range(1, len(sys.argv)):
        string += sys.argv[i]
    print(shuffle(string))