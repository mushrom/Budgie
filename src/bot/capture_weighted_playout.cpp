#include <budgie/mcts.hpp>
#include <math.h>
#include <assert.h>
#include <unistd.h>

namespace mcts_thing {

mcts_node* capture_weighted_playout::playout(board *state, mcts_node *ptr) {
	while (true) {
		coordinate next;
		point::color other = state->other_player(state->current_player);

		if (rand() % 3 == 0 && state->ataris[other].size() > 0) {
			// pick random move
			size_t ngroups = state->ataris[other].size();
			group *ptr = *std::next(state->ataris[other].begin(), rand() % ngroups);

			assert(ptr != nullptr);
			// if the group is in the 'ataris' set, it should only
			// have one liberty, so there's no need to randomly pick
			// from the liberty set.
			next = *ptr->liberties.begin();

			if (!state->is_valid_move(next)) {
				next = pick_random_leaf(state);
			}

			//std::cerr << "boom! captured a group!" << std::endl;

		} else {
			next = pick_random_leaf(state);
		}

		if (next == coordinate(0, 0)) {
			ptr->update(state);
			return nullptr;
		}

		state->make_move(next);
		//state->print();
		//usleep(100000);
	}

	return nullptr;
}

// namespace mcts_thing
}
