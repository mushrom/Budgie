#!/bin/sh

USERNAME=Budgie
APIKEY="$(cat apikey$MODE.txt)"

gtp2ogs --username $USERNAME --apikey $APIKEY $MODE --boardsize 9 --debug \
	--greeting "Hi, have fun! If you run into any problems, such as the bot being glitchy or timing out, please contact my admin." \
	--farewell "Thanks for the game!" \
	-- ./thing
