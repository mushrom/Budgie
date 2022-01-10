#include <budgie/mcts.hpp>
#include <math.h>
#include <budgie/parameters.hpp>
#include <budgie/pattern_db.hpp>
#include <cassert>

namespace mcts_thing::policies {

// TODO: utility header
#define MAX(A, B) (((A)>(B))? (A) : (B))
#define MIN(A, B) (((A)<(B))? (A) : (B))

static inline double uct(board *state, mcts_node *parent, mcts_node *child) {
	unsigned hash = coord_hash_v2(child->coord);

	/*
	if (get_pattern_db().search(state, child->coord) == 0) {
		return 0;
	}
	*/

	double rave_est = (*parent->rave)[hash].win_rate();
	double mcts_est = child->win_rate();
	double crit_est = 0;

	if (getBool(PARAM_BOOL_USE_CRITICALITY)) {
		double crit_est = (*parent->criticality)[hash].win_rate();
	}

	double uct =
		getFloat(PARAM_FLOAT_UCT_WEIGHT)
		// TODO: adding prior value traversals to real traversals
		//       probably shouldn't be but mimicing the behavior of the original
		//       for testing
		* sqrt(log((int)parent->traversals) / (child->traversals + 1));

	float expected = 0;
#if 0
	if (getBool(PARAM_BOOL_USE_EXP_SCORE)) {
		unsigned count = ptr->score_counts[hash];

		if (count > 0) {
			expected = ((ptr->color == point::color::White)? -1 : 1)
				* (ptr->expected_score[hash] / count);
			expected = MIN(20, MAX(-20, expected)) / 20.f;
			expected *= getFloat(PARAM_FLOAT_SCORE_WEIGHT);

			// TODO: configurable values here
			//expected = MIN(20, MAX(0, expected)) / 20.f;
			//expected = MIN(40, MAX(0, expected)) / 40.f;
			//expected = MIN(40, MAX(-40, expected)) / 40.f;
		}
	}
#endif

	// from the pachi paper
	float simsrave = (*parent->rave)[hash].traversals;
	float sims     = child->traversals + child->prior_traversals + 1;
	float B = simsrave / (simsrave + sims + simsrave*(sims/getInt(PARAM_INT_RAVE_WEIGHT)));
	//float B = sqrt(getInt(PARAM_INT_RAVE_WEIGHT) / (simsrave + getInt(PARAM_INT_RAVE_WEIGHT)));

	return (expected*0.05 + mcts_est)*(1.0-B)
		+ (crit_est+rave_est)*B
		+ uct;
}


static inline maybe_nodeptr max_utc(board *state, mcts_node *ptr) {
	double cur_max = 0;
	maybe_nodeptr ret = {};

	//for (const auto& x : state->available_moves) {
	for (mcts_node *leaf : ptr->leaves_alive) {
		//const coordinate& x = leaf->coord;
		double temp = uct(state, ptr, leaf);

		if (temp > cur_max) {
			cur_max = temp;
			ret = leaf;
		}
	}

	return ret;
}

maybe_nodeptr uct_rave_tree_policy(board *state, mcts_node *ptr) {
	while (ptr) {
		if (!ptr->can_traverse(state)) {
			return ptr;
		}

		assert(ptr->rave != nullptr);

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

// namespace mcts_thing
}
