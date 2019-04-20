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
	if (root->leaves[coord] != nullptr) {
		return root->leaves[coord]->win_rate();

	} else {
		return 0;
	}
}

void mcts_node::explore(coordinate& coord, board *state, bool use_patterns)
{
	if (state->moves > 2*state->dimension*state->dimension)
	{
		update(state->determine_winner());
		return;
	}

	if (leaves[coord] == nullptr) {
		leaves[coord] = nodeptr(new mcts_node(this, state->current_player));

		map_set_coord(coord, state);
	}

	mcts_node *leaf = leaves[coord].get();

	board foo(state);
	foo.make_move(coord);

	/*
	printf("\e[1;1H");
	foo.print();
	usleep(10000);
	*/

	if (leaf->fully_visited(state)) {
		//fprintf(stderr, "# fully visited node at (%u, %u) : (%u)\n", coord.first, coord.second, leaf->traversals);
		coordinate next = leaf->max_utc(&foo, use_patterns);
		leaf->explore(next, &foo, use_patterns);

	} else {
		coordinate next = {0, 0};
		unsigned spaces = foo.dimension * foo.dimension;

		// TODO: can just reuse the move map to find unvisited moves
		while (!leaf->fully_visited(state)) {
			//fprintf(stderr, "%p: asdasdfasdfasdf, %u, %lu\n", leaf, leaf->unique_traversed, sizeof(mcts_node));
			coordinate temp = random_coord(state);
			unsigned index = temp.second*state->dimension + temp.first;

			if (leaf->map_get_coord(temp, state)) {
				continue;
			}

			leaf->map_set_coord(temp, state);

			if (!foo.is_valid_move(temp)) {
				continue;
			}

			next = temp;
			break;
		}

		if (!foo.is_valid_move(next)) {
			leaf->update(foo.determine_winner());
			return;
		}

		leaf->explore(next, &foo, use_patterns);
	}
}

void mcts_node::map_set(unsigned index) {
	unsigned top = index / 8;
	unsigned bottom = index % 8;

	move_map[top] |= (1 << bottom);
}

void mcts_node::map_set_coord(coordinate& coord, board *state) {
	unsigned index = coord.second*state->dimension + coord.first;

	if (!map_get(index)) {
		map_set(index);
		unique_traversed++;
	}
}

bool mcts_node::map_get(unsigned index) {
	unsigned top = index / 8;
	unsigned bottom = index % 8;

	return !!(move_map[top] & (1 << bottom));
}

bool mcts_node::map_get_coord(coordinate& coord, board *state) {
	unsigned index = coord.second*state->dimension + coord.first;

	return map_get(index);
}

bool mcts_node::fully_visited(board *state) {
	return unique_traversed >= state->dimension * state->dimension;
}

coordinate mcts_node::best_move(void) {
	if (leaves.begin() == leaves.end()) {
		fprintf(stderr, "unexplored!\n");
		return {0, 0}; // no more nodes, return invalid coordinate
	}

	auto it = leaves.begin();
	auto max = leaves.begin();

	for (; it != leaves.end(); it++) {
		if (it->second == nullptr) {
			continue;
		}

		if (it->second->traversals > max->second->traversals) {
			max = it;
		}
	}

	return max->first;
}

coordinate mcts_node::max_utc(board *state, bool use_patterns) {
	double cur_max = 0;
	coordinate ret = {0, 0};

	/*
	for (auto& x : state->available_moves()) {
		double temp = uct(x, state, use_patterns);

		if (temp > cur_max) {
			cur_max = temp;
			ret = x;
		}
	}
	*/

	for (auto& x : leaves) {
		if (x.second == nullptr) {
			continue;
		}

		double temp = uct(x.first, state, use_patterns);

		if (temp > cur_max) {
			cur_max = temp;
			ret = x.first;
		}
	}

	return ret;
}

double mcts_node::win_rate(void){
	return (double)wins / (double)traversals;
}

double mcts_node::uct(const coordinate& coord, board *state, bool use_patterns) {
	/*
	if (leaves[coord] == nullptr || leaves[coord]->traversals == 0) {
		return 1;
	}
	*/

	if (leaves[coord] == nullptr) {
		return 0;
	}

	double weight = use_patterns? patterns.search(state, coord) / 100.0 : 1;

	return weight*leaves[coord]->win_rate()
		+ MCTS_UCT_C * sqrt(log(traversals / leaves[coord]->traversals));
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
			if (thing.second != nullptr) {
				ret += thing.second->terminal_nodes();
				if (thing.second->parent != this) {
					fprintf(stderr, "got problems!\n");
				}
			}
		}

		return ret;
	}
}

// namespace mcts_thing
}
