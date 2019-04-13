#pragma once

#include <mcts-gb/game.hpp>
#include <list>
#include <utility>
#include <map>

namespace mcts_thing {

class mcts_node {
	public:
		mcts_node(mcts_node *n_parent = nullptr, point::color player = point::color::Empty) {
			color = player;
			parent = n_parent;
			traversals = wins = 0;
		};

		void explore(coordinate coord, board *state,
		             unsigned playouts=1500, unsigned depth=4, unsigned branching=10);
		coordinate best_move(void);
		void update(coordinate& coord, point::color winner);
		std::map<coordinate, mcts_node> leaves;
		mcts_node *parent;

		unsigned color;
		unsigned traversals;
		unsigned wins;
};

class mcts {
	public:
		mcts() {
			root = new mcts_node(nullptr, point::color::Empty);
		};

		~mcts(){};

		mcts_node *root;
};

// namespace mcts_thing
}
