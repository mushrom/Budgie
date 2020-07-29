#include <budgie/mcts.hpp>
#include <math.h>
#include <assert.h>
#include <unistd.h>

namespace mcts_thing {

coordinate attack_enemy_groups_playout::apply(board *state) {
	coordinate next = {0, 0};
	point::color other = other_player(state->current_player);
	// opponent groups with two liberties
	std::list<group*>& weakgrps = state->group_liberties[other][2];

	// TODO: probability here should be configurable
	if (rand() % 6 == 0 && !weakgrps.empty()) {
		// pick a random group to capture
		group *ptr = *std::next(weakgrps.begin(), rand() % weakgrps.size());
		assert(ptr != nullptr);

		// coin flip between the two liberties
		next = *std::next(ptr->liberties.begin(), rand() % 1);

		if (!state->is_valid_move(next)) {
			next = {0, 0};
		}

		//std::cerr << "boom! attacked a group!" << std::endl;
	}

	return next;
}

// namespace mcts_thing
}
