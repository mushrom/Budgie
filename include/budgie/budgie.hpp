#pragma once

#include <budgie/mcts.hpp>
#include <budgie/args_parser.hpp>

// TODO: change namespace to 'budgie'
namespace mcts_thing {

static args_parser::default_map budgie_options = {
	{"boardsize",
		{"9", "Initial board size, currently only square boards are supported.",
			{"<any integer greater than 0>"}}},
	{"komi",
		{"5.5", "Komi given to the white player",
			{"<any real number>"}}},
	{"mode",
		{"gtp", "Interface mode to use",
			{"gtp", "distributed-gtp", "distributed-worker"}}},
	{"playouts",
		{"10000", "Number of playouts to use per move",
			{"<any unsigned integer>"}}},
	{"use_patterns",
		{"1", "Boolean to enable/disable the pattern database",
			{"0", "1"}}},
	{"tree_policy",
		{"uct-rave", "Policy to use for tree traversal",
			{"uct-rave", "uct", "mcts"}}},
	{"playout_policy",
		{"random", "Policy to use when playing out games",
			{"random", "local_weighted"}}},
	{"patterns",
		{"patterns.txt", "Local pattern database file (gnugo format)",
			{"patterns.txt"}}},
	{"server-host",
		{"tcp://localhost:5555", "Master server using the distributed mode",
			{"<ZMQ socket string>"}}},
	{"worker-threads",
		{"0", "Number of workers to start as a distributed client",
			{"0 for auto", "<any unsigned integer>"}}},
};

class budgie {
	public:
		class move {
			public:
				enum types {
					Pass,
					Resign,
					Move,
				};

				move(types t = types::Move,
					 coordinate c = {0, 0},
					 point::color p = point::color::Empty)
				{
					type = t;
					coord = c;
					player = p;
				}

				// TODO: more generic move format so we can adapt this to games
				//       other than go
				types type;
				coordinate coord = {0, 0};
				point::color player;
			};

		budgie(args_parser::option_map& opts);

		// these functions wrap the board and mcts classes to keep everything
		// in sync
		void make_move(move m);
		move genmove(void);
		void reset(void);
		void set_player(point::color p);

		args_parser::option_map options;

		// TODO: maybe some accessor functions for game parameters
		int komi;
		int boardsize;
		int playouts;
		bool passed = false;

		board game;
		std::unique_ptr<mcts> tree;

	private:
		std::unique_ptr<mcts> init_mcts(args_parser::option_map& options);
};

// namespace budgie
};