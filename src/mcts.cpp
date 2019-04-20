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

coordinate mcts::do_search(board *state, unsigned playouts) {
	// fully visit root node
	for (auto& x : state->available_moves()) {
		//root->leaves[x].parent = root;
		//root->leaves[x].color = state->current_player;
		root->explore(x, state);
		//fprintf(stderr, "(%d, %d) : %g\n", x.first, x.second, root->leaves[x].win_rate());
	}

	while (root->traversals < playouts) {
		coordinate coord = root->max_utc(state);
		root->explore(coord, state);
	}

	fprintf(stderr, "# %u playouts\n", root->traversals);

	return root->best_move();
}

double mcts::win_rate(coordinate& coord) {
	return root->leaves[coord].win_rate();
}

std::vector<coordinate> mcts_node::random_move_selector(board *state, unsigned n) {
	std::vector<coordinate> ret = {};

	for (unsigned k = 0; k < n; k++) {
		coordinate coord;

		ret.push_back(coord);
	}

	return ret;
}

std::vector<coordinate> mcts_node::pattern_move_selector(board *state, unsigned n) {
	// TODO: could we make an iterator for available moves?
	std::vector<std::pair<coordinate, unsigned>> weighted_moves = {};

	for (unsigned y = 1; y <= state->dimension; y++) {
		for (unsigned x = 1; x <= state->dimension; x++) {
			coordinate coord = {x, y};

			if (state->is_valid_move(coord)) {
				unsigned weight = patterns.search(state, coord);

				if (weight != 0) {
					weighted_moves.push_back({coord, weight});
				}
			}
		}
	}

	std::shuffle(weighted_moves.begin(), weighted_moves.end(), generator);
	std::sort(weighted_moves.begin(), weighted_moves.end(),
		[](auto& a, auto& b) {
			return a.second > b.second;
		});

	std::vector<coordinate> ret = {};

	for (auto& x : weighted_moves) {
		ret.push_back(x.first);
	}

	return ret;
}

void mcts_node::explore(coordinate& coord, board *state)
{
	leaves[coord].parent = this;
	leaves[coord].color = state->current_player;

	if (!state->is_valid_move(coord)) {
		// should only get here from the root level, so using Invalid
		// as a color ensures that neither player gets "free wins" from
		// illegal moves
		update(point::color::Invalid);
		return;
	}

	if (state->moves >= 2*state->dimension*state->dimension)
	{
		update(state->determine_winner());
		return;
	}

	board foo(state);
	foo.make_move(coord);

	if (leaves[coord].fully_visited(state)) {
		//fprintf(stderr, "# fully visited node at (%u, %u) : (%u)\n", coord.first, coord.second, leaves[coord].traversals);
		coordinate next = leaves[coord].max_utc(&foo);
		leaves[coord].explore(next, &foo);

	} else {
		coordinate next = {0, 0};

		for (unsigned j = 0; j < 2*foo.dimension*foo.dimension; j++) {
			coordinate temp = random_coord(state);

			if (!foo.is_valid_move(temp)) {
				// allocate dummy node
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

		leaves[coord].explore(next, &foo);
	}

	//unsigned branching = 1;
	// TODO: asdf
	//std::vector<coordinate> moves = selector(&foo, 1);

	//printf("\e[1;1H");
	//foo.print();
	//usleep(10000);


	//leaves[coord].explore(moves[0], &foo);
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

coordinate mcts_node::max_utc(board *state) {
	double cur_max = 0;
	coordinate ret = {0, 0};

	for (auto& x : state->available_moves()) {
		double temp = uct(x, state);

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

double mcts_node::uct(const coordinate& coord, board *state) {
	if (leaves[coord].traversals == 0) {
		return 1;
	}

	//double weight = patterns.search(state, coord) / 10.0;
	double weight = 1;

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
