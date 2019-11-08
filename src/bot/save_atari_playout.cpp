#include <budgie/mcts.hpp>
#include <math.h>
#include <assert.h>
#include <unistd.h>

namespace mcts_thing {

coordinate save_atari_playout::apply(board *state) {
	coordinate next = {0, 0};
	point::color current = state->current_player;
	std::list<group*>& ataris = state->group_liberties[current][1];

	// TODO: probability here should be configurable
	if (rand() % 6 == 0 && !ataris.empty()) {
		// pick a random group to capture
		group *ptr = *std::next(ataris.begin(), rand() % ataris.size());
		assert(ptr != nullptr);
		next = *ptr->liberties.begin();

		if (!state->is_valid_move(next)) {
			next = {0, 0};
		}

		//std::cerr << "boom! saved a group!" << std::endl;
	}

	return next;
}

// namespace mcts_thing
}
