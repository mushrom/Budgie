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

uint64_t pattern::hash(void) {
	uint64_t ret = 1;

	// XXX: only handling exact specifiers here
	for (unsigned i = 0; i < 9; i++) {
		switch (minigrid[i]) {
			case '.':
			case '*':
				ret |= 0;
				break;

			case 'O':
				ret |= 1;
				break;

			case 'X':
				ret |= 2;
				break;

			case '-':
			case '|':
			case '+':
				ret |= 3;
				break;

			default:
				break;
		}

		ret <<= 2;
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
		load_pattern(p);
	}

	dump_patterns();

	std::cerr << "# total patterns loaded: " << patterns.size() << std::endl;
}

void pattern_db::dump_patterns(void) {
	/*
	for (auto& thing : patterns) {
		thing.print();
	}
	*/
}

void pattern_db::load_pattern(pattern& pat) {
	if (!pat.valid) {
		return;
	}

	load_permutations(pat);

	pat.flip_horizontally();
	load_permutations(pat);

	pat.flip_vertically();
	load_permutations(pat);

	pat.flip_horizontally();
	load_permutations(pat);

	pat.flip_vertically();
	pat.rotate_grid();
	load_permutations(pat);

	pat.flip_horizontally();
	load_permutations(pat);

	pat.flip_vertically();
	load_permutations(pat);

	pat.flip_horizontally();
	load_permutations(pat);
}

void pattern_db::load_permutations(pattern pat, unsigned index) {
	if (index >= 9) {
		// end of pattern grid
		load_compile(pat);
		return;
	}

	switch (pat.minigrid[index]) {
		case 'X':
		case 'O':
		case '.':
		case '|':
		case '-':
		case '+':
		case '*':
			// nothing to do for exact pattern specifiers, so we continue onwards
			load_permutations(pat, index + 1);
			break;

		case 'x':
			// load patterns with and without an enemy stone
			pat.minigrid[index] = '.';
			load_permutations(pat, index + 1);
			pat.minigrid[index] = 'X';
			load_permutations(pat, index + 1);
			break;

		case 'o':
			// same with own stones
			pat.minigrid[index] = '.';
			load_permutations(pat, index + 1);
			pat.minigrid[index] = 'O';
			load_permutations(pat, index + 1);
			break;

		case '?':
			// true wildcard, generate all possible combinations
			pat.minigrid[index] = '.';
			load_permutations(pat, index + 1);
			pat.minigrid[index] = 'O';
			load_permutations(pat, index + 1);
			pat.minigrid[index] = 'X';
			load_permutations(pat, index + 1);
			pat.minigrid[index] = '+';
			load_permutations(pat, index + 1);

			break;

		default:
			// we shouldn't get here, maybe print an error
			load_permutations(pat, index + 1);
			break;
	}
}

void pattern_db::load_compile(pattern& pat) {
	patterns[pat.hash()] = pat.weight;
}

unsigned pattern_db::search(board *state, coordinate coord) {
	point::color grid[9];
	read_grid(state, coord, grid);

	uint64_t hash = hash_grid(state, grid);
	auto it = patterns.find(hash);

	if (it == patterns.end()) {
		return 100;
	}

	return it->second;
}

uint64_t pattern_db::hash_grid(board *state, point::color grid[9]) {
	uint64_t ret = 1;

	for (unsigned i = 0; i < 9; i++) {
		if (grid[i] == point::color::Empty) {
			ret |= 0;
		}

		if (grid[i] == state->current_player) {
			ret |= 1;
		}

		if (grid[i] == state->other_player(state->current_player)) {
			ret |= 2;
		}

		if (grid[i] == point::color::Invalid) {
			ret |= 3;
		}

		ret <<= 2;
	}

	return ret;
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
