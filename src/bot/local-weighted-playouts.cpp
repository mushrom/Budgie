#include <budgie/mcts.hpp>
#include <math.h>

namespace mcts_thing::playouts {

maybe_coord local_weighted_playout(board *state) {
	coordinate things[9];
	unsigned found = 0;
	// default weight is 100, so look for anything better than random
	unsigned best = 0;

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

			// TODO: update, store patterns in board class or something
			unsigned weight = get_pattern_db().search(state, foo);
			//unsigned weight = 100;

			if (weight == 0 || !state->is_valid_move(foo)) {
				continue;
			}

			if (weight > best) {
				best = weight;
				found = 0;
				things[found++] = foo;
			}

			else if (weight == best) {
				things[found++] = foo;
			}
		}
	}

	if (found > 0) {
		std::uniform_int_distribution<unsigned> foundindex(0, found-1);
		return things[foundindex(state->randomgen)];
	}

	return {};
}

// namespace mcts_thing::playouts
}
