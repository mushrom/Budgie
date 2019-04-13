#include <mcts-gb/mcts.hpp>
#include <random>
#include <chrono>
#include <unistd.h>

namespace mcts_thing {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);

// TODO: Maybe move this
coordinate random_coord(board *b) {
	std::uniform_int_distribution<int> distribution(1, b->dimension);

	return coordinate(distribution(generator), distribution(generator));
}

void mcts_node::explore(coordinate coord, board *state,
                        unsigned playouts,
                        unsigned depth,
                        unsigned branching)
{
	// TODO: heavy playouts
	// TODO: more sophisticated searching here

	board foo(*state);
	foo.make_move(coord);
	//foo.print();
	//usleep(100000);

	if (foo.moves >= (foo.dimension * foo.dimension)) {
		// TODO: scoring
		//foo.print();
		update(coord, foo.determine_winner());

		// asdfasdf
		return;
	}

	unsigned iters = (depth > 0 && playouts > 1)? branching : 1; 

	for (unsigned i = 0; i < iters; i++) {
		coordinate coord = random_coord(&foo);

		// TODO: this will loop forever, need to test for end of game
		for (unsigned i = 0; (!foo.is_valid_move(coord) || foo.is_suicide(coord, foo.current_player)) && i < 50; i++) {
			coord = random_coord(&foo);
		}

		if (!foo.is_valid_move(coord) || foo.is_suicide(coord, foo.current_player)) {
			// TODO: scoring
			//foo.print();
			// XXX:  penalize illegal/suicide moves, most definitely a better way...
			if (foo.moves < (foo.dimension * foo.dimension)) {
				update(coord, foo.other_player(foo.current_player));
				continue;

			} else {
				update(coord, foo.determine_winner());
			}
			return;
		}

		//leaves[coord] = mcts_node(this, foo.current_player);
		leaves[coord].parent = this;
		leaves[coord].color  = foo.current_player;
		leaves[coord].explore(coord, &foo, playouts / branching, depth - (depth > 0), branching);
		//explore(coord, &foo);
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
		double weight = (double)it->second.wins / (double)it->second.wins;
		double cur = (double)max->second.wins / (double)max->second.wins;

		if (weight > cur) {
			max = it;
		}
	}

	// TODO: actually find the move with the highest win rate
	return max->first;
}

void mcts_node::update(coordinate& coord, point::color winner) {
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
		parent->update(coord, winner);
	}
}

// namespace mcts_thing
}
