#!/bin/bash

PUZZLE_COUNT=50

# Generate 10 simple, 15 easy, 15 intermediate, and 10 expert
EASY_THRESH=10
INTERMEDIATE_THRESH=25
EXPERT_THRESH=40

DIFF="simple"

qqwing --version >/dev/null
if [ "$?" -ne "0" ]; then
	echo "qqwing is not installed! exiting" >2
	exit 1
fi


I=0
while [ "${I}" -lt "${PUZZLE_COUNT}" ]; do
	if [ "${I}" -eq "${EASY_THRESH}" ]; then
		DIFF="easy"
	elif [ "${I}" -eq "${INTERMEDIATE_THRESH}" ]; then
		DIFF="intermediate"
	elif [ "${I}" -eq "${EXPERT_THRESH}" ]; then
		DIFF="expert"
	fi

	OUTPUT="$(printf './assets/sudoku/sudoku_puz_%03d.sdk' ${I})"

	PUZZLE="$(qqwing --generate 1 --difficulty $DIFF)"
	cat <<EOF > $OUTPUT
#Adylwhich
#DA randomly-generated ${DIFF} puzzle
#B$(date -I)
#L${DIFF}
#Sqqwing
${PUZZLE}
EOF

	I=$(($I + 1))
done
