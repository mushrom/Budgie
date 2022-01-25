#include <budgie/mcts.hpp>
#include <budgie/playout_strategies.hpp>
#include <math.h>
#include <assert.h>
#include <unistd.h>

namespace bdg::playouts {

void attack_enemy_groups_playout(board *state, move_queue& queue) {
	point::color other = other_player(state->current_player);
	// opponent groups with two liberties
	std::list<group*>& weakgrps = state->group_liberties[other][2];
	std::uniform_int_distribution<unsigned> dice(0, 5);

	// TODO: probability here should be configurable
	if (/*roll == 0 &&*/ !weakgrps.empty()) {
		std::uniform_int_distribution<unsigned> coinflip(0, 1);
		std::uniform_int_distribution<unsigned> groupindex(0, weakgrps.size()-1);

		// pick a random group to capture
		group *ptr = *std::next(weakgrps.begin(), groupindex(state->randomgen));
		assert(ptr != nullptr);

		// coin flip between the two liberties
		coordinate next = *std::next(ptr->liberties.begin(), coinflip(state->randomgen));

		if (state->is_valid_move(next)) {
			queue.add(next, 100);
		}
	}
}

// namespace bdg
}
