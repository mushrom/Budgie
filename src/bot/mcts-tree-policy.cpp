#include <budgie/mcts.hpp>
#include <math.h>

namespace mcts_thing {

mcts_node* mcts_tree_policy::search(board *state, mcts_node *ptr) {
	while (ptr) {
		if (!ptr->fully_visited(state)) {
			coordinate next = pick_random_leaf(state);

			// no valid moves from here, just return
			if (!state->is_valid_move(next)) {
				ptr->update(state);
				return nullptr;
			}

			ptr->new_node(next, state->current_player);
			return ptr->leaves[next].get();
		}

		coordinate next = ptr->best_move();

		if (next == coordinate(0, 0)) {
			ptr->update(state);
			return nullptr;
		}

		ptr->new_node(next, state->current_player);
		state->make_move(next);
		ptr = ptr->leaves[next].get();
	}

	return nullptr;
}

// namespace mcts_thing
}