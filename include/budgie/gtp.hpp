#pragma once

#include <budgie/game.hpp>
#include <budgie/mcts.hpp>
#include <budgie/args_parser.hpp>
#include <map>
#include <memory>

namespace mcts_thing {

class gtp_client {
	public:
		gtp_client() {
			game = board(9);
		}

		void repl(args_parser::option_map& options);
		void clear_board(void);
		void set_board_size(void);

		int komi = 5;
		int boardsize = 9;
		bool passed = false;

		board game;
		std::unique_ptr<mcts> search_tree;
};

}
