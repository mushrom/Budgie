#pragma once

#include <vector>
#include <utility>
#include <stdio.h>
#include <stdint.h>
#include <map>
#include <memory>
#include <bitset>
#include <anserial/anserial.hpp>

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

// TODO: generic 'game' class, add a way to enumerate/validate/randomly pick moves
class board {
	public:
		enum {
			//InitialHash = 19937,
			InitialHash = 1073741827,
			//InitialHash = 65537,
		};

		board(unsigned size = 9);

		// XXX
		board(board *other);
		void set(board *other);

		point::color get_coordinate(const coordinate& coord);

		// TODO: no reason for this to be a member function,
		//       we could inline this...
		point::color other_player(point::color color);

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
		void reset(unsigned boardsize, unsigned n_komi);

		std::vector<uint32_t> serialize(void);
		void serialize(anserial::serializer& ser, uint32_t parent);
		void deserialize(anserial::s_node *node);

		move::moveptr move_list;
		std::vector<point::color> grid;
		uint64_t hash = InitialHash;
		int komi = 7;
		unsigned dimension;
		unsigned moves = 0;
		point::color current_player = point::color::Black;
		coordinate last_move;
		board *parent = nullptr;

		bool owns(const coordinate& coord, point::color color);
		point::color ownership[384];

	private:
		void regen_hash(void);
		void endgame_mark_captured(const coordinate& coord, point::color color,
		                           std::bitset<384>& marked);
		void endgame_clear_captured(void);
		unsigned territory_flood(const coordinate& coord,
		                         bool is_territory,
		                         std::bitset<384>& traversed_map);

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

		void clear_stones(const coordinate& coord, point::color color);
		bool clear_enemy_stones(const coordinate& coord, point::color color);
		bool clear_own_stones(const coordinate& coord, point::color color);
};

// namespace mcts_thing
}
