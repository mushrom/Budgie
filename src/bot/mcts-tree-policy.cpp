#include <budgie/mcts.hpp>
#include <math.h>

namespace mcts_thing::policies {

maybe_nodeptr mcts_tree_policy(board *state, mcts_node *ptr) {
	while (ptr) {
		if (!ptr->fully_visited(state) || !ptr->try_expanding(state)) {
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
			//ptr->new_node(state, next, state->current_player);
			state->make_move(*next);
			//ptr = ptr->leaves[coord_hash_v2(*next)];
			ptr = nextleaf;
		}
	}

	return {};
}

// namespace mcts_thing
}
