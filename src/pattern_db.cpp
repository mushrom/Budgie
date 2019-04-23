#include <mcts-gb/pattern_db.hpp>
#include <stdlib.h>

namespace mcts_thing {

void pattern::rotate_grid(void) {
	for (unsigned y = 0; y < 3; y++) {
		for (unsigned x = y; x < 3; x++) {
			char temp = minigrid[3*y + x];

			minigrid[3*y + x] = minigrid[3*x + y];
			minigrid[3*x + y] = temp;
		}
	}
}

void pattern::flip_horizontally(void) {
	for (unsigned y = 0; y < 3; y++) {
		char temp = minigrid[3*y];
		minigrid[3*y] = minigrid[3*y + 2];
		minigrid[3*y + 2] = temp;
	}
}

void pattern::flip_vertically(void) {
	for (unsigned x = 0; x < 3; x++) {
		char temp = minigrid[x];
		minigrid[x] = minigrid[3*2 + x];
		minigrid[3*2 + x] = temp;
	}
}

#define RET_IF_MATCHES(S, T) {\
	if (test_grid(S, T)) {\
		return true;\
	}\
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
			load_pattern(p);
		}
	}

	tree.dump();
}

void pattern_node::dump(unsigned depth) {
	for (unsigned i = 0; i < depth; i++) {
		std::cerr << "  ";
	}

	fprintf(stderr, "'%c' : %u\n", exp, weight);

	for (const auto& x : matchers) {
		if (x.second != nullptr) {
			x.second->dump(depth + 1);
		}
	}
}

void pattern_db::load_pattern(pattern& pat) {
	tree_load_pattern(pat);

	// test different orientations of the grid which still match some pattern
	pat.flip_horizontally();
	tree_load_pattern(pat);

	pat.flip_vertically();
	tree_load_pattern(pat);

	pat.flip_horizontally();
	tree_load_pattern(pat);

	pat.flip_vertically();
	pat.rotate_grid();
	tree_load_pattern(pat);

	pat.flip_horizontally();
	tree_load_pattern(pat);

	pat.flip_vertically();
	tree_load_pattern(pat);

	pat.flip_horizontally();
	tree_load_pattern(pat);
}

void pattern_db::tree_load_pattern(pattern& pat) {
	pattern_node *ptr = &tree;

	for (unsigned i = 0; i < 9 /* TODO: maybe make grid size variable? */; i++) {
		char c = pat.minigrid[i];

		if (ptr->matchers[c] == nullptr) {
			ptr->matchers[c] = new pattern_node();
		}

		ptr->matchers[c]->exp = pat.minigrid[i];
		ptr->matchers[c]->weight = pat.weight;
		ptr = ptr->matchers[pat.minigrid[i]];
	}
}

unsigned pattern_db::search(board *state, coordinate coord) {
	pattern_node *ptr = &tree;
	point::color grid[9];
	read_grid(state, coord, grid);

	for (unsigned i = 0; i < 9; i++) {
		pattern_node *next = nullptr;

		for (const auto& x : ptr->matchers) {
			if (test_match(state, x.second->exp, grid[i])) {
				next = x.second;
				break;
			}
		}

		if ((ptr = next) == nullptr) {
			return 100;
		}
	}

	// TODO: add 'unlikely()' here
	if (!ptr) {
		/*
		std::cerr << "Hold up, we're supposed to have another pointer here!"
			<< std::endl;
			*/

		return 100;
	}

	return ptr->weight;
}

bool pattern_db::test_match(board *state, char m, point::color c) {
	switch (m) {
		case 'O':
			return c == state->current_player;
			break;

		case 'o':
			return c == state->current_player || c == point::color::Empty;
			break;

		case 'X':
			return c == state->other_player(state->current_player);
			break;

		case 'x':
			return c == state->other_player(state->current_player)
			    || c == point::color::Empty;
			break;

		case '|':
		case '-':
		case '+':
			return c == point::color::Invalid;
			break;

		case '*':
			return c == point::color::Empty;
			break;

		case '.':
			// '*' will be matched as the point for the next move,
			// so it can't be used as a matcher for empty space
			return c == point::color::Empty;
			break;

		case '?':
			return true;
			break;

		default:
			return false;
			break;
	}
}

void pattern_db::read_grid(board *state, coordinate coord, point::color grid[9]) {
	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			//fprintf(stderr, "- x: %d, y: %d\n", x, y);
			coordinate k = {coord.first + x, coord.second + y};
			//grid[3*(y-y_offset) + (x-x_offset)] = state->get_coordinate(k);
			grid[(y+1)*3 + (x+1)] = state->get_coordinate(k);
		}
	}
}

// mcts_thing
}
