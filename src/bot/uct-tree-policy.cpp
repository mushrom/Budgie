#include <budgie/mcts.hpp>
#include <math.h>

namespace mcts_thing {

mcts_node* uct_tree_policy::search(board *state, mcts_node *ptr) {
	while (ptr) {
		if (!ptr->fully_visited(state)) {
			coordinate next = pick_random_leaf(state);

			// no valid moves from here, just return
			if (!state->is_valid_move(next)) {
				ptr->update(state);
				return nullptr;
			}

			ptr->new_node(next, state->current_player);
			return ptr->leaves[next].get();
		}

		coordinate next = max_utc(state, ptr);

		if (next == coordinate(0, 0)) {
			ptr->update(state);
			return nullptr;
		}

		ptr->new_node(next, state->current_player);
		state->make_move(next);
		ptr = ptr->leaves[next].get();
	}

	return nullptr;
}


coordinate uct_tree_policy::max_utc(board *state, mcts_node *ptr) {
	double cur_max = 0;
	coordinate ret = {0, 0};

#if 0
	for (auto& x : ptr->leaves) {
		double temp = uct(x.first, state, ptr);

		if (temp > cur_max) {
			cur_max = temp;
			ret = x.first;
		}
	}
#else
	for (const auto& x : state->available_moves) {
		double temp = uct(x, state, ptr);

		if (temp > cur_max) {
			cur_max = temp;
			ret = x;
		}
	}

#endif

	return ret;
}

double uct_tree_policy::uct(const coordinate& coord, board *state, mcts_node *ptr) {
	if (!state->is_valid_move(coord)) {
		return 0;
	}

	double weight = patterns->search(state, coord) / 100.0;

	if (weight == 0) {
		return 0;
	}

	/*
	if (ptr->nodestats[coord].traversals == 0) {
		// unexplored leaf
		return 0.5;
	}
	*/

	double mcts_est = ptr->nodestats[coord].win_rate();

	/*
	double uct = uct_weight
		* sqrt(log(ptr->traversals) / ptr->nodestats[coord].traversals);
		* */
	double uct =
		uct_weight * sqrt(log(ptr->traversals) /
			((ptr->nodestats[coord].traversals > 0)
				? ptr->nodestats[coord].traversals
				: 1));


	double mc_uct_est = mcts_est + uct;

	return mc_uct_est;
}

// namespace mcts_thing
}
