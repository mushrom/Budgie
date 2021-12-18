#include <budgie/mcts.hpp>
#include <math.h>
#include <assert.h>
#include <unistd.h>

namespace mcts_thing {

coordinate capture_weighted_playout::apply(board *state) {
	coordinate next = {0, 0};
	point::color other = other_player(state->current_player);
	std::list<group*>& ataris = state->group_liberties[other][1];

	if (state->moves == 0) {
		// nothing to do on the first move
		return {0, 0};
	}

	// check if the group of the opponent's last move is in atari,
	// do a coin flip to determine whether to capture it
	group *g = *(state->groups + state->coord_to_index(state->last_move));
	if (g && g->liberties.size() == 1 && (rand()&1) == 1) {
		coordinate temp = *g->liberties.begin();

		if (!state->is_valid_coordinate(temp)) {
			printf("invalid liberty at (%d, %d)?\n", temp.first, temp.second);
		}

		if (state->is_valid_move(temp)) {
			return temp;
		}
	}

	// TODO: probability here should be configurable
	if (rand() % 6 == 0 && !ataris.empty()) {
		// pick a random group to capture
		group *ptr = *std::next(ataris.begin(), rand() % ataris.size());
		assert(ptr != nullptr);
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
