#include <mcts-gb/mcts.hpp>
#include <random>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <unistd.h>

namespace mcts_thing {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);

// TODO: Maybe move this
coordinate random_coord(board *b) {
	std::uniform_int_distribution<int> distribution(1, b->dimension);

	return coordinate(distribution(generator), distribution(generator));
}

void mcts_node::explore(board *state,
                        unsigned playouts,
                        unsigned branching)
{
	// TODO: heavy playouts
	// TODO: more sophisticated searching here

	unsigned iters = (playouts > 1 && branching > 1)? branching : 1; 

	for (unsigned i = 0; i < iters; i++) {
		board foo(*state);
		coordinate coord = random_coord(&foo);

		// TODO: this will loop forever, need to test for end of game
		for (unsigned i = 0;
		    (!foo.is_valid_move(coord)
		       || leaves.find(coord) != leaves.end()
		       || foo.is_suicide(coord, foo.current_player))
		    && i < 2*foo.dimension*foo.dimension; i++)
		{
			coord = random_coord(&foo);
		}

		if (!foo.is_valid_move(coord) || foo.is_suicide(coord, foo.current_player)) {
			// TODO: scoring
			//foo.print();
			// XXX:  penalize illegal/suicide moves, most definitely a better way...
			/*
			if (foo.moves < (foo.dimension * foo.dimension)) {
				//update(coord, foo.other_player(foo.current_player));
				continue;
				//return;

			} else {
			*/
				update(coord, foo.determine_winner());
				return;
			//}
		}

		/*
		if (foo.moves >= 2*(foo.dimension * foo.dimension)) {
			// TODO: scoring
			//foo.print();
			update(coord, foo.determine_winner());

			// asdfasdf
			return;
		}
		*/

		unsigned temp = (branching > 2)? branching / 2 : 1;

		leaves[coord].parent = this;
		leaves[coord].color  = foo.current_player;

		foo.make_move(coord);
		leaves[coord].explore(&foo, playouts / branching, temp );
	}
}

void mcts_node::exploit(board *state, unsigned moves) {
	if (moves < 1 || leaves.begin() == leaves.end()) {
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
		/*
		std::cout << "# trying out move "
		          << "(" << it->first.first << "," << it->first.second << ")"
		          << ", win rate: " << it->second.win_rate() << std::endl;
				  */

		leaves[it->first].explore(state, 30 * moves);
		leaves[it->first].exploit(state, moves / 2);

		/*
		std::cout << "# adjusted win rate: " << leaves[it->first].win_rate() << std::endl;
		std::cout << "# traversals: " << leaves[it->first].traversals << std::endl;
		*/
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
	return (double)wins / (double)traversals;
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
