#include <mcts-gb/pattern_db.hpp>
#include <stdlib.h>

namespace mcts_thing {

bool pattern::test_grid(board *state, point::color grid[9]) {
	bool ret = true;

	for (unsigned i = 0; ret && i < 9; i++) {
		switch (minigrid[i]) {
			case 'O':
				ret = grid[i] == state->current_player;
				break;

			case 'o':
				ret = grid[i] == state->current_player
				   || grid[i] == point::color::Empty;
				break;

			case 'X':
				ret = grid[i] == state->other_player(state->current_player);
				break;

			case 'x':
				ret = grid[i] == state->other_player(state->current_player)
				   || grid[i] == point::color::Empty;
				break;

			case '|':
			case '-':
			case '+':
				ret = grid[i] == point::color::Invalid;
				break;

			case '*':
				ret = grid[i] == point::color::Empty;
				break;

			case '?':
			default:
				break;
		}
	}

	return ret;
}

void pattern::flip_grid_horizontally(point::color grid[9]) {
	for (unsigned i = 0; i < 3; i++) {
		point::color temp = grid[3*i];
		grid[3*i] = grid[3*i + 2];
		grid[3*i + 2] = temp;
	}
}

void pattern::flip_grid_vertically(point::color grid[9]) {
	for (unsigned i = 0; i < 3; i++) {
		point::color temp = grid[i];
		grid[i] = grid[6 + i];
		grid[6 + i] = temp;
	}
}

bool pattern::matches(board *state, coordinate coord) {
	bool ret = false;
	point::color temp[9];

	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			coordinate k = {coord.first + x, coord.second + y};
			temp[3*(y+1) + (x+1)] = state->get_coordinate(k);
		}
	}

	for (unsigned y = 0; y < 2; y++) {
		for (unsigned x = 0; x < 2; x++) {
			ret = ret || test_grid(state, temp);
			flip_grid_horizontally(temp);
		}

		flip_grid_vertically(temp);
	}

	return ret;
}

pattern pattern_db::read_pattern(std::ifstream& f){
	pattern ret;
	std::string buf = "";

	for (unsigned i = 0; i < 4; i++) {
asdf:
		if (!std::getline(f, buf)) {
			f.close();
			return ret;
		}

		while (buf == "" || buf[0] == '#') {
			goto asdf;
		}

		if (i < 3) {
			for (unsigned k = 0; k < 3; k++) {
				ret.minigrid[i*3 + k] = buf[k];
			}
		}

		else {
			ret.weight = atoi(buf.substr(1).c_str());
		}
	}

	/*
	std::cerr << "read a pattern with weight " << ret.weight << ":\n";

	for (unsigned y = 0; y < 3; y++) {
		for (unsigned x = 0; x < 3; x++) {
			std::cerr << ret.minigrid[y*3 + x];
		}

		std::cerr << std::endl;
	}
	*/

	ret.valid = true;
	return ret;
}

pattern_db::pattern_db(const std::string& db) {
	std::ifstream pf(db);

	while (pf.is_open()) {
		pattern p = read_pattern(pf);

		if (p.valid) {
			patterns.push_back(p);
		}
	}
}

unsigned pattern_db::search(board *state, coordinate coord) {
	for (auto& p : patterns) {
		if (p.matches(state, coord)) {
			//std::cerr << "matched pattern with weight " << p.weight << std::endl;
			return p.weight;
		}
	}

	return 10;
}

// mcts_thing
}
