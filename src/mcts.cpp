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

		if (coord == coordinate(0, 0)) {
			return {0, 0};
		}

		root->explore(coord, state, use_patterns);
	}

	fprintf(stderr, "# %u playouts\n", root->traversals);

	coordinate temp = {1, 1};
	root->dump_node_statistics(temp, state);
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

		if (next == coordinate(0, 0)) {
			leaf->update(foo.determine_winner());
			return;
		}

		leaf->explore(next, &foo, use_patterns);

	} else {
		coordinate next = {0, 0};

		// TODO: can just reuse the move map to find unvisited moves
		while (!leaf->fully_visited(state)) {
			coordinate temp = random_coord(state);
			if (leaf->map_get_coord(temp, state)) {
				continue;
			}

			leaf->map_set_coord(temp, state);

			if (!foo.is_valid_move(temp)) {
				continue;
			}

			if (use_patterns && patterns.search(state, temp) == 0) {
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

void mcts_node::map_set_coord(coordinate& coord, board *state) {
	unsigned index = coord.second*state->dimension + coord.first;

	if (!move_map[index]) {
		move_map[index] = true;
		//map_set(index);
		unique_traversed++;
	}
}

bool mcts_node::map_get_coord(coordinate& coord, board *state) {
	unsigned index = coord.second*state->dimension + coord.first;

	return move_map[index];
	//return map_get(index);
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
	if (leaves[coord] == nullptr) {
		return 0;
	}

	if (!state->is_valid_move(coord)) {
		return 0;
	}

	double weight = use_patterns? patterns.search(state, coord) / 100.0 : 1;

	if (weight == 0) {
		return 0;
	}

	return weight*leaves[coord]->win_rate()
		+ MCTS_UCT_C * sqrt(log(traversals) / leaves[coord]->traversals);
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
			}
		}

		return ret;
	}
}

unsigned mcts_node::nodes(void){
	unsigned ret = 1;

	for (auto& thing : leaves) {
		if (thing.second != nullptr) {
			ret += thing.second->nodes();
		}
	}

	return ret;
}

void mcts_node::dump_node_statistics(const coordinate& coord, board *state, unsigned depth) {
	auto print_spaces = [&](){
		for (unsigned i = 0; i < depth*2; i++) {
			std::cerr << ' ';
		}
	};

	if (depth >= 2) {
		return;
	}

	print_spaces();

	fprintf(stderr, "%s coord (%u, %u), winrate: %g, traversals: %u\n",
		fully_visited(state)? "fully visited" : "visited",
		coord.first, coord.second, win_rate(), traversals);

	for (auto& x : leaves) {
		if (x.second == nullptr) {
			continue;
		}

		x.second->dump_node_statistics(x.first, state, depth + 1);
	}
}

// namespace mcts_thing
}
