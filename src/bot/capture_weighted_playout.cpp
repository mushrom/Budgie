#include <budgie/mcts.hpp>
#include <math.h>
#include <assert.h>
#include <unistd.h>

namespace mcts_thing {

coordinate capture_weighted_playout::apply(board *state) {
	coordinate next = {0, 0};
	point::color other = state->other_player(state->current_player);

	if (rand() % 3 == 0 && !state->ataris[other].empty()) {
		// pick random move
		size_t ngroups = state->ataris[other].size();
		group *ptr = *std::next(state->ataris[other].begin(), rand() % ngroups);

		assert(ptr != nullptr);
		// if the group is in the 'ataris' set, it should only
		// have one liberty, so there's no need to randomly pick
		// from the liberty set.
		next = *ptr->liberties.begin();

		if (!state->is_valid_move(next)) {
			next = {0, 0};
		}

		//std::cerr << "boom! captured a group!" << std::endl;
	}

	return next;
}

// namespace mcts_thing
}
