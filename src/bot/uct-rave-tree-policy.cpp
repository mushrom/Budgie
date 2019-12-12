#include <budgie/mcts.hpp>
#include <math.h>

namespace mcts_thing {

mcts_node* uct_rave_tree_policy::search(board *state, mcts_node *ptr) {
	while (ptr) {
		// TODO: keep a list of available moves in the game class, could be a
		//       major speedup and would allow removing the different code paths
		//       for fully visited nodes
		//
		//       this would also improve non-rave policies, since they don't
		//       have exploration after transistioning into a fully visited node.
		//       All valild moves should be considered, not just the random nodes
		//       picked in the "exploration" phase...
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

	//for (auto& x : ptr->leaves) {
	// XXX: need rave here because we're basically using rave for exploration,
	//      we should keep a list of available moves in the game class now
	//      that we have working group tracking, that way we don't need to rely
	//      on either 'rave' or 'leaves' having all the moves to traverse.
	//
	//      This was commented out for the above originally because in
	//      distributed mode rave nodes won't be sent, so this technically
	//      breaks that, although that was already broken with uct-rave anyway.
	//      Now it's just more broken.
#if 0
	for (auto& x : ptr->rave) {
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

double uct_rave_tree_policy::uct(const coordinate& coord, board *state, mcts_node *ptr) {
	if (!state->is_valid_move(coord)) {
		return 0;
	}

	double weight = patterns->search(state, coord) / 100.0;

	if (weight == 0) {
		return 0;
	}

	double rave_est = ptr->rave[coord].win_rate();
	double mcts_est = ptr->nodestats[coord].win_rate();
	double crit_est = (*ptr->criticality)[coord].win_rate();

	//double uct = uct_weight * sqrt(log(ptr->traversals) / ptr->nodestats[coord].traversals);


	double uct =
		uct_weight * sqrt(log(ptr->traversals) /
			((ptr->nodestats[coord].traversals > 0)
				? ptr->nodestats[coord].traversals
				: 1));
	//printf("uct weight: %g\n", uct);

	double mc_uct_est = mcts_est + uct;

	double B = (ptr->nodestats[coord].traversals)/rave_weight;
	//double B = (ptr->traversals)/rave_weight;
	B = (B > 1)? 1 : B;

#if 0
	double stats_weight = 0;

	// TODO: do we really need a threshold for crit_est? if so, why?
	if (ptr->traversals >= 100) {
	//if ((*ptr->criticality)[coord].traversals >= 100) {
		stats_weight = (crit_est + rave_est) * (1-B);

	} else {
		stats_weight = rave_est * (1-B);
	}

	stats_weight = crit_est + rave_est*(1-B);
#else
#endif

	//meh = (crit_est*(1-B) + rave_est*(1-B)) / 2;

	// weighted sum to prefer rave estimations initially, then later
	// prefer uct+mcts playout rates
	//double foo = (rave_est * (1-B)) + (mc_uct_est * B);
	//double foo = (crit_est * (1-B)) + (rave_est * (1-B)) + (mc_uct_est * B);
	//double foo = stats_weight + (mc_uct_est * B);
	//double foo = mcts_est*B + rave_ + mcts_est*B + uct;
	double foo = mcts_est*B + (crit_est+rave_est)*(1-B) + uct;
	//printf("returning %g for node with %u traversals (rave est: %g, mc est: %g)\n", foo, ptr->nodestats[coord].traversals, rave_est, mc_uct_est);
	return foo;
}

// namespace mcts_thing
}
