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
		for (unsigned x = y; x < 3; x++) {
			/*
			unsigned ny = 2 - x;
			unsigned nx = y;

			point::color temp = grid[3*ny + nx];

			grid[3*ny + nx] = grid[3*y + x];
			grid[3*y + x] = temp;
			*/

			point::color temp = grid[3*y + x];

			grid[3*y + x] = grid[3*x + y];
			grid[3*x + y] = temp;
		}
	}
}

void pattern::flip_horizontally(point::color grid[9]) {
	for (unsigned y = 0; y < 3; y++) {
		point::color temp = grid[3*y];
		grid[3*y] = grid[3*y + 2];
		grid[3*y + 2] = temp;
	}
}

void pattern::flip_vertically(point::color grid[9]) {
	for (unsigned x = 0; x < 3; x++) {
		point::color temp = grid[x];
		grid[x] = grid[3*2 + x];
		grid[3*2 + x] = temp;
	}
}

void pattern::read_grid(board *state, coordinate coord, point::color grid[9]) {
	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			//fprintf(stderr, "- x: %d, y: %d\n", x, y);
			coordinate k = {coord.first + x, coord.second + y};
			//grid[3*(y-y_offset) + (x-x_offset)] = state->get_coordinate(k);
			grid[(y+1)*3 + (x+1)] = state->get_coordinate(k);
		}
	}
}

#define RET_IF_MATCHES(S, T) {\
	if (test_grid(S, T)) {\
		return true;\
	}\
}

bool pattern::matches(board *state, coordinate coord) {
	bool ret = false;
	point::color temp[9];

	read_grid(state, coord, temp);

	if (temp[4] != point::color::Empty) {
		// if the point isn't empty, there's no pattern that will match
		return false;
	}

	ret = ret || test_grid(state, temp);

	// test different orientations of the grid which still match some pattern
	flip_horizontally(temp);
	//ret = ret || test_grid(state, temp);
	RET_IF_MATCHES(state, temp);

	flip_vertically(temp);
	//ret = ret || test_grid(state, temp);
	RET_IF_MATCHES(state, temp);

	flip_horizontally(temp);
	//ret = ret || test_grid(state, temp);
	RET_IF_MATCHES(state, temp);

	flip_vertically(temp);
	rotate_grid(temp);
	//ret = ret || test_grid(state, temp);
	RET_IF_MATCHES(state, temp);

	flip_horizontally(temp);
	//ret = ret || test_grid(state, temp);
	RET_IF_MATCHES(state, temp);

	flip_vertically(temp);
	//ret = ret || test_grid(state, temp);
	RET_IF_MATCHES(state, temp);

	flip_horizontally(temp);
	//ret = ret || test_grid(state, temp);
	RET_IF_MATCHES(state, temp);

	return false;
}

void pattern::print_grid(point::color grid[9]){
	for (unsigned y = 0; y < 3; y++) {
		for (unsigned x = 0; x < 3; x++) {
			switch (grid[y*3 + x]) {
				case point::color::Empty:
					std::cerr << '.';
					break;

				case point::color::Black:
					std::cerr << 'O';
					break;

				case point::color::White:
					std::cerr << '#';
					break;

				case point::color::Invalid:
					std::cerr << '+';
					break;
			}

			std::cerr << ' ';
			//std::cerr << grid[y*3 + x];
		}

		std::cerr << std::endl;
	}
}

void pattern::print(void) {
	std::cerr << "weight: " << weight << ", ";
	std::cerr << "x off: " << x_offset << ", y off: " << y_offset << std::endl;

	for (unsigned y = 0; y < 3; y++) {
		for (unsigned x = 0; x < 3; x++) {
			std::cerr << minigrid[y*3 + x] << ' ';
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

	// TODO: remove y_offset and x_offset fields
	if (ret.y_offset != 1 && ret.x_offset != 1) {
		std::cerr << "warning: invalid pattern:" << std::endl;
		ret.valid = false;
		ret.print();
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
			//if (p.weight > 20) {
				std::cerr << "matched pattern with weight " << p.weight <<
					" at (" << coord.first << "," << coord.second << ")" << std::endl;
				state->print();
				p.print();
			//}
			*/
			return p.weight;
		}
	}

	return 10;
}

// mcts_thing
}
