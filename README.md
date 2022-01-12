Budgie Go Bot [![CI](https://github.com/mushrom/Budgie/actions/workflows/cmake.yml/badge.svg)](https://github.com/mushrom/Budgie/actions/workflows/cmake.yml)
=============

# What's it?
This is an MCTS-based go-playing AI. For each move, it plays a large number
of random games, and chooses the move that seems the most promising.

# What can it do?
Right now it's about intermediate strength on 9x9 boards, holding an average rank
of 10k to 8k on OGS, and an elo rating of 1775 on CGOS, both with 5000 playouts.

Currently supports:
  - Very fast multithreaded, lock-free tree search and playouts.
  - Partial support for 3x3 pattern specification files from GnuGo, in both tree search and playouts
  - Joseki databases
  - A GTP interface

Coming soon:
  - Distributed computation (broken at the time of writing, but there)
  - Neural networks
  - Analysis tools, like the lz-analyze commands, score estimation, etc
  - Generalizing the bot for use with other games

# How to use?
#### Cloning the repo and libraries:
    git clone --recursive 'https://github.com/mushrom/Budgie'
    cd Budgie

#### Building:
    cmake -B build -DCMAKE_BUILD_TYPE=Release # or Debug
    cd build; make

#### Using:
Currently GTP is the only supported way to play against the bot. You'll need
a client to interact with it, such as Sabaki or GnuGo.

Budgie also plays on online-go.com, if you just want to have a quick match
against it.

# Why though?
To learn about game-playing AIs, and to provide a base to further learn about
neural networks as used in eg. AlphaGo. Currently I want to work on making it
as strong as I can with older techniques (eg. UCT-RAVE from Mogo, Pachi),
just because they're interesting.
