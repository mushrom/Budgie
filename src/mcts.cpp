#include <mcts-gb/mcts.hpp>
#include <mcts-gb/pattern_db.hpp>
#include <random>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <unistd.h>

#define MIN(a, b) ((a < b)? a : b)

namespace mcts_thing {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	pattern_db patterns("patterns.txt");

// TODO: Maybe move this
coordinate random_coord(board *b) {
	std::uniform_int_distribution<int> distribution(1, b->dimension);

	return coordinate(distribution(generator), distribution(generator));
}

auto random_choice(auto& c) {
	std::uniform_int_distribution<int> distribution(0, c.size());

	return c[distribution(generator)];
}

void mcts_node::explore(board *state,
                        unsigned playouts,
                        unsigned branching)
{
	// TODO: heavy playouts
	// TODO: more sophisticated searching here

	unsigned iters = (playouts > 1 && branching > 1)? MIN(playouts, branching) : 1;
	std::vector<coordinate> moves = state->available_moves();

	if (state->moves >= 2*state->dimension*state->dimension || moves.size() == 0) {
		update(state->determine_winner());
		return;
	}

	std::vector<std::pair<coordinate, unsigned>> weighted_moves = {};

	for (coordinate& thing : moves) {
		weighted_moves.push_back({thing, patterns.search(state, thing)});
	}

	// TODO: pattern weights, sort by weights
	//std::shuffle(moves.begin(), moves.end(), generator);
	std::sort(weighted_moves.begin(), weighted_moves.end(),
		[](auto& a, auto& b) {
			return a.second > b.second;
		});

	for (unsigned i = 0; i < iters && i < moves.size() - 1; i++) {
		board foo(state);
		coordinate coord = weighted_moves[i].first;

		leaves[coord].parent = this;
		leaves[coord].color  = foo.current_player;

		foo.make_move(coord);
		//foo.print();
		leaves[coord].explore(&foo, playouts / iters, branching);
	}
}

void mcts_node::exploit(board *state, unsigned moves, unsigned depth) {
	if (depth == 0 || leaves.begin() == leaves.end()) {
		return;
	}

	std::vector<std::pair<coordinate, mcts_node>> vec(leaves.begin(), leaves.end());

	std::sort(vec.begin(), vec.end(),
		[](auto& a, auto& b){
			return a.second.win_rate() > b.second.win_rate();
		});

	// trim less-useful search trees
	for (auto it = vec.begin() + moves; it < vec.end(); it++) {
		leaves.erase(it->first);
	}

	for (auto it = vec.begin(); it < vec.begin() + moves; it++) {
		std::cerr << "# trying out move "
		          << "(" << it->first.first << "," << it->first.second << ")"
		          << ", win rate: " << it->second.win_rate() << std::endl;

		leaves[it->first].explore(state, 45 * depth);
		leaves[it->first].exploit(state, moves, depth - 1);

		std::cerr << "# adjusted win rate: " << leaves[it->first].win_rate() << std::endl;
		std::cerr << "# traversals: " << leaves[it->first].traversals << std::endl;
	}
}

coordinate mcts_node::best_move(void) {
	if (leaves.begin() == leaves.end()) {
		fprintf(stderr, "unexplored!\n");
		return {0, 0}; // no more nodes, return invalid coordinate
	}

	auto it = leaves.begin();
	auto max = leaves.begin();

	for (; it != leaves.end(); it++) {
		double weight = (double)it->second.wins / (double)it->second.traversals;
		double cur = (double)max->second.wins / (double)max->second.traversals;

		if (weight > cur) {
			max = it;
		}
	}

	double cur = (double)max->second.wins / (double)max->second.traversals;
	/*
	printf("# estimated win rate: %g (%u/%u) at (%u, %u)\n",
	       cur, max->second.wins, max->second.traversals,
	       max->first.first, max->first.second);
		   */

	return max->first;
}

double mcts_node::win_rate(void){
	return (traversals > 0)?  (double)wins / (double)traversals : 0;
}

void mcts_node::update(point::color winner) {
	//leaves[coord].traversals++;
	//leaves[coord].wins += this->color == winner;
	traversals++;
	wins += color == winner;

	/*
	if (traversals < 1000 || parent == nullptr) {
		printf("(%u) %u/%u   \n", winner, traversals, wins);
	}
	*/

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
