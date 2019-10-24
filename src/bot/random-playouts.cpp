#include <budgie/mcts.hpp>
#include <math.h>

namespace mcts_thing {

coordinate random_playout::apply(board *state) {
	/*
	while (true) {
		coordinate next = pick_random_leaf(state);

		if (next == coordinate(0, 0)) {
			ptr->update(state);
			return nullptr;
		}

		state->make_move(next);
	}

	return nullptr;
	*/

	return pick_random_leaf(state);
}

// namespace mcts_thing
}
