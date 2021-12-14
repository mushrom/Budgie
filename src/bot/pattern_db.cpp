#include <budgie/pattern_db.hpp>
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

	for (unsigned y = 0; y < 3; y++) {
		for (unsigned x = 0; x < 3; x++) {
			std::cerr << minigrid[y*3 + x] << ' ';
		}

		std::cerr << std::endl;
	}
}

// TODO: if hashes only use 18 bits, why not use uint32_t?
uint32_t pattern::hash(void) {
	uint32_t ret = 0;

	// XXX: only handling exact specifiers here
	for (unsigned i = 0; i < 9; i++) {
		// TODO: should do this before ORing no?
		//       update: yeah, probably
		//       leaving this here in case this causes problems in the future
		ret <<= 2;

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
	}

	return ret;
}

pattern pattern_db::read_pattern(std::ifstream& f){
	pattern ret;
	std::string buf = "";

	for (unsigned i = 0; i < 4; i++) {
retry:
		if (!std::getline(f, buf)) {
			f.close();
			return ret;
		}

		if (buf == "" || buf[0] == '#') {
			goto retry;
		}

		// handle pattern line
		if (i < 3) {
			for (unsigned k = 0; k < 3; k++) {
				ret.minigrid[i*3 + k] = buf[k];
			}

		// last line is weight definition
		} else {
			ret.weight = atoi(buf.substr(1).c_str());
		}
	}

	//ret.print();
	ret.valid = true;
	return ret;
}

pattern_db::pattern_db(const std::string& db) {
	std::ifstream pf(db);
	pattern_values = std::unique_ptr<uint8_t>(new uint8_t[1 << 18]);

	uint8_t *values = pattern_values.get();
	for (int i = 0; i < (1<<18); i++) {
		values[i] = 100;
	}

	while (pf.is_open()) {
		pattern p = read_pattern(pf);
		load_pattern(p);
	}

	dump_patterns();

	std::cerr << "# total patterns loaded: " << total_patterns << std::endl;
}

void pattern_db::dump_patterns(void) {
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
			// wildcard, generate all possible combinations
			pat.minigrid[index] = '.';
			load_permutations(pat, index + 1);
			pat.minigrid[index] = 'O';
			load_permutations(pat, index + 1);
			pat.minigrid[index] = 'X';
			load_permutations(pat, index + 1);
			break;

		default:
			// exact specifier, continue onwards
			load_permutations(pat, index + 1);
			break;
	}
}

void pattern_db::load_compile(pattern& pat) {
	uint32_t hash = pat.hash();

	uint8_t *ptr = pattern_values.get();
	/* 0x3ffff = (1<<18)-1 */
	ptr[hash & 0x3ffff] = (uint8_t)(pat.weight&0xff);
	total_patterns++;
}

unsigned pattern_db::search(board *state, coordinate coord) {
	point::color grid[9];
	read_grid(state, coord, grid);

	uint32_t hash = hash_grid(state, grid);

	uint8_t *ptr = pattern_values.get();
	return ptr[hash & 0x3ffff];
}

uint32_t pattern_db::hash_grid(board *state, point::color grid[9]) {
	uint32_t ret = 0;
	point::color player = state->current_player;
	point::color other  = other_player(player);

	for (unsigned i = 0; i < 9; i++) {
		ret <<= 2;

		if (grid[i] == point::color::Empty) {
			ret |= 0;
		}

		if (grid[i] == player) {
			ret |= 1;
		}

		if (grid[i] == other) {
			ret |= 2;
		}

		if (grid[i] == point::color::Invalid) {
			ret |= 3;
		}
	}

	return ret;
}

void pattern_db::read_grid(board *state, coordinate coord, point::color grid[9]) {
	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			//coordinate k = {coord.first + x, coord.second + y};
			grid[(y+1)*3 + (x+1)] = state->get_coordinate(coord.first + x, coord.second + y);
		}
	}
}

// mcts_thing
}
