#!/usr/bin/env python3

import sys

def winRatio(columns):
    wins = {}
    games = {}

    for datas in columns:
        for i in range(1, 3):
            if datas[i] not in wins:
                wins.update({datas[i] : 0})
            if datas[i] not in games:
                games.update({datas[i] : 0})

        if datas[3][0] == "B":
            wins[datas[1]] += 1
        else:
            wins[datas[2]] += 1

        games[datas[1]] += 1
        games[datas[2]] += 1

    return wins, games

"""
def approxElo(Rold, Rother, Won, K=32, C=200):
    Lost = (not Won)+0

    return Rold + K/2 * (Won - (Lost + .5*(Rother-Rold)/C))
"""

def gelo(Rold, Rother, Won, K=32, C=400):
    p1 = 1.0 / (1.0 + pow(10, ((Rother - Rold)/C)))

    return Rold + K*(Won - p1)

def calcElo(columns):
    elos = {}

    for datas in columns:
        for i in range(1, 3):
            if datas[i] not in elos:
                # default elo of 1000
                elos.update({datas[i] : 1000})

        winner = datas[3][0]
        wonBlack = (winner == "B")+0
        wonWhite = (winner == "W")+0

        ea = elos[datas[1]]
        eb = elos[datas[2]]

        elos[datas[1]] = gelo(ea, eb, wonBlack);
        elos[datas[2]] = gelo(eb, ea, wonWhite);

    return elos

def sortByColumn(n):
    def cmp(a, b):
        return a[n] < b[n]
    return cmp

from operator import itemgetter

if __name__ == "__main__":
    if (len(sys.argv) < 2):
        print("Usage: gen_ranks.py data.csv")
        exit()

    columns = []
    for csv in sys.argv[1:]:
        datafile = open(csv, "r")
        columns += [line.split(",") for line in datafile.readlines()]

    print(" "*30 + "===== win/lose ratio:")
    wins, games = winRatio(columns)
    datas = sorted([(k, v, games[k]-v, v/games[k]) for k,v in wins.items()], key=itemgetter(3), reverse=True)

    for stuff in datas:
        print("%40s:\t%4d wins\t%4d loses\t%g" % stuff)

    print()
    print(" "*30 + "===== Elo ratings:")
    stuff = calcElo(columns).items()
    elos = sorted([(name, elo) for name, elo in stuff], key=itemgetter(1), reverse=True)
    for rank in elos:
        print("%40s:\t%g elo" % rank)
