#pragma once

#include <mcts-gb/game.hpp>
#include <mcts-gb/mcts.hpp>
#include <map>
#include <memory>

namespace mcts_thing {

class gtp_client {
	public:
		gtp_client() {
			game = board(9);
		}

		void repl(std::map<std::string, std::string> options);
		void clear_board(void);
		void set_board_size(void);

		int komi = 5;
		int boardsize = 9;
		bool passed = false;

		board game;
		std::unique_ptr<mcts> search_tree;
};

}
