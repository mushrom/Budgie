#pragma once

#include <mcts-gb/game.hpp>
#include <list>
#include <utility>
#include <map>
#include <memory>
#include <bitset>

#define MCTS_UCT_C 0.35

namespace mcts_thing {

class mcts_node {
	public:
		typedef std::unique_ptr<mcts_node> nodeptr;

		mcts_node(mcts_node *n_parent = nullptr, point::color player = point::color::Empty) {
			color = player;
			parent = n_parent;
			traversals = wins = 0;
		};

		// XXX: toggleable patterns in UCT weighting for testing, good chance
		//      it'll be removed... eventually
		void explore(coordinate& coord, board *state, bool use_patterns);
		void update(point::color winner);
		double win_rate(void);
		double uct(const coordinate& coord, board *state, bool use_patterns);
		coordinate best_move(void);
		coordinate max_utc(board *state, bool use_patterns);
		unsigned terminal_nodes(void);
		unsigned nodes(void);
		bool fully_visited(board *state);
		void map_set_coord(coordinate& coord, board *state);
		bool map_get_coord(coordinate& coord, board *state);

		void dump_node_statistics(const coordinate& coord, board *state, unsigned depth=0);

		std::map<coordinate, nodeptr> leaves;
		mcts_node *parent;
		unsigned color;
		unsigned traversals;
		unsigned wins;

		std::bitset<384> move_map = {0};
		unsigned unique_traversed = 0;
};

class mcts {
	public:
		mcts() {
			root = new mcts_node(nullptr, point::color::Empty);
		};

		~mcts(){};

		coordinate do_search(board *state,
		                     unsigned playouts=20000,
		                     bool use_patterns=true);
		double win_rate(coordinate& coord);

		void reset() {
			delete root;
			root = new mcts_node(nullptr, point::color::Empty);
		}

		mcts_node *root;
};

// namespace mcts_thing
}
