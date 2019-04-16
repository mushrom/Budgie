#include <mcts-gb/gtp.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

namespace mcts_thing {

std::vector<std::string> split_string(std::string& s) {
	std::istringstream iss(s);
	std::vector<std::string> ret {
		std::istream_iterator<std::string>(iss),
		std::istream_iterator<std::string>{}
	};

	return ret;
}

void gtp_client::repl(void) {
	std::string s;

	while (std::getline(std::cin, s)) {
		std::cerr << "DEBUG: " << s << std::endl << std::flush;

		if (s == "" || s[0] == '#') {
			continue;
		}

		auto args = split_string(s);

		if (s == "name") {
			std::cout << "= BudgieBot\n\n";
		}

		else if (s == "version") {
			std::cout << "= 0.0.1\n\n";
		}

		else if (s == "protocol_version") {
			std::cout << "= 2\n\n";
		}

		else if (s == "list_commands") {
			std::cout << "= name\nversion\nlist_commands\nboardsize\ngenmove\n"
					  << "clear_board\nkomi\nplay\nprotocol_version\nquit\n"
			          << "showboard\n\n";
		}

		else if (args[0] == "komi") {
			komi = atoi(args[1].c_str());
			game.komi = komi;

			std::cout << "=\n\n";
		}

		else if (args[0] == "boardsize") {
			// needs to be followed by clear_board to take effect, dunno if this is
			// spec but it makes the implementation less messy

			boardsize = atoi(args[1].c_str());
			std::cout << "=\n\n";
		}

		else if (args[0] == "clear_board") {
			board temp(boardsize);
			game.set(temp);

			current_move = search_tree.root;
			game.komi = komi;
			game.moves = 0;

			std::cout << "=\n\n";
		}

		else if (args[0] == "play") {
			point::color player = (args[1] == "B" || args[1] == "black")
			                          ? point::color::Black
			                          : point::color::White;

			std::string asdf = "abcdefghjklmnopqrstuvwxyz";

			unsigned x = asdf.find(std::tolower(args[2][0])) + 1;
			unsigned y = atoi(args[2].substr(1).c_str());

			//printf("(%u, %u)\n", x, y);

			coordinate coord = {x, y};
			game.make_move(coord);

			std::cout << "=\n\n";
		}

		else if (args[0] == "genmove") {
			/*
			point::color player = (args[1] == "B")
			                          ? point::color::Black
			                          : point::color::White;
									  */
			current_move->explore(&game);
			/*
			std::cout << "# total playouts (completed games): "
			          << current_move->terminal_nodes() << std::endl;
					  */

			current_move->exploit(&game, 8, 2);
			current_move->exploit(&game, 4, 4);

			coordinate coord = current_move->best_move();

			if (current_move->leaves[coord].win_rate() < 0.1) {
				std::cout << "= resign\n\n";
				continue;
			}

			if (current_move->leaves[coord].win_rate() > 0.9) {
				std::cout << "= pass\n\n";
				continue;
			}

			if (!game.is_valid_coordinate(coord)
			    || game.is_suicide(coord, game.current_player))
			{
				std::cout << "= pass\n\n";
				continue;
			}

			std::string asdf = "ABCDEFGHJKLMNOPQRSTUVWXYZ";
			std::cout << "= " << asdf[coord.first - 1] << std::to_string(coord.second) << "\n";

			std::cout << "\n\n";

			search_tree.reset();
			current_move = search_tree.root;
			//current_move = &current_move->leaves[coord];
			game.make_move(coord);
			//game.print();
		}

		else if (s == "showboard") {
			game.print();
		}

		else if (s == "quit") {
			return;
		}

		else {
			//std::cout << "? unknown command\n\n";
			std::cout << "= \n\n";
		}

		std::cerr << "DEBUG: Done" << std::endl << std::flush;
		std::cout << std::flush;
	}
}

// mcts_thing
}
