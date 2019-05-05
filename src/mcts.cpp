#include <mcts-gb/mcts.hpp>
#include <mcts-gb/pattern_db.hpp>
#include <random>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <assert.h>

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

/*
// keeping this here because we might use it later
auto random_choice(auto& x) {
	std::uniform_int_distribution<int> distribution(0, x.size() - 1);

	return x[distribution(generator)];
}
*/

coordinate mcts::do_search(board *state, unsigned playouts, bool use_patterns) {
	root->color = state->other_player(state->current_player);

	while (root->traversals < playouts) {
		board scratch(state);
		root->explore(&scratch, use_patterns);
	}

	fprintf(stderr, "# %u playouts\n", root->traversals);

	coordinate temp = {1, 1};
	root->dump_node_statistics(temp, state);

	std::cerr << "# predicted playout: ";
	root->dump_best_move_statistics(state);
	std::cerr << std::endl;

	return root->best_move();
}

double mcts::win_rate(coordinate& coord) {
	if (root->leaves[coord] != nullptr) {
		return root->leaves[coord]->win_rate();

	} else {
		return 0.5;
	}
}

void mcts_node::new_node(board *state, coordinate& coord) {
	if (leaves[coord] == nullptr) {
		// this node is unvisited, set up a new node
		leaves[coord] = nodeptr(new mcts_node(this, state->current_player));
	}
}

void mcts_node::explore(board *state, bool use_patterns)
{
	mcts_node* ptr = tree_search(state, use_patterns);
	ptr = ptr? ptr->local_weighted_playout(state, use_patterns) : ptr;
	ptr? ptr->random_playout(state, use_patterns) : ptr;
}

mcts_node* mcts_node::tree_search(board *state, bool use_patterns) {
	mcts_node *ptr = this;

	while (ptr) {
		if (!ptr->fully_visited(state)) {
			coordinate next = ptr->pick_random_leaf(state, use_patterns);

			// no valid moves from here, just return
			if (!state->is_valid_move(next)) {
				ptr->update(state);
				return nullptr;
			}

			ptr->new_node(state, next);
			return ptr->leaves[next].get();
		}

		coordinate next = ptr->max_utc(state, use_patterns);

		if (next == coordinate(0, 0)) {
			ptr->update(state);
			return nullptr;
		}

		state->make_move(next);
		ptr = ptr->leaves[next].get();

		/*
		// debugging output
		   printf("\e[1;1H");
		   state->print();
		   usleep(10000);
		   */
	}

	return nullptr;
}

mcts_node* mcts_node::random_playout(board *state, bool use_patterns) {
	/*
	mcts_node *ptr = this;

	while (ptr) {
		coordinate next = ptr->pick_random_leaf(state, use_patterns);

		// create a new rave map for this if the node is now fully visited
		if (ptr->fully_visited(state)) {
			ptr->rave = rave_map::ptr(new rave_map(ptr->rave.get()));
		}

		if (!state->is_valid_move(next)) {
			ptr->update(state->determine_winner());
			return nullptr;
		}

		ptr->new_node(state, next);

		state->make_move(next);
		ptr = ptr->leaves[next].get();
	}
	*/

	unsigned boardsquares = state->dimension * state->dimension;

	while (true) {
		coordinate next = {0, 0};
		std::bitset<384> map;

		// TODO: this is just a slightly different form of pick_random_leaf(),
		//       could make a more general function
		while (map.count() != boardsquares) {
			coordinate temp = random_coord(state);
			unsigned index = state->coord_to_index(temp);

			if (map[index]) continue;
			map[index] = true;

			if (!state->is_valid_move(temp)
			   || (use_patterns && patterns.search(state, temp) == 0))
			{
				continue;
			}

			next = temp;
			break;
		}

		if (next == coordinate(0, 0)) {
			update(state);
			return nullptr;
		}

		state->make_move(next);
	}

	return nullptr;
}

mcts_node* mcts_node::weighted_playout(board *state, bool use_patterns) {
	unsigned boardsquares = state->dimension * state->dimension;

	while (true) {
		coordinate next = {0, 0};
		unsigned best = 0;

		for (unsigned i = 0; i < boardsquares/2; i++) {
			coordinate temp = random_coord(state);
			unsigned weight = patterns.search(state, temp);

			if (!weight || !state->is_valid_move(temp)) {
				continue;
			}

			if (weight > best) {
				next = temp;
				best = weight;
			}
		}

		/*
		for (unsigned y = 1; y < state->dimension; y++) {
			for (unsigned x = 1; x < state->dimension; x++) {
				//coordinate temp = random_coord(state);
				coordinate temp = {x, y};
				unsigned weight = patterns.search(state, temp);

				if (!weight || !state->is_valid_move(temp)) {
					continue;
				}

				if (weight > best) {
					next = temp;
					best = weight;
				}
			}
		}
		*/

		if (best == 0) {
			//update(state);
			// return a valid point to signal that we should continue to random playouts
			return this;
		}

		state->make_move(next);
	}

	return nullptr;
}

mcts_node* mcts_node::local_weighted_playout(board *state, bool use_patterns) {
	unsigned boardsquares = state->dimension * state->dimension;

	while (true) {
		coordinate next = {0, 0};
		std::bitset<384> map;

		unsigned best = 100;

		for (int y = -1; y <= 1; y++) {
			for (int x = -1; x <= 1; x++) {
				coordinate foo = {
					state->last_move.first  + x,
					state->last_move.second + y
				};

				unsigned weight = patterns.search(state, foo);

				if (weight > best && state->is_valid_move(foo)) {
					best = weight;
					next = foo;
					goto asdf;
				}
			}
		}

		// TODO: this is just a slightly different form of pick_random_leaf(),
		//       could make a more general function
		while (map.count() != boardsquares) {
			coordinate temp = random_coord(state);
			unsigned index = state->coord_to_index(temp);

			if (map[index]) continue;
			map[index] = true;

			if (!state->is_valid_move(temp)
			   || (use_patterns && patterns.search(state, temp) == 0))
			{
				continue;
			}

			next = temp;
			break;
		}

		if (next == coordinate(0, 0)) {
			update(state);
			return nullptr;
		}

asdf:
		state->make_move(next);
	}

	return nullptr;
}

coordinate mcts_node::pick_random_leaf(board *state, bool use_patterns) {
	coordinate ret = {0, 0};

	// TODO: might be able to just reuse the move map to find unvisited moves
	while (!fully_visited(state)) {
		coordinate temp = random_coord(state);

		if (map_get_coord(temp, state)) {
			continue;
		}

		map_set_coord(temp, state);

		if (!state->is_valid_move(temp)
		    || (use_patterns && patterns.search(state, temp) == 0))
		{
			continue;
		}

		ret = temp;
		break;
	}

	return ret;
}

void mcts_node::map_set_coord(coordinate& coord, board *state) {
	unsigned index = coord.second*state->dimension + coord.first;

	if (!move_map[index]) {
		move_map[index] = true;
		unique_traversed++;
	}
}

bool mcts_node::map_get_coord(coordinate& coord, board *state) {
	unsigned index = coord.second*state->dimension + coord.first;

	return move_map[index];
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

// low exploration constant with rave
#define MCTS_UCT_C       0.05
// how much to value rave estimations initially (in amount of playouts)
#define MCTS_RAVE_WEIGHT 1000.0

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

	double rave_est = ravemap[coord].win_rate();
	double mcts_est = leaves[coord]->win_rate();
	double uct = MCTS_UCT_C * sqrt(log(traversals) / leaves[coord]->traversals);
	//double mc_uct_est = weight*mcts_est + uct;
	double mc_uct_est = mcts_est + uct;

	double B = traversals/MCTS_RAVE_WEIGHT;
	B = (B > 1)? 1 : B;

	// weighted sum to prefer rave estimations initially, then later
	// prefer uct+mcts playout rates
	double foo = (rave_est * (1-B)) + (mc_uct_est * B);
	return foo;

	/*
	return weight*leaves[coord]->win_rate()
		+ MCTS_UCT_C * sqrt(log(traversals) / leaves[coord]->traversals);
		*/
}

void mcts_node::update_rave(board *state, point::color winner) {
	for (move::moveptr foo = state->move_list; foo; foo = foo->previous) {
		for (mcts_node *ptr = this; ptr; ptr = ptr->parent) {
			// node rave maps track the best moves for the oppenent
			if (ptr->color == foo->color) {
				continue;
			}

			bool won = foo->color == winner;

			ptr->ravemap[foo->coord].wins += won;
			ptr->ravemap[foo->coord].traversals++;
		}
	}
}

void mcts_node::update(board *state) {
	point::color winner = state->determine_winner();

	update_rave(state, winner);

	for (mcts_node *ptr = this; ptr; ptr = ptr->parent) {
		bool won = ptr->color == winner;

		ptr->traversals++;
		ptr->wins += won;
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

	/*
	if (depth >= 4) {
		return;
	}
	*/

	print_spaces();

	fprintf(stderr, "%s: %s coord (%u, %u), winrate: %g, traversals: %u\n",
		(color == point::color::Black)? "black" : "white",
		fully_visited(state)? "fully visited" : "visited",
		coord.first, coord.second, win_rate(), traversals);

	for (auto& x : leaves) {
		if (x.second == nullptr || !x.second->fully_visited(state)) {
			continue;
		}

		x.second->dump_node_statistics(x.first, state, depth + 1);
	}
}

// defined in gtp.cpp
// TODO: maybe move this to a utility function
std::string coord_string(coordinate& coord);

void mcts_node::dump_best_move_statistics(board *state) {
	coordinate coord = best_move();

	if (coord == coordinate(0, 0)) {
		return;
	}

	std::cerr << coord_string(coord);
	std::cerr << ((color == point::color::Black)? " (W)" : " (B)");

	if (fully_visited(state) && leaves[coord]) {
		//std::cerr << " => ";
		std::cerr << ", ";
		leaves[coord]->dump_best_move_statistics(state);
	}
}

// namespace mcts_thing
}
