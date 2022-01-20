#include <budgie/move_queue.hpp>
#include <budgie/mcts.hpp>

namespace bdg {

void move_queue::add(const coordinate& coord, uint16_t weight) {
	uint32_t hash = coord_hash_v2(coord);

	// override previous add
	if (contained[hash]) {
		total_weights -= weights[hash];
		total_weights += weight;
		weights[hash] = weight;

	} else {
		weights[hash] = weight;
		contained[hash] = true;
		coords[num_coords++] = hash;
		total_weights += weight;
	}
}

void move_queue::remove(const coordinate& coord) {
	// zero weight ensures coord is never choosen, effectively removing it
	add(coord, 0);
}

bool move_queue::contains(const coordinate& coord) {
	return contained[coord_hash_v2(coord)];
}

coordinate move_queue::choose(board *state) {
	std::uniform_int_distribution<unsigned> foundindex(0, total_weights-1);
	unsigned choice = foundindex(state->randomgen);
	uint32_t sum = 0;

	for (unsigned i = 0; i < num_coords; i++) {
		unsigned u = coords[i];
		sum += weights[u];

		if (choice < sum) {
			// TODO: unhash function
			return {u >> 5, u & ((1<<5)-1)};
		}
	}

	// should never get here
	return {0, 0};
}

// namespace bdg
}
