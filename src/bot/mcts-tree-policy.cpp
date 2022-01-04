#include <budgie/mcts.hpp>
#include <math.h>

namespace mcts_thing::policies {

maybe_nodeptr mcts_tree_policy(board *state, mcts_node *ptr) {
	while (ptr) {
		if (!ptr->fully_visited(state) || !ptr->try_expanding(state)) {
			return ptr;
		}

		maybe_coord next = {};
		double max = 0;

		for (const auto& leaf : ptr->leaves_alive) {
			const coordinate& x = leaf->coord;
			unsigned hash = coord_hash_v2(x);

			/*
			if (!state->is_valid_move(x)) {
				continue;
			}
			*/

			float winrate = ptr->leaves[hash]->win_rate();
			if (winrate > max) {
				next = x;
				max = winrate;
			}
		}

		/*
		if (next == coordinate(0, 0)) {
			ptr->update(state);
			return {};
		}
		*/

		if (!next) {
			return ptr;

		} else {
			//ptr->new_node(state, next, state->current_player);
			state->make_move(*next);
			ptr = ptr->leaves[coord_hash_v2(*next)];
		}
	}

	return {};
}

// namespace mcts_thing
}
