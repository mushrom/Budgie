#include <budgie/mcts.hpp>
#include <math.h>

namespace mcts_thing {

mcts_node* mcts_tree_policy::search(board *state, mcts_node *ptr) {
	while (ptr) {
		if (!ptr->fully_visited(state)) {
			return ptr;
		}

		coordinate next = {0, 0};
		double max = 0;

		for (const auto& x : state->available_moves) {
			if (!state->is_valid_move(x)) {
				continue;
			}

			unsigned hash = coord_hash_v2(x);
			if (ptr->nodestats[hash].win_rate() > max) {
				next = x;
				max = ptr->nodestats[hash].win_rate();
			}
		}

		if (next == coordinate(0, 0)) {
			ptr->update(state);
			return nullptr;
		}

		ptr->new_node(state, next, state->current_player);
		state->make_move(next);
		ptr = ptr->leaves[coord_hash_v2(next)];
	}

	return nullptr;
}

// namespace mcts_thing
}
