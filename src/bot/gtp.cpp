#include <budgie/gtp.hpp>
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

std::string coord_string(const coordinate& coord) {
	std::string asdf = "ABCDEFGHJKLMNOPQRSTUVWXYZ";
	std::string ret = asdf[coord.first - 1] + std::to_string(coord.second);

	return ret;
}

std::string color_to_string(point::color color) {
	return (color == point::color::Black)? "black" : "white";
}

point::color string_to_color(std::string& str) {
	if (str == "B" || str == "b" || str == "black") {
		return point::color::Black;
	}

	return point::color::White;
}

void gtp_client::repl(args_parser::option_map& options) {
	// TODO: we should have an AI instance class that handles
	//       all of the board/mcts interaction
	unsigned playouts = stoi(options["playouts"]);
	pattern_dbptr db = pattern_dbptr(new pattern_db(options["patterns"]));

	// only have this tree policy right now
	tree_policy *tree_pol = new uct_rave_tree_policy(db);
	playout_policy *playout_pol = (options["playout_policy"] == "local_weighted")
		? (playout_policy *)(new local_weighted_playout(db))
		: (playout_policy *)(new random_playout(db));

	search_tree = std::unique_ptr<mcts>(new mcts(tree_pol, playout_pol));

	std::string s;

	while (std::getline(std::cin, s)) {
		if (s == "" || s[0] == '#') {
			continue;
		}

		auto args = split_string(s);

		if (args[0] == "name") {
			std::cout << "= BudgieBot\n\n";
		}

		else if (args[0] == "version") {
			std::cout << "= 0.0.1\n\n";
		}

		else if (args[0] == "protocol_version") {
			std::cout << "= 2\n\n";
		}

		else if (args[0] == "list_commands") {
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

			coordinate coord = string_to_coord(args[2]);

			if (args[2] == "pass" || args[2] == "PASS") {
				passed = true;

			} else {
				// TODO: same todo as below
				passed = false;
				game.current_player = player;
				game.make_move(coord);
			}

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
			// TODO: should have function to handle this, rather than mutating the
			//       game state from here

			game.current_player = string_to_color(args[1]);
			search_tree->reset();
			coordinate coord = search_tree->do_search(&game, playouts);

			if (!game.is_valid_coordinate(coord)) {
				std::cerr << "# no valid moves, passing" << std::endl;
				std::cout << "= pass\n\n";
				continue;
			}

			std::cerr << "# coord: (" << coord.first << ", " << coord.second
				<< "), win rate: " << search_tree->win_rate(coord)
				<< ", traversals: "
				<< std::dec << search_tree->root->leaves[coord]->traversals
				<< std::endl;

			if (search_tree->win_rate(coord) < 0.15) {
				std::cout << "= resign\n\n";
				continue;
			}

			if (passed || !game.is_valid_move(coord)) {
				std::cout << "= pass\n\n";
				continue;
			}

			/*
			if (search_tree->win_rate(coord) == 1) {
				std::cout << "= pass\n\n";
				continue;
			}
			*/

			if (game.is_suicide(coord, game.current_player)) {
				std::cout << "= pass\n\n";
				continue;
			}

			std::cout << "= " << coord_string(coord) << "\n\n";

			game.make_move(coord);
			std::cerr << "# board hash: " << std::hex << game.hash << std::dec << std::endl;

			//std::string k = search_tree->serialize();
			std::vector<uint32_t> k = search_tree->serialize();
			std::cerr << "serialized length: " << 4*k.size()
				<< " (" << 4*k.size() / 1024 << "KiB)"<< std::endl;

		}

		else if (args[0] == "move_history") {
			std::cout << "= ";

			for (move::moveptr ptr = game.move_list;
			     ptr != nullptr;
			     ptr = ptr->previous)
			{
				std::cout << color_to_string(ptr->color) << " "
				          << coord_string(ptr->coord) << '\n';
			}

			std::cout << '\n';
		}

		else if (args[0] == "showboard") {
			std::cout << "= \n";
			game.print();
			std::cout << "\n\n";
		}

		else if (args[0] == "quit") {
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
