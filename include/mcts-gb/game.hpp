#pragma once

#include <vector>
#include <utility>
#include <stdio.h>
#include <map>

namespace mcts_thing {

typedef std::pair<unsigned, unsigned> coordinate;

class point {
	public:
		coordinate coord;
		enum color {
			Empty,
			Invalid,
			Black,
			White,
		} state;

		point(){};
		point(coordinate x, enum color c){
			coord = x;
			state = c;
		};
		~point(){};
};

class board {
	public:
		board(unsigned size = 9){
			grid.reserve(size * size);
			dimension = size;

			for (unsigned i = 0; i < size*size; i++){
				grid[i] = point::color::Empty;
			}
		};

		// XXX
		board(board& other) {
			set(other);
		}

		void set(board& other) {
			dimension = other.dimension;
			moves = other.moves;
			current_player = other.current_player;

			grid.reserve(dimension * dimension);

			for (unsigned i = 0; i < dimension * dimension; i++) {
				grid[i] = other.grid[i];
			}
		}

		point::color get_coordinate(coordinate& coord);
		point::color other_player(point::color color) {
			return (color == point::color::Black)
				? point::color::White
				: point::color::Black;
		};

		bool is_valid_move(coordinate& coord);
		bool is_valid_coordinate(coordinate& coord);
		bool is_suicide(coordinate& coord, point::color color);
		bool captures_enemy(coordinate& coord, point::color color);
		void make_move(coordinate& coord);
		unsigned count_stones(point::color player);
		unsigned count_territory(point::color player);
		point::color determine_winner(void);
		void print(void);

		std::vector<point::color> grid;
		int komi = 7;
		unsigned dimension;
		unsigned moves = 0;
		point::color current_player = point::color::Black;
		coordinate last_move;

	private:
		void set_coordinate(coordinate& coord, point::color color);
		bool reaches_iter(coordinate& coord, point::color color, point::color target, std::map<coordinate, bool>& marked);
		bool reaches(coordinate& coord, point::color color, point::color target);
		bool reaches_empty(coordinate& coord, point::color color);
		void clear_stones(coordinate& coord, point::color color);
		void clear_enemy_stones(coordinate& coord, point::color color);
		void clear_own_stones(coordinate& coord, point::color color);
};

// namespace mcts_thing
}
