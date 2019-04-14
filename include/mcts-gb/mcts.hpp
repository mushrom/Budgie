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

		void explore(board *state, unsigned playouts=4000, unsigned branching=80);
		void exploit(board *state, unsigned moves=8);
		coordinate best_move(void);
		void update(coordinate& coord, point::color winner);
		double win_rate(void);

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

		void reset() {
			delete root;
			root = new mcts_node(nullptr, point::color::Empty);
		}

		mcts_node *root;
};

// namespace mcts_thing
}
