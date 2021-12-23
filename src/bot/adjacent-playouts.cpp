#include <budgie/mcts.hpp>
#include <math.h>

namespace mcts_thing {

coordinate adjacent_3x3_playout::apply(board *state) {
	coordinate things[25];
	unsigned found = 0;

	if (state->moves == 0) {
		// don't use adjacency for the first move
		return {0, 0};
	}

	// picks randomly from the 3x3 grid surrounding the player
	// (not including the last move, which is at the center)
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
		// TODO: will constructing distributions like this be a performance problem?
		std::uniform_int_distribution<unsigned> udist(0, found-1);
		coordinate next = things[udist(state->randomgen)];

		// TODO: check some number of other possibilities
		if (!state->is_valid_move(next)) {
			return {0, 0};

		} else {
			return next;
		}
	}

	return coordinate(0, 0);
}

coordinate adjacent_5x5_playout::apply(board *state) {
	coordinate things[25];
	unsigned found = 0;

	if (state->moves == 0) {
		// don't use adjacency for the first move
		return {0, 0};
	}

	// picks randomly from the 5x5 grid surrounding the player
	// (not including the last move, which is at the center)
	for (int y = -2; y <= 2; y++) {
		for (int x = -2; x <= 2; x++) {
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
		std::uniform_int_distribution<unsigned> udist(0, found-1);
		coordinate next = things[udist(state->randomgen)];

		// TODO: check some number of other possibilities
		if (!state->is_valid_move(next)) {
			return {0, 0};

		} else {
			return next;
		}
	}

	return coordinate(0, 0);
}

// namespace mcts_thing
}
