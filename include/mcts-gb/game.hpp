#pragma once

#include <vector>
#include <utility>
#include <stdio.h>
#include <stdint.h>
#include <map>
#include <memory>
#include <bitset>

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

class move {
	public:
		typedef std::shared_ptr<move> moveptr;

		move(moveptr prev,
		     const coordinate& koord,
		     point::color c,
		     uint64_t h)
		{
			previous = prev;
			coord    = koord;
			color    = c;
			hash     = h;
		}

		std::shared_ptr<move> previous;
		coordinate coord;
		point::color color;
		uint64_t hash;
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
		board(board *other) {
			set(other);
		}

		void set(board *other) {
			// copy game info
			komi = other->komi;
			dimension = other->dimension;
			current_player = other->current_player;
			moves = other->moves;
			last_move = other->last_move;

			// copy internal info
			move_list = other->move_list;
			hash = other->hash;

			grid.reserve(dimension * dimension);

			for (unsigned i = 0; i < dimension * dimension; i++) {
				grid[i] = other->grid[i];
			}
		}

		enum {
			//InitialHash = 19937,
			InitialHash = 1073741827,
			//InitialHash = 65537,
		};

		point::color get_coordinate(const coordinate& coord);
		point::color other_player(point::color color) {
			return (color == point::color::Black)
				? point::color::White
				: point::color::Black;
		};

		bool is_valid_move(const coordinate& coord);
		bool is_valid_coordinate(const coordinate& coord);
		bool is_suicide(const coordinate& coord, point::color color);
		bool violates_ko(const coordinate& coord);
		bool captures_enemy(const coordinate& coord, point::color color);
		void make_move(const coordinate& coord);
		unsigned count_stones(point::color player);
		unsigned count_territory(point::color player);
		std::vector<coordinate> available_moves(void);
		point::color determine_winner(void);
		uint64_t gen_hash(const coordinate& coord, point::color color);
		unsigned coord_to_index(const coordinate& coord);
		void print(void);
		void reset(unsigned boardsize, unsigned n_komi) {
			dimension = boardsize;
			komi = n_komi;
			grid.reserve(dimension * dimension);
			moves = 0;

			for (unsigned i = 0; i < dimension * dimension; i++) {
				grid[i] = point::color::Empty;
			}
		}

		move::moveptr move_list;
		std::vector<point::color> grid;
		uint64_t hash = InitialHash;
		int komi = 7;
		unsigned dimension;
		unsigned moves = 0;
		point::color current_player = point::color::Black;
		coordinate last_move;
		board *parent = nullptr;

	private:
		unsigned territory_flood(const coordinate& coord,
		                         bool is_territory,
		                         std::bitset<384>& traversed_map);

		void set_coordinate(const coordinate& coord, point::color color);
		bool reaches_iter(const coordinate& coord, point::color color, point::color target, std::bitset<384>& marked);
		bool reaches(const coordinate& coord, point::color color, point::color target);
		bool reaches_empty(const coordinate& coord, point::color color);
		void clear_stones(const coordinate& coord, point::color color);
		bool clear_enemy_stones(const coordinate& coord, point::color color);
		bool clear_own_stones(const coordinate& coord, point::color color);
};

// namespace mcts_thing
}
