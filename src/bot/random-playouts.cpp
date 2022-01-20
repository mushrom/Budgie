#include <budgie/mcts.hpp>
#include <math.h>

namespace bdg::playouts {

void random_playout(board *state, move_queue& queue) {
	coordinate coord = pick_random_leaf(state, &get_pattern_db());

	if (coord.first != 0) {
		queue.add(coord, 100);
	}
}

// namespace bdg
}
