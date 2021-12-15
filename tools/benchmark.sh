#!/usr/bin/env bash

for playouts in 1000 2000 3000 4000 5000 7500 10000; do
	echo; echo; echo "$playouts playouts:"
	time echo -e "genmove b\nquit\n" | ./bin/thing --playouts $playouts 2 $@ &> /dev/null
done
