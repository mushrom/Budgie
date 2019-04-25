#pragma once

#include <vector>
#include <list>
#include <utility>
#include <stdio.h>
#include <stdint.h>
#include <map>
#include <memory>
#include <bitset>
#include <unordered_map>

namespace mcts_thing {

typedef std::pair<unsigned, unsigned> coordinate;

struct coord_hash {
	std::size_t operator () (const coordinate& coord) const {
		return (coord.first * 3313) + (coord.second * 797);
	}
};

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

class stone_group {
	public:
		typedef std::shared_ptr<stone_group> ptr;

		//std::vector<coordinate> stones;
		std::list<coordinate> stones;
		point::color color;

		unsigned liberties = 0;
};

class board {
	public:
		board(unsigned size = 9){
			grid.reserve(size * size);
			//groups.reserve(2 * size * size);
			dimension = size;

			for (unsigned i = 0; i < size*size; i++){
				grid[i] = point::color::Empty;
				//groups[i] = nullptr;
			}

			groups.clear();
			groups.resize(dimension * dimension, nullptr);
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
			//groups.reserve(2 * dimension * dimension);
			groups.clear();
			groups.resize(dimension * dimension, nullptr);

			for (unsigned i = 0; i < dimension * dimension; i++) {
				grid[i] = other->grid[i];

				if (other->groups[i] != nullptr) {
					stone_group::ptr foo(new stone_group());
					//*foo = *other->groups[i];
					foo->liberties = other->groups[i]->liberties;
					foo->color = other->groups[i]->color;

					/*
					for (auto& x : other->groups[i]->stones) {
						foo->stones.push_back(x);
					}
					*/

					foo->stones = {other->groups[i]->stones.begin(), other->groups[i]->stones.end()};

					//groups.push_back(foo);
					groups[i] = foo;
				}

				//groups[i] = stone_group::ptr(new stone_group);
				//*groups[i] = *other->groups[i];
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

		// main thing
		void make_move(const coordinate& coord);

		// basic checking functions
		bool is_valid_move(const coordinate& coord);
		bool is_valid_coordinate(const coordinate& coord);
		bool violates_ko(const coordinate& coord);

		// public stone group functions
		//bool is_suicide(const coordinate& coord, point::color color);
		// TODO: remove color param
		bool is_suicide(const coordinate& coord);

		// scoring stuff
		unsigned count_stones(point::color player);
		unsigned count_territory(point::color player);
		std::vector<coordinate> available_moves(void);
		point::color determine_winner(void);

		// helpers for internal data structures
		uint64_t gen_hash(const coordinate& coord, point::color color);
		unsigned coord_to_index(const coordinate& coord);

		// misc
		void print(void);
		void reset(unsigned boardsize, unsigned n_komi) {
			dimension = boardsize;
			komi = n_komi;
			moves = 0;

			grid.reserve(dimension * dimension);
			groups.clear();
			groups.resize(dimension * dimension, nullptr);
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

		//std::unordered_map<coordinate, stone_group::ptr, coord_hash> groups;
		std::vector<stone_group::ptr> groups;

	private:
		void endgame_mark_captured(const coordinate& coord, point::color color,
		                           std::bitset<384>& marked);
		void endgame_clear_captured(void);
		unsigned territory_flood(const coordinate& coord,
		                         bool is_territory,
		                         std::bitset<384>& traversed_map);

		// TODO: remove set_coordinate
		//void set_coordinate(const coordinate& coord, point::color color) {};
		void set_coordinate(const coordinate& coord, point::color color);

		bool reaches_iter(const coordinate& coord, point::color color,
		                  point::color target, std::bitset<384>& marked);
		bool reaches(const coordinate& coord, point::color color, point::color target);
		bool reaches_empty(const coordinate& coord, point::color color);

		unsigned count_iter(const coordinate& coord,
		                    point::color color,
		                    point::color target,
		                    std::bitset<384>& marked,
		                    std::bitset<384>& traversed);
		unsigned count_reachable(const coordinate& coord,
		                         point::color color,
		                         point::color target,
		                         std::bitset<384>& traversed);

		//void clear_stones(const coordinate& coord, point::color color);
		//bool clear_enemy_stones(const coordinate& coord, point::color color);
		//bool clear_own_stones(const coordinate& coord, point::color color);

		// stone group functions
		unsigned get_neighbors(const coordinate& coord, stone_group::ptr buf[4]);
		unsigned friendly_neighbors(stone_group::ptr buf[4]);
		unsigned enemy_neighbors(stone_group::ptr buf[4]);
		unsigned neighbor_liberties(stone_group::ptr neighbors[4], point::color color);
		stone_group::ptr link_friendly(stone_group::ptr neighbors[4]);
		//bool captures_enemy(const coordinate& coord, point::color color);
		// TODO: remove color param
		bool captures_enemy(const coordinate& coord);
		void capture_enemy_stones(const coordinate& coord);
		void place_stone(const coordinate& coord, point::color color);
};

// namespace mcts_thing
}
