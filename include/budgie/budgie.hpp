#pragma once

#include <budgie/mcts.hpp>
#include <budgie/args_parser.hpp>
#include <budgie/thread_pool.hpp>

// TODO: change namespace to 'budgie'
namespace bdg {

static args_parser::default_map budgie_options = {
	// general game settings
	{"boardsize",
		{"9", "Initial board size, currently only square boards are supported.",
			{"<any integer greater than 0>"}}},
	{"komi",
		{"5.5", "Komi given to the white player",
			{"<any real number>"}}},

	// general interface/behaviour settings
	{"pass",
		{"true", "Determine whether to detect when to pass. Good for humans, bad for bot matches.",
			{"false", "true"}}},
	{"ogs_output",
		{"false", "Outputs statistics parsed by gtp2ogs for malkovich chat.",
			{"false", "true"}}},

	// AI settings
	{"mode",
		{"gtp", "Interface mode to use",
			{"gtp", "distributed-gtp", "distributed-worker"}}},
	{"playouts",
		{"10000", "Number of playouts to use per move",
			{"<any postive integer>"}}},

	{"joseki_db",
		{"", "List of filenames containing joseki hash tables",
			{"<comma-delimited list of filenames>"}}},
	{"tree_policy",
		{"uct-rave", "Policy to use for tree traversal",
			{"uct-rave", "uct", "mcts"}}},
	{"playout_policy",
		{"capture_enemy_ataris,save_own_ataris,attack_enemy_groups,adjacent-3x3,random",
			"Comma-separated list of strategies to use when playing out games",
			{"random", "adjacent-3x3", "adjacent-5x5", "local_weighted", "save_own_ataris",
				"capture_enemy_ataris", "attack_enemy_groups"}}},
	{"patterns",
		{"patterns.txt", "Local pattern database file (gnugo format)",
			{"patterns.txt"}}},
	{"parameters",
		{"", "comma-separated list of key=value pairs to control AI behaviour.",
			{"key=value,..."}}},

	// distributed mode configuration
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
		bool make_move(move m);
		bool make_move(const coordinate& c);
		move genmove(void);
		void reset(void);
		void set_player(point::color p);

		args_parser::option_map options;

		float komi;
		int boardsize;
		int playouts;
		bool passed = false;
		bool allowPassing = true;
		bool ogsChat = false;

		board game;
		std::unique_ptr<mcts> tree;
		unsigned woncount = 0;
		thread_pool pool;

	private:
		std::unique_ptr<mcts> init_mcts(args_parser::option_map& options);
};

// namespace budgie
};
