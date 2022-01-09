#include <budgie/gtp.hpp>
#include <budgie/budgie.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

namespace mcts_thing {

// XXX: leaving this here for now, to avoid inadvertently messing up
//      GTP parsing in the same commit as adding utility.hpp
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
	budgie bot(options);
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
			// XXX: duplicated state here is because might not necessarily
			//      want to reset the board, this usually happens after
			//      clear_board.
			bot.komi = atof(args[1].c_str());
			bot.game.komi = bot.komi;

			std::cout << "=\n\n";
		}

		else if (args[0] == "boardsize") {
			// needs to be followed by clear_board to take effect, dunno if this is
			// spec but it makes the implementation less messy

			bot.boardsize = atoi(args[1].c_str());
			std::cout << "=\n\n";
		}

		else if (args[0] == "clear_board") {
			bot.reset();
			std::cout << "=\n\n";
		}

		else if (args[0] == "play") {
			point::color player = string_to_color(args[1]);
			coordinate coord = string_to_coord(args[2]);

			if (args[2] == "pass" || args[2] == "PASS") {
				bot.make_move(budgie::move::types::Pass);
				std::cout << "=\n\n";
			}

			else {
				bool valid = bot.make_move(
				    budgie::move(budgie::move::types::Move,
				                 coord,
				                 player));

				std::cout << (valid? "=\n\n" : "? invalid move\n\n");
			}
		}

		else if (args[0] == "set_free_handicap") {
			//point::color player = game.current_player;
			point::color player = bot.game.current_player;

			for (auto it = args.begin() + 1; it != args.end(); it++) {
				coordinate coord = string_to_coord(*it);
				bot.make_move(budgie::move(budgie::move::types::Move,
				                           coord,
				                           player));
			}

			std::cout << "=\n\n";
		}

		else if (args[0] == "genmove") {
			bot.set_player(string_to_color(args[1]));
			budgie::move move = bot.genmove();

			//std::cerr << "Testing this!" << std::endl;

			if (move.type == budgie::move::types::Move) {
				unsigned hash = coord_hash_v2(move.coord);

				if (bot.ogsChat) {
					std::cerr
						<< "MALKOVICH: "
						<< "win rate: " << 100*bot.tree->win_rate(move.coord)
						<< "%, traversals: "
						//<< std::dec << bot.tree->root->leaves[hash]->traversals
						<< std::endl;

					std::flush(std::cerr);
				}
			}

			/*
			std::cerr << "# board hash: "
				<< std::hex << bot.game.hash << std::dec
				<< std::endl;
				*/

			switch (move.type) {
				case budgie::move::types::Pass:
					std::cout << "= pass\n\n";
					break;

				case budgie::move::types::Resign:
					std::cout << "= resign\n\n";
					break;

				default:
					bot.make_move(move);
					std::cout << "= " << coord_string(move.coord) << "\n\n";
					break;
			}

		}

		else if (args[0] == "move_history") {
			std::cout << "= ";

			for (move::moveptr ptr = bot.game.move_list;
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
			bot.game.print();
			std::cout << "\n";
		}

		else if (args[0] == "quit") {
			std::cout << "=\n\n";
			return;
		}

		else if (args[0] == "final_score") {
			std::cout << "= ";
			std::cout << bot.game.get_score_string();
			std::cout << "\n\n";
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
