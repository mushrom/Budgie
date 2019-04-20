#include <mcts-gb/mcts.hpp>
#include <mcts-gb/pattern_db.hpp>
#include <random>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <unistd.h>

#define MIN(a, b) ((a < b)? a : b)
#define MAX(a, b) ((a > b)? a : b)

namespace mcts_thing {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	pattern_db patterns("patterns.txt");

// TODO: Maybe move this
coordinate random_coord(board *b) {
	std::uniform_int_distribution<int> distribution(1, b->dimension);

	return coordinate(distribution(generator), distribution(generator));
}

coordinate mcts::do_search(board *state, unsigned playouts, bool use_patterns) {
	// fully visit root node
	for (auto& x : state->available_moves()) {
		root->explore(x, state, use_patterns);
	}

	while (root->traversals < playouts) {
		coordinate coord = root->max_utc(state, use_patterns);
		root->explore(coord, state, use_patterns);
	}

	fprintf(stderr, "# %u playouts\n", root->traversals);

	return root->best_move();
}

double mcts::win_rate(coordinate& coord) {
	return root->leaves[coord].win_rate();
}

void mcts_node::explore(coordinate& coord, board *state, bool use_patterns)
{
	leaves[coord].parent = this;
	leaves[coord].color = state->current_player;
	if (state->moves > 2*state->dimension*state->dimension)
	{
		update(state->determine_winner());
		return;
	}

	board foo(state);
	foo.make_move(coord);

	/*
	printf("\e[1;1H");
	foo.print();
	usleep(10000);
	*/

	if (leaves[coord].fully_visited(state)) {
		//fprintf(stderr, "# fully visited node at (%u, %u) : (%u)\n", coord.first, coord.second, leaves[coord].traversals);
		coordinate next = leaves[coord].max_utc(&foo, use_patterns);
		leaves[coord].explore(next, &foo, use_patterns);

	} else {
		coordinate next = {0, 0};

		for (unsigned j = 0; j < 2*foo.dimension*foo.dimension; j++) {
			coordinate temp = random_coord(state);

			if (!foo.is_valid_move(temp)) {
				// allocate dummy node
				// TODO: smart pointers for leaf nodes
				leaves[coord].leaves[temp].parent = &leaves[coord];
				continue;
			}

			if (leaves[coord].leaves.find(temp) == leaves[coord].leaves.end()) {
				next = temp;
				break;
			}
		}

		if (!foo.is_valid_move(next)) {
			leaves[coord].update(foo.determine_winner());
			return;
		}

		leaves[coord].explore(next, &foo, use_patterns);
	}
}

bool mcts_node::fully_visited(board *state) {
	return leaves.size() == state->dimension * state->dimension;
}

coordinate mcts_node::best_move(void) {
	if (leaves.begin() == leaves.end()) {
		fprintf(stderr, "unexplored!\n");
		return {0, 0}; // no more nodes, return invalid coordinate
	}

	auto it = leaves.begin();
	auto max = leaves.begin();

	for (; it != leaves.end(); it++) {
		if (it->second.traversals > max->second.traversals) {
			max = it;
		}
	}

	return max->first;
}

coordinate mcts_node::max_utc(board *state, bool use_patterns) {
	double cur_max = 0;
	coordinate ret = {0, 0};

	for (auto& x : state->available_moves()) {
		double temp = uct(x, state, use_patterns);

		if (temp > cur_max) {
			cur_max = temp;
			ret = x;
		}
	}

	return ret;
}

double mcts_node::win_rate(void){
	return (traversals > 0)?  (double)wins / (double)traversals : 0;
}

double mcts_node::uct(const coordinate& coord, board *state, bool use_patterns) {
	if (leaves[coord].traversals == 0) {
		return 1;
	}

	double weight = use_patterns? patterns.search(state, coord) / 10.0
	                            : 1;

	return weight*leaves[coord].win_rate()
		+ MCTS_UCT_C * sqrt(log(traversals / leaves[coord].traversals));
}

void mcts_node::update(point::color winner) {
	traversals++;
	wins += color == winner;

	if (parent != nullptr) {
		parent->update(winner);
	}
}

unsigned mcts_node::terminal_nodes(void) {
	if (leaves.size() == 0) {
		return 1;

	} else {
		unsigned ret = 0;

		for (auto& thing : leaves) {
			ret += thing.second.terminal_nodes();
		}

		return ret;
	}
}

// namespace mcts_thing
}
