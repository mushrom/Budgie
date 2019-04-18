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

			case '.':
				// '*' will be matched as the point for the next move,
				// so it can't be used as a matcher for empty space
				ret = grid[i] == point::color::Empty;
				break;

			case '?':
			default:
				break;
		}
	}

	return ret;
}

void pattern::rotate_grid(point::color grid[9]) {
	for (unsigned y = 0; y < 3; y++) {
		for (unsigned x = 0; x < 3; x++) {
			point::color temp = grid[y*3 + x];

			grid[y*3 + x] = grid[x*3 + y];
			grid[x*3 + y] = temp;
		}
	}
}

void pattern::read_grid(board *state, coordinate coord, point::color grid[9], int y_dir, int x_dir) {
	int y_start = y_dir * y_offset;
	int y_end = y_start + y_dir*-3;

	int x_start = x_dir * x_offset;
	int x_end = x_start + x_dir*-3;

	int j = 0;
	for (int y = y_start; y != y_end; y -= y_dir, j++) {

		int m = 0;
		for (int x = x_start; x != x_end; x -= x_dir, m++) {
			coordinate k = {coord.first + x, coord.second + y};
			//grid[3*(y-y_offset) + (x-x_offset)] = state->get_coordinate(k);
			grid[j*3 + m] = state->get_coordinate(k);
		}
	}
}

bool pattern::matches(board *state, coordinate coord) {
	bool ret = false;
	point::color temp[9];

	for (int y = -1; y <= 1; y += 2) {
		for (int x = -1; x <= 1; x += 2) {
			read_grid(state, coord, temp, y, x);
			ret = ret || test_grid(state, temp);

			rotate_grid(temp);
			ret = ret || test_grid(state, temp);

			rotate_grid(temp);
			ret = ret || test_grid(state, temp);

			rotate_grid(temp);
			ret = ret || test_grid(state, temp);
		}
	}

	return ret;
}

void pattern::print(void) {
	std::cerr << "weight: " << weight << ", ";
	std::cerr << "x off: " << x_offset << ", y off: " << y_offset << std::endl;

	for (unsigned y = 0; y < 3; y++) {
		for (unsigned x = 0; x < 3; x++) {
			std::cerr << minigrid[y*3 + x];
		}

		std::cerr << std::endl;
	}
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

		if (buf == "" || buf[0] == '#') {
			goto asdf;
		}

		if (i < 3) {
			for (unsigned k = 0; k < 3; k++) {
				ret.minigrid[i*3 + k] = buf[k];

				if (buf[k] == '*') {
					ret.x_offset = k;
					ret.y_offset = i;
				}
			}
		}

		else {
			ret.weight = atoi(buf.substr(1).c_str());
		}
	}

	//ret.print();
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
			/*
			if (p.weight > 20) {
				std::cerr << "matched pattern with weight " << p.weight <<
					" at (" << coord.first << "," << coord.second << ")" << std::endl;
				state->print();
				p.print();
			}
			*/
			return p.weight;
		}
	}

	return 10;
}

// mcts_thing
}
