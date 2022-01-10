#include <budgie/mcts.hpp>
#include <math.h>

namespace mcts_thing::policies {

maybe_nodeptr mcts_tree_policy(board *state, mcts_node *ptr) {
	while (ptr) {
		if (!ptr->can_traverse(state)) {
			return ptr;
		}

		maybe_coord next = {};
		mcts_node *nextleaf = nullptr;
		double max = 0;

		for (mcts_node *leaf : ptr->leaves_alive) {
			float winrate = leaf->win_rate();
			if (winrate > max) {
				next = leaf->coord;
				max = winrate;
				nextleaf = leaf;
			}
		}

		if (!next) {
			return ptr;

		} else {
			state->make_move(*next);
			ptr = nextleaf;
		}
	}

	return {};
}

// namespace mcts_thing
}
