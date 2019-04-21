#!/bin/sh

for i in {0..40}; do
	if [[ $i == 0 ]]; then
		echo boardsize 6
		echo komi 2.5
		echo clear_board
	fi;

	echo genmove b; echo showboard
	echo genmove w; echo showboard
done | ./thing $@
