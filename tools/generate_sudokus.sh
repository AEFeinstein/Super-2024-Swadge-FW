#!/bin/bash

PUZZLE_COUNT=50

# Generate 10 simple, 15 easy, 15 intermediate, and 10 expert

SIMPLE_COUNT=10
EASY_COUNT=15
INTERMEDIATE_COUNT=15
EXPERT_COUNT=10

EASY_THRESH=$SIMPLE_COUNT
INTERMEDIATE_THRESH=$(($EASY_THRESH+$EASY_COUNT))
EXPERT_THRESH=$(($INTERMEDIATE_THRESH+$INTERMEDIATE_COUNT))

COUNT=$SIMPLE_COUNT
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
		COUNT=$EASY_COUNT
	elif [ "${I}" -eq "${INTERMEDIATE_THRESH}" ]; then
		DIFF="intermediate"
		COUNT=$INTERMEDIATE_COUNT
	elif [ "${I}" -eq "${EXPERT_THRESH}" ]; then
		DIFF="expert"
		COUNT=$EXPERT_COUNT
	fi



	# APPARENTLY qqwing uses srand(time()), so if you generate two puzzles in the same second they're the same
	# So we have to use the --generate <n> option to do multiple ones, with this monstrosity
	PUZZLE=
	while REPLY=; read -r || [[ $REPLY ]]; do
		if [[ $REPLY ]]; then
			if [[ $PUZZLE ]]; then
				PUZZLE+=$'\n'"$REPLY"
			else
				PUZZLE+=$REPLY
			fi
		else
			OUTPUT="$(printf './assets/sudoku/sudoku_puz_%03d.sdk' ${I})"
			cat <<EOF > $OUTPUT
#Adylwhich
#DA randomly-generated ${DIFF} puzzle
#B$(date -I)
#L${DIFF}
#Sqqwing
${PUZZLE}
EOF

			I=$(($I + 1))
			PUZZLE=
		fi
	done < <(qqwing --generate $COUNT --difficulty $DIFF | tee qqqwing_$I)
done
