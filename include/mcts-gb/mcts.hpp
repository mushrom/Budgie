#pragma once

#include <mcts-gb/game.hpp>
#include <list>
#include <utility>
#include <unordered_map>
#include <map>
#include <memory>
#include <bitset>

namespace mcts_thing {

// TODO: maybe move this to the 'board' class
struct coord_hash {
	std::size_t operator () (const coordinate& coord) const {
		return (coord.first * 3313) + (coord.second * 797);
	}
};

class rave_map {
	public:
		rave_map(rave_map *par = nullptr) {
			parent = par;
		};

		class stats {
			public:
				unsigned wins = 0;
				unsigned traversals = 0;
				double win_rate(void) {
					// TODO: we shouldn't have a state where traversals is 0, right?
					return traversals? (double)wins / (double)traversals : 0.5;
				};
		};

		typedef std::shared_ptr<rave_map> ptr;

		rave_map *parent = nullptr;
		std::unordered_map<coordinate, stats, coord_hash> leaves;
};

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
		coordinate pick_random_leaf(board *state, bool use_patterns);

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

		std::unordered_map<coordinate, nodeptr, coord_hash> leaves;
		rave_map::ptr rave;
		coordinate self_coord;

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
			//root = new mcts_node(nullptr, point::color::Empty);
			reset();
		};

		~mcts(){};

		coordinate do_search(board *state,
		                     unsigned playouts=20000,
		                     bool use_patterns=true);
		double win_rate(coordinate& coord);

		void reset() {
			delete root;
			root = new mcts_node(nullptr, point::color::Empty);
			root->rave = rave_map::ptr(new rave_map(nullptr));
		}

		mcts_node *root = nullptr;
};

// namespace mcts_thing
}
