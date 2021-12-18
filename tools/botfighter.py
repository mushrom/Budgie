#!/usr/bin/env python3

from subprocess import Popen, PIPE
from random import shuffle
import sys
import os
import fcntl
import datetime
import time

gnugoArgs = ["gnugo", "--mode", "gtp", "--chinese-rules", "--play-out-aftermath"] 

"""
bots = [
    ("Budgie 5000p",            ["./bin/thing", "--playouts", "5000"]),
    ("Budgie 5000p lowUCT",     ["./bin/thing", "--playouts", "5000", "--uct_weight", "0.01"]),
    ("Budgie 1000p",            ["./bin/thing", "--playouts", "1000"]),
    ("Budgie 7500p",            ["./bin/thing", "--playouts", "7500"]),
    ("Budgie 10000p",           ["./bin/thing", "--playouts", "10000"]),
    ("Budgie 10000p lowUCT",    ["./bin/thing", "--playouts", "10000", "--uct_weight", "0.01"]),
    ("Budgie UCT 2500p",        ["./bin/thing", "--tree_policy", "uct", "--playouts", "5000"]),
    ("Budgie UCT 5000p",        ["./bin/thing", "--tree_policy", "uct", "--playouts", "5000"]),
    ("Budgie UCT 10000p",       ["./bin/thing", "--tree_policy", "uct", "--playouts", "10000"]),
    ("Budgie joseki 5000p",     ["./bin/thing", "--joseki_db", "joseki9.dat,joseki13.dat,joseki19.dat", "--playouts", "5000"]),
    ("Budgie joseki 10000p",    ["./bin/thing", "--joseki_db", "joseki9.dat,joseki13.dat,joseki19.dat", "--playouts", "10000"]),
    ("Budgie joseki 5000p LU",  ["./bin/thing", "--joseki_db", "joseki9.dat,joseki13.dat,joseki19.dat", "--playouts", "5000", "--uct_weight", "0.01"]),
    ("Budgie joseki 10000p LU", ["./bin/thing", "--joseki_db", "joseki9.dat,joseki13.dat,joseki19.dat", "--playouts", "10000", "--uct_weight", "0.01"]),
    ("GnuGo level 10", gnugoArgs + ["--level", "10",]),
    ("GnuGo level 8",  gnugoArgs + ["--level", "8",]),
    ("GnuGo level 5",  gnugoArgs + ["--level", "5",]),
    ("GnuGo level 3",  gnugoArgs + ["--level", "3",]),
    ("GnuGo level 0",  gnugoArgs + ["--level", "0",]),
]
"""

"""
bots = [
    ("Budgie 5000p",            ["./bin/thing", "--playouts", "5000"]),
    ("Budgie 5000p lowUCT",     ["./bin/thing", "--playouts", "5000", "--uct_weight", "0.01"]),
    ("Budgie 5000p adj3x3",            ["./bin/thing", "--playouts", "5000", "--playout_policy", "capture_enemy_ataris save_own_ataris attack_enemy_groups adjacent-3x3 random"]),
    ("Budgie 5000p adj3x3 lowUCT",     ["./bin/thing", "--playouts", "5000", "--uct_weight", "0.01", "--playout_policy", "capture_enemy_ataris save_own_ataris attack_enemy_groups adjacent-3x3 random"]),
]
"""

bots = [
    ("Budgie 5000p (2lib)",            ["./bin/thing-2lib", "--playouts", "5000"]),
    ("Budgie 5000p (2lib) lowUCT",     ["./bin/thing-2lib", "--playouts", "5000", "--uct_weight", "0.01"]),
    ("Budgie 5000p (2lib) adj3x3",            ["./bin/thing-2lib", "--playouts", "5000", "--playout_policy", "capture_enemy_ataris save_own_ataris attack_enemy_groups adjacent-3x3 random"]),
    ("Budgie 5000p (2lib) adj3x3 lowUCT",     ["./bin/thing-2lib", "--playouts", "5000", "--uct_weight", "0.01", "--playout_policy", "capture_enemy_ataris save_own_ataris attack_enemy_groups adjacent-3x3 random"]),
    ("Budgie 5000p (badadj)",            ["./bin/thing-badadj", "--playouts", "5000"]),
    ("Budgie 5000p (badadj) lowUCT",     ["./bin/thing-badadj", "--playouts", "5000", "--uct_weight", "0.01"]),
    ("Budgie 5000p (badadj) adj3x3",            ["./bin/thing-badadj", "--playouts", "5000", "--playout_policy", "capture_enemy_ataris save_own_ataris attack_enemy_groups adjacent-3x3 random"]),
    ("Budgie 5000p (badadj) adj3x3 lowUCT",     ["./bin/thing-badadj", "--playouts", "5000", "--uct_weight", "0.01", "--playout_policy", "capture_enemy_ataris save_own_ataris attack_enemy_groups adjacent-3x3 random"]),
    ("Budgie 5000p",            ["./bin/thing", "--playouts", "5000"]),
    ("Budgie 5000p lowUCT",     ["./bin/thing", "--playouts", "5000", "--uct_weight", "0.01"]),
    ("Budgie 5000p adj3x3",            ["./bin/thing", "--playouts", "5000", "--playout_policy", "capture_enemy_ataris save_own_ataris attack_enemy_groups adjacent-3x3 random"]),
    ("Budgie 5000p adj3x3 lowUCT",     ["./bin/thing", "--playouts", "5000", "--uct_weight", "0.01", "--playout_policy", "capture_enemy_ataris save_own_ataris attack_enemy_groups adjacent-3x3 random"]),
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
        time.sleep(0.5);

        if moveA == "resign":
            return "W+Res"

        moveB = sendCommand(botB,  "genmove w")[0][2:]
        sendCommand(botA, f"play w {moveB}")
        moves.append(f"play w {moveB}")
        printTick()
        time.sleep(0.5);
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
    fname = "/tmp/data.csv"

    if len(sys.argv) > 1:
        fname = sys.argv[1]

    print(f"Writing data to {fname}")
    thing = open(fname, "a")

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
