#pragma once

#include <mcts-gb/game.hpp>
#include <mcts-gb/mcts.hpp>
#include <map>

namespace mcts_thing {

class gtp_client {
	public:
		gtp_client() {
			game = board(9);
			current_move = search_tree.root;
		}

		void repl(std::map<std::string, std::string> options);
		void clear_board(void);
		void set_board_size(void);

		int komi = 5;
		int boardsize = 9;

		board game;
		mcts search_tree;
		mcts_node *current_move;
};

}
