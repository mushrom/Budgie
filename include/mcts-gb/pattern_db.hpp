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
		char minigrid[9] = {0};
		unsigned weight = 0;
		bool valid = false;

	private:
		bool test_grid(board *state, point::color grid[9]);
		void flip_grid_horizontally(point::color grid[9]);
		void flip_grid_vertically(point::color grid[9]);

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
