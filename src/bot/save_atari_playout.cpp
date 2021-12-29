#include <budgie/mcts.hpp>
#include <budgie/playout_strategies.hpp>
#include <math.h>
#include <assert.h>
#include <unistd.h>

namespace mcts_thing::playouts {

maybe_coord save_atari_playout(board *state) {
	point::color current = state->current_player;
	std::list<group*>& ataris = state->group_liberties[current][1];
	std::uniform_int_distribution<unsigned> diceroll(0, 5);

	// TODO: probability here should be configurable
	if (diceroll(state->randomgen) == 0 && !ataris.empty()) {
		std::uniform_int_distribution<unsigned> atarichoice(0, ataris.size()-1);

		// pick a random group to capture
		group *ptr = *std::next(ataris.begin(), atarichoice(state->randomgen));
		//assert(ptr != nullptr);
		coordinate next = *ptr->liberties.begin();

		if (!state->is_valid_move(next)) {
			return {};
		} else {
			return next;
		}

		//std::cerr << "boom! saved a group!" << std::endl;
	}

	return {};
}

// namespace mcts_thing
}
