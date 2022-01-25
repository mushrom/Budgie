#include <budgie/mcts.hpp>
#include <math.h>

namespace bdg::playouts {

void local_weighted_playout(board *state, move_queue& queue) {
	// TODO: collect all neighbors and choose randomly based on weight,
	//       rather than picking the highest rated
	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			coordinate foo = {
				state->last_move.first  + x,
				state->last_move.second + y
			};

			if (foo.first == 0 && foo.second == 0) {
				continue;
			}

			if (state->get_coordinate(foo) != point::color::Empty) {
				continue;
			}

			if (!state->is_valid_move(foo)) {
				continue;
			}

			unsigned weight = get_pattern_db().search(state, foo);
			queue.add(foo, weight);
		}
	}
}

// namespace bdg::playouts
}
