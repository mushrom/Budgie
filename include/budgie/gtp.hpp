#pragma once

#include <budgie/game.hpp>
#include <budgie/mcts.hpp>
#include <budgie/args_parser.hpp>
#include <map>
#include <memory>

namespace mcts_thing {

class gtp_client {
	public:
		void repl(args_parser::option_map& options);
		void clear_board(void);
		void set_board_size(void);

		int komi = 5;
		int boardsize = 9;
		bool passed = false;
};

// XXX: defining this here so we can reuse it
std::vector<std::string> split_string(std::string& s);

}
