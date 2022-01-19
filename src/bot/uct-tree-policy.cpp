#include <budgie/mcts.hpp>
#include <budgie/parameters.hpp>
#include <math.h>

namespace bdg::policies {

static inline double uct(board *state, mcts_node *parent, mcts_node *child) {
	//unsigned hash = coord_hash_v2(coord);

	/*
	// TODO: could just initialize with only valid moves...
	if (!state->is_valid_move(coord)) {
		return 0;
	}
	*/

	//double mcts_est = ptr->leaves[hash]->win_rate();
	double mcts_est = child->win_rate();

	double uct =
		getFloat(PARAM_FLOAT_UCT_WEIGHT)
		* sqrt(log((int)parent->traversals) / (child->traversals + 1));
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

static inline maybe_nodeptr max_utc(board *state, mcts_node *ptr) {
	double cur_max = 0;
	maybe_nodeptr ret = {};

	for (const auto& leaf : ptr->leaves_alive) {
		double temp = uct(state, ptr, leaf);

		if (temp > cur_max) {
			cur_max = temp;
			ret = leaf;
		}
	}

	return ret;
}


maybe_nodeptr uct_tree_policy(board *state, mcts_node *ptr) {
	while (ptr) {
		if (!ptr->can_traverse(state)) {
			return ptr;
		}

		maybe_nodeptr next = max_utc(state, ptr);

		if (!next) {
			return ptr;

		} else {
			state->make_move((*next)->coord);
			ptr = *next;
		}
	}

	return {};
}

// namespace bdg
}
