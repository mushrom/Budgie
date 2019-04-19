#pragma once

#include <mcts-gb/game.hpp>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>

namespace mcts_thing {

class pattern {
	public:
		bool matches(board *state, coordinate coord);
		void print(void);
		char minigrid[9] = {0};
		unsigned weight = 0;
		int x_offset = 0;
		int y_offset = 0;
		bool valid = false;

	private:
		bool test_grid(board *state, point::color grid[9]);
		void rotate_grid(point::color grid[9]);
		void flip_horizontally(point::color grid[9]);
		void flip_vertically(point::color grid[9]);

		void read_grid(board *state, coordinate coord, point::color grid[9], int y_dir, int x_dir);
		void print_grid(point::color grid[9]);

};

class pattern_db {
	public:
		pattern_db(const std::string& db);
		unsigned search(board *state, coordinate coord);
	
	private:
		pattern read_pattern(std::ifstream& f);

		std::vector<pattern> patterns;
};

}
