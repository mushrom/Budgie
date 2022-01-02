#include <budgie/mcts.hpp>
#include <budgie/playout_strategies.hpp>
#include <math.h>
#include <assert.h>
#include <unistd.h>

namespace mcts_thing::playouts {

maybe_coord capture_weighted_playout(board *state) {
	point::color other = other_player(state->current_player);
	std::list<group*>& ataris = state->group_liberties[other][1];

	if (state->moves == 0) {
		// nothing to do on the first move
		return {};
	}

	std::uniform_int_distribution<unsigned> diceroll(0, 5);
	std::uniform_int_distribution<unsigned> coinflip(0, 1);

	if (state->last_move != coordinate {0, 0}) {
		// check if the group of the opponent's last move is in atari,
		// do a coin flip to determine whether to capture it
		group *g = *(state->groups + state->coord_to_index(state->last_move));
		if (g && g->liberties.size() == 1 && coinflip(state->randomgen) == 1) {
			coordinate temp = *g->liberties.begin();

			if (!state->is_valid_coordinate(temp)) {
				fprintf(stderr, "invalid liberty at (%d, %d)?\n", temp.first, temp.second);
			}

			if (state->is_valid_move(temp)) {
				return temp;
			}
		}
	}

	// TODO: probability here should be configurable
	if (diceroll(state->randomgen) == 0 && !ataris.empty()) {
		std::uniform_int_distribution<unsigned> atarichoice(0, ataris.size()-1);

		// pick a random group to capture
		group *ptr = *std::next(ataris.begin(), atarichoice(state->randomgen));
		assert(ptr != nullptr);
		coordinate next = *ptr->liberties.begin();

		if (!state->is_valid_move(next)) {
			return {};
		} else {
			return next;
		}

		//std::cerr << "boom! captured a group!" << std::endl;
	}

	return {};
}

// namespace mcts_thing
}
