#!/bin/bash

find ../../ -iname cnfs_image* -delete
find ../../ -iname table.bin -delete
find ../../ -iname pinball.raw -delete
make -C ../cnfs/ clean
python svg-to-pinball.py
cp table.bin ../../assets/pinball.raw
