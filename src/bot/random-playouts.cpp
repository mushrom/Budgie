#include <budgie/mcts.hpp>
#include <math.h>

namespace bdg::playouts {

maybe_coord random_playout(board *state) {
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

	return pick_random_leaf(state, patterns.get());
	*/

	coordinate coord = pick_random_leaf(state, &get_pattern_db());

	if (coord.first == 0) {
		return {};
	} else {
		return coord;
	}
}

// namespace bdg
}
