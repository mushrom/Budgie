#include <mcts-gb/mcts.hpp>
#include <math.h>

namespace mcts_thing {

coordinate local_weighted_playout::local_best(board *state) {
	coordinate things[9];
	unsigned found = 0;
	// default weight is 100, so look for anything better than random
	unsigned best = 101;

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

			if (weight > best && state->is_valid_move(foo)) {
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

mcts_node* local_weighted_playout::playout(board *state, mcts_node *ptr) {
	while (true) {
		coordinate next = {0, 0};
		std::bitset<384> map;

		next = local_best(state);

		if (next != coordinate(0, 0)) {
			goto asdf;
		}

		next = pick_random_leaf(state);

		if (next == coordinate(0, 0)) {
			ptr->update(state);
			return nullptr;
		}

// TODO: asdf
asdf:
		/*
		// debugging output
		   printf("\e[1;1H");
		   state->print();
		   usleep(100000);
		   */

		state->make_move(next);
	}

	return nullptr;
}

// namespace mcts_thing
}
