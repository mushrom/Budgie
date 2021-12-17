#include <budgie/mcts.hpp>
#include <math.h>

namespace mcts_thing {

mcts_node* uct_rave_tree_policy::search(board *state, mcts_node *ptr) {
	while (ptr) {
		if (!ptr->fully_visited(state)) {
			coordinate next = pick_random_leaf(state, patterns.get());

			// no valid moves from here, just return
			if (!state->is_valid_move(next)) {
				ptr->update(state);
				return nullptr;
			}

			ptr->new_node(state, next, state->current_player);
			return ptr->leaves[coord_hash_v2(next)];
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

	double weight = patterns->search(state, coord) / 100.0;

	if (weight == 0) {
		return 0;
	}

	unsigned hash = coord_hash_v2(coord);
	double rave_est = ptr->rave[hash].win_rate();
	double mcts_est = ptr->nodestats[hash].win_rate();
	// TODO: criticality table
	double crit_est = (*ptr->criticality)[coord].win_rate();


	double uct = uct_weight * sqrt(log(ptr->traversals) /
	                 ((ptr->nodestats[hash].traversals > 0)
	                     ? ptr->nodestats[hash].traversals
	                     : 1));

	double B = MIN(1.0, (ptr->nodestats[hash].traversals)/rave_weight);

	return mcts_est*B + (crit_est+rave_est)*(1.0-B) + uct;
}

// namespace mcts_thing
}
