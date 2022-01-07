#include <budgie/mcts.hpp>
#include <math.h>
#include <budgie/parameters.hpp>
#include <budgie/pattern_db.hpp>

namespace mcts_thing::policies {

// TODO: utility header
#define MAX(A, B) (((A)>(B))? (A) : (B))
#define MIN(A, B) (((A)<(B))? (A) : (B))

static inline double uct(const coordinate& coord, board *state, mcts_node *ptr) {
	unsigned hash = coord_hash_v2(coord);

	/*
	if (!state->is_valid_move(coord) || !leaves[hash]) {
		return 0;
	}
	*/

	/*
	if (get_pattern_db().search(state, coord) == 0) {
		return 0;
	}
	*/

	double rave_est = ptr->rave[hash].win_rate();
	double mcts_est = ptr->leaves[hash]->win_rate();
	double crit_est = 0;

	if (getBool(PARAM_BOOL_USE_CRITICALITY)) {
		double crit_est = (*ptr->criticality)[hash].win_rate();
	}

	double uct =
		getFloat(PARAM_FLOAT_UCT_WEIGHT)
		// TODO: adding prior value traversals to real traversals
		//       probably shouldn't be but mimicing the behavior of the original
		//       for testing
		* sqrt(log((int)ptr->traversals) / (ptr->leaves[hash]->traversals + 1));

	/*
	double uct = uct_weight * sqrt(log(ptr->traversals) /
	                 ((ptr->nodestats[hash].traversals > 0)
	                     ? ptr->nodestats[hash].traversals
	                     : 1));
						 */

	float expected = 0;
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

	// from the pachi paper
	float simsrave = ptr->rave[hash].traversals;
	float sims     = ptr->leaves[hash]->traversals + 1;
	float B = simsrave / (simsrave + sims + simsrave*(sims/getInt(PARAM_INT_RAVE_WEIGHT)));
	//float B = sqrt(getInt(PARAM_INT_RAVE_WEIGHT) / (simsrave + getInt(PARAM_INT_RAVE_WEIGHT)));

	return (expected*0.05 + mcts_est)*(1.0-B)
		+ (crit_est+rave_est)*B
		+ uct;
}


static inline maybe_coord max_utc(board *state, mcts_node *ptr) {
	double cur_max = 0;
	maybe_coord ret = {};

	//for (const auto& x : state->available_moves) {
	for (const auto& leaf : ptr->leaves_alive) {
		const coordinate& x = leaf->coord;
		double temp = uct(x, state, ptr);

		if (temp > cur_max) {
			cur_max = temp;
			ret = x;
		}
	}

	return ret;
}

maybe_nodeptr uct_rave_tree_policy(board *state, mcts_node *ptr) {
	while (ptr) {
		if (!ptr->fully_visited(state) || !ptr->try_expanding(state)) {
			return ptr;
		}

		maybe_coord next = max_utc(state, ptr);

		if (!next) {
			/*
			ptr->update(state);
			return {};
			*/
			return ptr;

		} else {
			//ptr->new_node(state, *next, state->current_player);
			state->make_move(*next);
			ptr = ptr->leaves[coord_hash_v2(*next)];
		}
	}

	return {};
}

// namespace mcts_thing
}
