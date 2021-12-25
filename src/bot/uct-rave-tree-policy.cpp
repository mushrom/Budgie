#include <budgie/mcts.hpp>
#include <math.h>
#include <budgie/parameters.hpp>

namespace mcts_thing {

mcts_node* uct_rave_tree_policy::search(board *state, mcts_node *ptr) {
	while (ptr) {
		if (!ptr->fully_visited(state)) {
			return ptr;
		}

		coordinate next = max_utc(state, ptr);

		if (next == coordinate(0, 0)) {
			ptr->update(state);
			return nullptr;
		}

		ptr->new_node(state, next, state->current_player);
		state->make_move(next);
		ptr = ptr->leaves[coord_hash_v2(next)];
	}

	return nullptr;
}


coordinate uct_rave_tree_policy::max_utc(board *state, mcts_node *ptr) {
	double cur_max = 0;
	coordinate ret = {0, 0};

	for (const auto& x : state->available_moves) {
		double temp = uct(x, state, ptr);

		if (temp > cur_max) {
			cur_max = temp;
			ret = x;
		}
	}

	return ret;
}

// TODO: utility header
#define MAX(A, B) (((A)>(B))? (A) : (B))
#define MIN(A, B) (((A)<(B))? (A) : (B))

double uct_rave_tree_policy::uct(const coordinate& coord, board *state, mcts_node *ptr) {
	if (!state->is_valid_move(coord)) {
		return 0;
	}

	unsigned weight = patterns->search(state, coord) / 100.0;

	if (weight == 0) {
		return 0;
	}

	unsigned hash = coord_hash_v2(coord);
	double rave_est = ptr->rave[hash].win_rate();
	double mcts_est = ptr->nodestats[hash].win_rate();
	double crit_est = (*ptr->criticality)[hash].win_rate();

	double uct =
		getFloat(PARAM_FLOAT_UCT_WEIGHT)
		* sqrt(log((int)ptr->traversals) / ptr->nodestats[hash].traversals);

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
	float sims     = ptr->nodestats[hash].traversals;
	float B = simsrave / (simsrave + sims + simsrave*(sims/getInt(PARAM_INT_RAVE_WEIGHT)));

	return (expected*0.05 + mcts_est)*(1.0-B)
		+ (crit_est+rave_est)*B
		+ uct;
}

// namespace mcts_thing
}
