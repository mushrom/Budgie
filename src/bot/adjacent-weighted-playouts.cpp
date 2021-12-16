#include <budgie/mcts.hpp>
#include <math.h>

namespace mcts_thing {

coordinate adjacent_playout::apply(board *state) {
	coordinate things[9];
	unsigned found = 0;

	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			coordinate foo = {
				state->last_move.first  + x,
				state->last_move.second + y
			};

			if (foo.first == 0 && foo.second == 0) {
				continue;
			}

			if (state->get_coordinate(foo) == point::Empty) {
				things[found++] = foo;
			}
		}
	}

	if (found > 0) {
		coordinate next = things[rand() % found];
		if (!state->is_valid_move(next)) {
			return {0, 0};
		}
	}

	return coordinate(0, 0);
}

// namespace mcts_thing
}
