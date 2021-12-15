#!/usr/bin/env python3

from subprocess import Popen, PIPE
from random import shuffle
import sys
import os
import fcntl
import datetime

gnugoArgs = ["gnugo", "--mode", "gtp", "--chinese-rules", "--play-out-aftermath"] 

bots = [
    ("Budgie 5000p", ["./bin/thing", "--playouts", "5000"]),
    ("Budgie 1000p", ["./bin/thing", "--playouts", "1000"]),
    ("Budgie 7500p", ["./bin/thing", "--playouts", "7500"]),
    ("GnuGo level 10", gnugoArgs + ["--level", "10",]),
    ("GnuGo level 5",  gnugoArgs + ["--level", "5",]),
    ("GnuGo level 3",  gnugoArgs + ["--level", "3",]),
    ("GnuGo level 0",  gnugoArgs + ["--level", "0",]),
]

scorer = ["./bin/thing"]

def startBot(args):
    return Popen(args, shell=False, stdin=PIPE, stdout=PIPE,
                 close_fds=True, bufsize=1, text=True)

def sendCommand(bot, command):
    bot.stdin.write(command + "\n")
    bot.stdin.flush()

    result = []
    while True:
        line = bot.stdout.readline()[:-1]
        if len(line) == 0:
            break;
        result += [line]

    return result

def showboard(bot):
    board = sendCommand(bot, "showboard")
    [print(ln) for ln in board]

def printTick():
    sys.stdout.write(".")
    sys.stdout.flush()

def playGame(botA, botB):
    moves = []
    result = "U+0"

    # A is black, B is white
    while True:
        moveA = sendCommand(botA,  "genmove b")[0][2:]
        sendCommand(botB, f"play b {moveA}")
        moves.append(f"play b {moveA}")
        #showboard(botA);
        printTick()

        if moveA == "resign":
            return "W+Res"

        moveB = sendCommand(botB,  "genmove w")[0][2:]
        sendCommand(botA, f"play w {moveB}")
        moves.append(f"play w {moveB}")
        printTick()
        #showboard(botB);
        if moveB == "resign":
            return "B+Res"

        if moveA.lower() == "pass" and moveB.lower() == "pass":
            # ask a bot for the final score
            return scoreGame(moves)

    #print("Result: %s" % result)
    return result

def scoreGame(moves):
    scoreBot = startBot(scorer)
    initBoard(scoreBot)

    for move in moves:
        sendCommand(scoreBot, move)

    result = sendCommand(scoreBot, "final_score")[0][2:]
    scoreBot.terminate()
    return result

def initBoard(bot):
    sendCommand(bot, "boardsize 9")
    sendCommand(bot, "komi 4.5")
    sendCommand(bot, "clear_board")

if __name__ == "__main__":
    thing = open("/tmp/data.csv", "a")

    while True:
        scram = bots.copy()
        shuffle(scram)

        while len(scram) > 1:
            nameA, argsA = scram.pop()
            nameB, argsB = scram.pop()

            botA = startBot(argsA)
            botB = startBot(argsB)

            initBoard(botA)
            initBoard(botB)

            print("==== New round: %s vs. %s, 9x9" % (nameA, nameB))
            result = playGame(botA, botB)
            showboard(botA)
            print("==== Round complete: %s vs. %s, 9x9: %s" % (nameA, nameB, result))

            thing.write("%s,%s,%s,%s\n" % (str(datetime.datetime.now()), nameA, nameB, result))
            thing.flush()

            # push second item popped, becomes new botA next round
            scram.append((nameB, argsB))
            botA.terminate()
            botB.terminate()
