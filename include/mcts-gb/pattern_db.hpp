#pragma once

#include <mcts-gb/game.hpp>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>

namespace mcts_thing {

class pattern {
	public:
		void print(void);
		char minigrid[9] = {0};
		unsigned weight = 0;
		int x_offset = 0;
		int y_offset = 0;
		bool valid = false;

		void rotate_grid(void);
		void flip_horizontally(void);
		void flip_vertically(void);
};

class pattern_node {
	public:
		char exp;
		unsigned weight;

		std::unordered_map<char, pattern_node*> matchers;
		void dump(unsigned depth=0);
};

class pattern_db {
	public:
		pattern_db(const std::string& db);
		unsigned search(board *state, coordinate coord);
	
	private:
		void read_grid(board *state, coordinate coord, point::color grid[9]);
		bool test_match(board *state, char m, point::color c);
		pattern read_pattern(std::ifstream& f);
		void tree_load_pattern(pattern& pat);
		void load_pattern(pattern& pat);
		unsigned specificity(char c);
		pattern_node tree;
};

}
