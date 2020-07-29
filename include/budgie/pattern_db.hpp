#pragma once

#include <budgie/game.hpp>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <stdint.h>

namespace mcts_thing {

class pattern {
	public:
		void print(void);
		char minigrid[9] = {0};
		unsigned weight = 0;
		bool valid = false;

		void rotate_grid(void);
		void flip_horizontally(void);
		void flip_vertically(void);
		uint32_t hash(void);
};

class pattern_db {
	public:
		pattern_db(const std::string& db);
		unsigned search(board *state, coordinate coord);
		uint32_t hash_grid(board *state, point::color grid[9]);
	
	private:
		void read_grid(board *state, coordinate coord, point::color grid[9]);
		pattern read_pattern(std::ifstream& f);
		void load_pattern(pattern& pat);
		void load_permutations(pattern pat, unsigned index=0);
		void load_compile(pattern& pat);
		void dump_patterns(void);
		std::unordered_map<uint32_t, unsigned> patterns;
};

}
