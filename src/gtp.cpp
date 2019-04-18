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

coordinate string_to_coord(std::string& str) {
	std::string asdf = "abcdefghjklmnopqrstuvwxyz";

	unsigned x = asdf.find(std::tolower(str[0])) + 1;
	unsigned y = atoi(str.substr(1).c_str());

	return {x, y};
}

void gtp_client::repl(void) {
	std::string s;

	while (std::getline(std::cin, s)) {
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
			game.reset(boardsize, komi);

			/*
			current_move = search_tree.root;
			game.komi = komi;
			game.moves = 0;
			*/

			std::cout << "=\n\n";
		}

		else if (args[0] == "play") {
			point::color player = (args[1] == "B" || args[1] == "b" || args[1] == "black")
			                          ? point::color::Black
			                          : point::color::White;

			/*
			unsigned x = asdf.find(std::tolower(args[2][0])) + 1;
			unsigned y = atoi(args[2].substr(1).c_str());
			coordinate coord = {x, y};
			*/

			coordinate coord = string_to_coord(args[2]);

			//printf("(%u, %u)\n", x, y);

			// TODO: same todo as below
			game.current_player = player;
			game.make_move(coord);

			std::cout << "=\n\n";
		}

		else if (args[0] == "set_free_handicap") {
			point::color player = game.current_player;

			for (auto it = args.begin() + 1; it != args.end(); it++) {
				coordinate coord = string_to_coord(*it);

				// TODO: again same todo
				game.current_player = player;
				game.make_move(coord);
			}

			std::cout << "=\n\n";
		}

		else if (args[0] == "genmove") {
			// TODO: should have function to handle this, rather than mutating the game state
			//       from here
			// TODO: should have function to map words to player colors
			game.current_player = (args[1] == "B" || args[1] == "b" || args[1] == "black")
			                          ? point::color::Black
			                          : point::color::White;

			current_move->explore(&game);
			std::cerr << "# initial search playouts (completed games): "
			          << current_move->terminal_nodes() << std::endl;

			//current_move->exploit(&game, 6, 2);
			//current_move->exploit(&game, 6, 2);
			//current_move->exploit(&game, 6, 2);
			//current_move->exploit(&game, 2, 4);
			//current_move->exploit(&game, 8, 2);
			/*
			current_move->exploit(&game, 4, 4);
			*/

			coordinate coord = current_move->best_move();
			std::cerr << "# exploit search playouts (completed games): "
			          << current_move->terminal_nodes() << std::endl;
			std::cerr << "# estimated win rate: "
			          << current_move->leaves[coord].win_rate() << std::endl;

			if (current_move->leaves[coord].win_rate() < 0.15) {
				std::cout << "= resign\n\n";
				continue;
			}

			/*
			if (current_move->leaves[coord].win_rate() > 0.99) {
				std::cout << "= pass\n\n";
				continue;
			}
			*/

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
			game.make_move(coord);

#ifdef GTP2OGS_WORKAROUND
			// XXX: gtp2ogs is a little buggy, doesn't properly kill the bot, so need to
			//      exit here after every move to avoid bot instances building up...
			return;
#endif
		}

		else if (s == "showboard") {
			game.print();
		}

		else if (s == "quit") {
			std::cout << "=\n\n";
			return;
		}

		else {
			//std::cout << "? unknown command\n\n";
			std::cerr << "# unknown command: " << s << std::endl;
			std::cout << "= \n\n";
		}
	}
}

// mcts_thing
}
