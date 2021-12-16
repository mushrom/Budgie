#include <budgie/mcts.hpp>
#include <math.h>

namespace mcts_thing {

coordinate local_weighted_playout::apply(board *state) {
	coordinate things[9];
	unsigned found = 0;
	// default weight is 100, so look for anything better than random
	unsigned best = 0;

	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			coordinate foo = {
				state->last_move.first  + x,
				state->last_move.second + y
			};

			if (foo.first == 0 && foo.second == 0) {
				continue;
			}

			unsigned weight = patterns->search(state, foo);

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
		return things[rand() % found];
	}

	return coordinate(0, 0);
}

// namespace mcts_thing
}
