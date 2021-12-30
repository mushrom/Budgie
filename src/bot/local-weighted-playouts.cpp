#include <budgie/mcts.hpp>
#include <math.h>

namespace mcts_thing::playouts {

maybe_coord local_weighted_playout(board *state) {
	coordinate things[9];
	unsigned weights[9];
	unsigned found = 0;
	// default weight is 100, so look for anything better than random
	unsigned total_weights = 0;

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

			unsigned weight = get_pattern_db().search(state, foo);
			if (weight < 100 || !state->is_valid_move(foo)) {
				continue;
			}

			things[found] = foo;
			weights[found] = weight;
			found++;
			total_weights += weight;
		}
	}

	if (found > 0) {
		std::uniform_int_distribution<unsigned> foundindex(0, total_weights-1);
		unsigned choice = foundindex(state->randomgen);
		unsigned weightsum = 0;

		for (unsigned i = 0; i < found; i++) {
			weightsum += weights[i];
			if (choice < weightsum) {
				return things[i];
			}
		}
	}

	return {};
}

// namespace mcts_thing::playouts
}
