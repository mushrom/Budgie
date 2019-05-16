#include <mcts-gb/mcts.hpp>
#include <math.h>

namespace mcts_thing {

mcts_node* random_playout::playout(board *state, mcts_node *ptr) {
	while (true) {
		coordinate next = pick_random_leaf(state);

		if (next == coordinate(0, 0)) {
			ptr->update(state);
			return nullptr;
		}

		state->make_move(next);
	}

	return nullptr;
}

// namespace mcts_thing
}
