Budgie Go Bot
=============

# What's it?
A basic go-playing AI, based on techniques from Mogo and Pachi.

# What can it do?
Right now it's pretty weak. It plays on OGS with about 7.5k
playouts, and is ranked at 15k as of now. It also has (mostly working)
support for distributed processing, which is pretty neat imo.

# How to use?
#### Cloning the repo and libraries:
    git clone --recursive 'https://github.com/mushrom/Budgie'
	cd Budgie

#### Building: (on linux)
    # To build just the bot:
	make

	# To build everything, including SDL debugging UI:
	make all

	# these both place binaries in ./bin

#### Using:
Currently GTP is the only supported way to play against the bot. You'll need
a client to interact with it, such as Sabaki or GnuGo.

Budgie also plays on online-go.com, if you just want to have a quick match
against it.

# Why though?
To learn about game-playing AIs, and to provide a base to further learn about
neural networks as used in eg. AlphaGo.

# Future plans
Neural networks, experimenting with different machine learning techniques,
adapting to other games, etc.
