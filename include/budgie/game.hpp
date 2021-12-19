#pragma once

#include <budgie/josekiDB.hpp>
#include <vector>
#include <utility>
#include <stdio.h>
#include <stdint.h>
#include <map>
#include <set>
#include <memory>
#include <bitset>
#include <anserial/anserial.hpp>
#include <unordered_set>

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

class group {
	struct coord_hash {
		std::size_t operator () (const coordinate& coord) const {
			return (coord.first << 5) | coord.second;
		}
	};

	public:
		point::color color;

		// list so we can splice stone lists together easily
		std::list<coordinate> members;

		// set so we can add/remove members quickly
		std::unordered_set<coordinate, coord_hash> liberties;

		// self-reference so that we can efficiently move groups around
		std::list<group*>::iterator it;

		// number of liberties at last update
		size_t last_update = 0xdeadbeef;
};

static inline constexpr std::array<coordinate, 4> adjacent(const coordinate& coord) {
	return {
		coordinate {coord.first - 1, coord.second},
		coordinate {coord.first + 1, coord.second},
		coordinate {coord.first,     coord.second - 1},
		coordinate {coord.first,     coord.second + 1},
	};
}

// TODO: no reason for this to be a member function,
//       we could inline this...
static inline point::color other_player(point::color color) {
	return (color == point::color::Black)
		? point::color::White
		: point::color::Black;
}

// TODO: generic 'game' class, add a way to enumerate/validate/randomly pick moves
class board {
	public:
		enum {
			//InitialHash = 19937,
			InitialHash = 1073741827,
		};

		board(unsigned size = 9);
		~board();

		// XXX
		board(board *other);
		void set(board *other);

		inline unsigned coord_to_index(const coordinate& coord) const {
			return dimension*(coord.second - 1) + (coord.first - 1);
		}

		inline coordinate index_to_coord(unsigned index) const {
			return {(index % dimension) + 1, (index / dimension) + 1};
		}

		inline bool is_valid_coordinate(const coordinate& coord) const {
			return is_valid_coordinate(coord.first, coord.second);
		}

		inline bool is_valid_coordinate(unsigned x, unsigned y) const {
			return x > 0 && y > 0 && x <= dimension && y <= dimension;
		}

		inline point::color get_coordinate(const coordinate& coord) const {
			if (!is_valid_coordinate(coord)) {
				return point::color::Invalid;
			}

			return (point::color)grid[(coord.second - 1)*dimension + (coord.first - 1)];
		}

		inline point::color get_coordinate(unsigned x, unsigned y) const {
			if (!is_valid_coordinate(x, y)) {
				return point::color::Invalid;
			}

			return (point::color)grid[(y - 1)*dimension + (x - 1)];
		}

		// get_coordinate() with no is_valid_coordinate(),
		// in case it's already known that the coordinate is valid
		inline point::color get_coordinate_unsafe(const coordinate& coord) const {
			return (point::color)grid[(coord.second - 1)*dimension + (coord.first - 1)];
		}

		inline point::color get_coordinate_unsafe(unsigned x, unsigned y) const {
			return (point::color)grid[(y - 1)*dimension + (x - 1)];
		}

		bool is_valid_move(const coordinate& coord);
		bool is_suicide(const coordinate& coord, point::color color);
		bool violates_ko(const coordinate& coord);
		bool captures_enemy(const coordinate& coord, point::color color);
		bool make_move(const coordinate& coord);
		unsigned count_stones(point::color player);
		unsigned count_territory(point::color player);
		point::color determine_winner(void);
		std::string determine_score(void);

		void print(void);
		void reset(unsigned boardsize, unsigned n_komi);
		void loadJosekis(std::string list);

		std::vector<uint32_t> serialize(void);
		void serialize(anserial::serializer& ser, uint32_t parent);
		void deserialize(anserial::s_node *node);

		move::moveptr move_list;
		uint8_t grid[384];
		uint64_t hash = InitialHash;
		int komi = 7;
		unsigned dimension;
		unsigned moves = 0;
		point::color current_player = point::color::Black;
		coordinate last_move;
		board *parent = nullptr;

		std::shared_ptr<josekiDB> josekis;


		// TODO: maybe we should have a group map class, no need to clutter the
		//       board class more...
		void group_place(const coordinate& coord);
		void group_link(group **a, group **b);
		void group_try_capture(group **a, const coordinate& coord);
		void group_clear(group **a);

		void group_libs_update(group *a);
		void group_libs_remove(group *a);

		void group_update_neighbors(group **a);
		void group_propagate(group **a);
		bool group_check(void);
		void group_print(void);
		size_t group_capturable(const coordinate& coord, group *arr[4]);
		uint64_t group_pseudocapture(const coordinate& coord, group *arr[4]);

		// group pointer for every board square
		group *groups[384];
		// array of lists sorted by color and the number of liberties
		// in the group
		std::list<group*> group_liberties[4][384];

		bool owns(const coordinate& coord, point::color color);
		uint8_t ownership[384];
		std::set<coordinate> available_moves;

		void regen_hash(void);
		uint64_t regen_hash(uint8_t grid[384]);

	private:
		void endgame_mark_captured(const coordinate& coord, point::color color,
		                           std::bitset<384>& marked);
		void endgame_clear_captured(void);
		unsigned territory_flood(const coordinate& coord,
		                         bool is_territory,
		                         std::bitset<384>& traversed_map);

		void set_coordinate(const coordinate& coord, point::color color);
		void set_coordinate_unsafe(const coordinate& coord, point::color color);
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

		void clear_stones(const coordinate& coord, point::color color);
		bool clear_enemy_stones(const coordinate& coord, point::color color);
		bool clear_own_stones(const coordinate& coord, point::color color);
};

// namespace mcts_thing
}
