#include <budgie/mcts.hpp>
#include <budgie/parameters.hpp>
#include <math.h>

namespace mcts_thing::policies {

static inline double uct(const coordinate& coord, board *state, mcts_node *ptr) {
	if (!state->is_valid_move(coord)) {
		return 0;
	}

	double weight = get_pattern_db().search(state, coord) / 100.0;

	if (weight == 0) {
		return 0;
	}

	/*
	if (ptr->nodestats[coord].traversals == 0) {
		// unexplored leaf
		return 0.5;
	}
	*/

	unsigned hash = coord_hash_v2(coord);
	double mcts_est = ptr->nodestats[hash].win_rate();

	double uct =
		getFloat(PARAM_FLOAT_UCT_WEIGHT)
		* sqrt(log((int)ptr->traversals) / ptr->nodestats[hash].traversals);
	/*
	double uct =
		uct_weight * sqrt(log(ptr->traversals) /
			((ptr->nodestats[hash].traversals > 0)
				? ptr->nodestats[hash].traversals
				: 1));
				*/


	double mc_uct_est = mcts_est + uct;

	return mc_uct_est;
}

static inline maybe_coord max_utc(board *state, mcts_node *ptr) {
	double cur_max = 0;
	maybe_coord ret = {};

	for (const auto& x : state->available_moves) {
		double temp = uct(x, state, ptr);

		if (temp > cur_max) {
			cur_max = temp;
			ret = x;
		}
	}

	return ret;
}


maybe_nodeptr uct_tree_policy(board *state, mcts_node *ptr) {
	while (ptr) {
		if (!ptr->fully_visited(state)) {
			return ptr;
		}

		maybe_coord next = max_utc(state, ptr);

		if (!next) {
			ptr->update(state);
			return {};
		}

		ptr->new_node(state, *next, state->current_player);
		state->make_move(*next);
		ptr = ptr->leaves[coord_hash_v2(*next)];
	}

	return {};
}

// namespace mcts_thing
}
