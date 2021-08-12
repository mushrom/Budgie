#!/bin/sh

if [ -z $BOARDSIZE ]; then BOARDSIZE=9; fi
if [ -z $KOMI ]; then KOMI=4.5; fi
if [ -z $MOVES ]; then MOVES=$(( BOARDSIZE * BOARDSIZE )); fi

echo playing $MOVES moves on a "$BOARDSIZE x $BOARDSIZE" board with $KOMI komi

seq 0 $MOVES | while read i; do
	if [ $i -eq 0 ]; then
		echo boardsize $BOARDSIZE
		echo komi $KOMI
		echo clear_board
	fi;

	echo genmove b; echo showboard
	echo genmove w; echo showboard
done | ./bin/thing "$@"
