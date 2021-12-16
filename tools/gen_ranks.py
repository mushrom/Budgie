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

if __name__ == "__main__":
    if (len(sys.argv) < 2):
        print("Usage: gen_ranks.py data.csv")
        exit()

    datafile = open(sys.argv[1], "r")
    columns = [line.split(",") for line in datafile.readlines()]

    wins, games = winRatio(columns)

    for k,v in wins.items():
        print("%20s:\t%4d wins\t%4d loses\t%g" % (k, v, games[k] - v, v/games[k]))
