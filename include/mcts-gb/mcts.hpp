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

class mcts_node {
	public:
		class stats {
			public:
				unsigned wins = 0;
				unsigned traversals = 0;
				double win_rate(void) {
					// TODO: we shouldn't have a state where traversals is 0, right?
					return traversals? (double)wins / (double)traversals : 0.5;
				};
		};

		typedef std::unique_ptr<mcts_node> nodeptr;

		mcts_node(mcts_node *n_parent = nullptr, point::color player = point::color::Empty) {
			color = player;
			parent = n_parent;
			traversals = wins = 0;
		};

		// XXX: toggleable patterns in UCT weighting for testing, good chance
		//      it'll be removed... eventually
		void explore(board *state, bool use_patterns);
		void update(board *state);
		void update_rave(board *state, point::color winner);
		coordinate pick_random_leaf(board *state, bool use_patterns);

		mcts_node* tree_search(board *state, bool use_patterns);
		mcts_node* random_playout(board *state, bool use_patterns);
		mcts_node* weighted_playout(board *state, bool use_patterns);
		mcts_node* local_weighted_playout(board *state, bool use_patterns);
		void new_node(board *state, coordinate& coord);

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
		void dump_best_move_statistics(board *state);

		std::unordered_map<coordinate, nodeptr, coord_hash> leaves;

		// rave stats
		std::unordered_map<coordinate, stats, coord_hash> ravemap;

		mcts_node *parent;
		unsigned color;
		unsigned traversals;
		unsigned wins;

		std::bitset<384> move_map;
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
			//root->rave = rave_map::ptr(new rave_map(point::color::Empty, nullptr));
		}

		mcts_node *root = nullptr;
};

// namespace mcts_thing
}
