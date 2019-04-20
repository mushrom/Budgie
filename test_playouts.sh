#!/bin/sh

for i in {0..40}; do
	echo genmove b; echo showboard
	echo genmove w; echo showboard
done | ./thing $@
