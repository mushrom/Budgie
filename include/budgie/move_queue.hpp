#pragma once

#include <budgie/game.hpp>

#include <vector>
#include <unordered_set>
#include <string.h>
#include <stdint.h>

namespace bdg {

struct move_queue {
	move_queue() {
		contained = 0;
		total_weights = 0;
		num_coords = 0;
	}

	void add(const coordinate& coord, uint16_t weight);
	void remove(const coordinate& coord);
	bool contains(const coordinate& coord);
	coordinate choose(board *state);

	unsigned coords[660];
	unsigned num_coords;

	uint16_t weights[660];
	uint32_t total_weights;
	std::bitset<660> contained;
};

// namespace bdg
}
