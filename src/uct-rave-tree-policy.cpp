#include <mcts-gb/mcts.hpp>
#include <math.h>

namespace mcts_thing {

mcts_node* uct_rave_tree_policy::search(board *state, mcts_node *ptr) {
	while (ptr) {
		if (!ptr->fully_visited(state)) {
			coordinate next = pick_random_leaf(state);

			// no valid moves from here, just return
			if (!state->is_valid_move(next)) {
				ptr->update(state);
				return nullptr;
			}

			ptr->new_node(state, next);
			return ptr->leaves[next].get();
		}

		coordinate next = max_utc(state, ptr);

		if (next == coordinate(0, 0)) {
			ptr->update(state);
			return nullptr;
		}

		state->make_move(next);
		ptr = ptr->leaves[next].get();

		/*
		// debugging output
		   printf("\e[1;1H");
		   state->print();
		   usleep(10000);
		   */
	}

	return nullptr;
}


coordinate uct_rave_tree_policy::max_utc(board *state, mcts_node *ptr) {
	double cur_max = 0;
	coordinate ret = {0, 0};

	for (auto& x : ptr->leaves) {
		/*
		if (x.second == nullptr) {
			continue;
		}
		*/

		double temp = uct(x.first, state, ptr);

		if (temp > cur_max) {
			cur_max = temp;
			ret = x.first;
		}
	}

	return ret;
}

double uct_rave_tree_policy::uct(const coordinate& coord, board *state, mcts_node *ptr) {
	if (!state->is_valid_move(coord)) {
		return 0;
	}

	double weight = patterns->search(state, coord) / 100.0;

	if (weight == 0) {
		return 0;
	}

	if (ptr->leaves[coord] == nullptr) {
		// unexplored leaf
		return 0.5;
	}

	double rave_est = (*ptr->rave)[coord].win_rate();
	double mcts_est = ptr->leaves[coord]->win_rate();
	double crit_est = (*ptr->criticality)[coord].win_rate();

	double uct = uct_weight
		* sqrt(log(ptr->traversals) / ptr->leaves[coord]->traversals);
	double mc_uct_est = mcts_est + uct;

	double B = (ptr->leaves[coord]->traversals)/rave_weight;
	//double B = (ptr->traversals / rave_weight);
	B = (B > 1)? 1 : B;

	double meh = 0;

	if (ptr->traversals >= 200) {
	//if ((*ptr->criticality)[coord].traversals >= 100) {
		//meh = crit_est*(1-B) + rave_est*(1-B);
		meh = (crit_est + rave_est) * (1-B);

	} else {
		meh = rave_est * (1-B);
	}

	//meh = (crit_est*(1-B) + rave_est*(1-B)) / 2;

	// weighted sum to prefer rave estimations initially, then later
	// prefer uct+mcts playout rates
	//double foo = (rave_est * (1-B)) + (mc_uct_est * B);
	//double foo = (crit_est * (1-B)) + (rave_est * (1-B)) + (mc_uct_est * B);
	double foo = meh + (mc_uct_est * B);
	return foo;
}

// namespace mcts_thing
}
