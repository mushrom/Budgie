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
	// XXX: we'll want to check to make sure this is already the case once we
	//      start reusing trees
	root->color = state->other_player(state->current_player);

	while (root->traversals < playouts) {
		board scratch(state);
		explore(&scratch);
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

void mcts::explore(board *state)
{
	mcts_node* ptr = tree_search(state, root);
	//ptr = ptr? ptr->local_weighted_playout(state, use_patterns) : ptr;
	ptr = ptr? random_playout(state, ptr) : ptr;
}

mcts_node* mcts::tree_search(board *state, mcts_node *ptr) {
	while (ptr) {
		if (!ptr->fully_visited(state)) {
			coordinate next = pick_random_leaf(state);

			// no valid moves from here, just return
			if (!state->is_valid_move(next)) {
				ptr->update(state);
				return nullptr;
			}

			ptr->new_node(state, next);
			return ptr->leaves[next].get();
		}

		coordinate next = max_utc(state, ptr);

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

coordinate mcts::max_utc(board *state, mcts_node *ptr) {
	double cur_max = 0;
	coordinate ret = {0, 0};

	for (auto& x : ptr->leaves) {
		/*
		if (x.second == nullptr) {
			continue;
		}
		*/

		double temp = uct(x.first, state, ptr);

		if (temp > cur_max) {
			cur_max = temp;
			ret = x.first;
		}
	}

	return ret;
}

// low exploration constant with rave
#define MCTS_UCT_C       0.05
// how much to value rave estimations initially (in amount of playouts)
#define MCTS_RAVE_WEIGHT 2000.0

double mcts::uct(const coordinate& coord, board *state, mcts_node *ptr) {
	if (!state->is_valid_move(coord)) {
		return 0;
	}

	double weight = patterns.search(state, coord) / 100.0;

	if (weight == 0) {
		return 0;
	}

	if (ptr->leaves[coord] == nullptr) {
		// unexplored leaf
		return 0.5;
	}

	double rave_est = (*ptr->rave)[coord].win_rate();
	double mcts_est = ptr->leaves[coord]->win_rate();
	double uct = MCTS_UCT_C
		* sqrt(log(ptr->traversals) / ptr->leaves[coord]->traversals);
	double mc_uct_est = mcts_est + uct;

	double B = (ptr->leaves[coord]->traversals)/MCTS_RAVE_WEIGHT;
	B = (B > 1)? 1 : B;

	// weighted sum to prefer rave estimations initially, then later
	// prefer uct+mcts playout rates
	double foo = (rave_est * (1-B)) + (mc_uct_est * B);
	return foo;
}

mcts_node* mcts::random_playout(board *state, mcts_node *ptr) {
	while (true) {
		coordinate next = pick_random_leaf(state);

		if (next == coordinate(0, 0)) {
			ptr->update(state);
			return nullptr;
		}

		state->make_move(next);
	}

	return nullptr;
}

coordinate mcts::local_best(board *state) {
	coordinate things[9];
	unsigned found = 0;
	// default weight is 100, so look for anything better than random
	unsigned best = 101;

	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			coordinate foo = {
				state->last_move.first  + x,
				state->last_move.second + y
			};

			if (foo.first == 0 && foo.second == 0) {
				continue;
			}

			unsigned weight = patterns.search(state, foo);

			if (weight == 0 || !state->is_valid_move(foo)) {
				continue;
			}

			if (weight > best && state->is_valid_move(foo)) {
				best = weight;
				found = 0;
				things[found++] = foo;
			}

			else if (weight == best) {
				things[found++] = foo;
			}
		}
	}

	if (found > 0) {
		return things[rand() % found];
	}

	return coordinate(0, 0);
}

mcts_node* mcts::local_weighted_playout(board *state, mcts_node *ptr) {
	while (true) {
		coordinate next = {0, 0};
		std::bitset<384> map;

		next = local_best(state);

		if (next != coordinate(0, 0)) {
			goto asdf;
		}

		next = pick_random_leaf(state);

		if (next == coordinate(0, 0)) {
			ptr->update(state);
			return nullptr;
		}

// TODO: asdf
asdf:
		/*
		// debugging output
		   printf("\e[1;1H");
		   state->print();
		   usleep(100000);
		   */

		state->make_move(next);
	}

	return nullptr;
}

coordinate mcts::pick_random_leaf(board *state) {
	coordinate ret = {0, 0};
	unsigned boardsquares = state->dimension * state->dimension;
	std::bitset<384> map;

	while (map.count() != boardsquares) {
		coordinate temp = random_coord(state);
		unsigned index = state->coord_to_index(temp);

		if (map[index]) continue;
		map[index] = true;

		if (!state->is_valid_move(temp) || patterns.search(state, temp) == 0) {
			continue;
		}

		ret = temp;
		break;
	}

	return ret;
}


void mcts_node::new_node(board *state, coordinate& coord) {
	// TODO: experimented with sharing rave stats between siblings but it seems to
	//       be hit-or-miss, leaving the code here for now...
	if (leaves[coord] == nullptr) {
		// this node is unvisited, set up a new node
		leaves[coord] = nodeptr(new mcts_node(this, state->current_player));
		//leaves[coord]->rave = child_rave;
		//leaves[coord]->child_rave = raveptr(new ravestats);
		leaves[coord]->rave = raveptr(new ravestats);
	}
}

bool mcts_node::fully_visited(board *state) {
	return traversals >= state->dimension * state->dimension;
	//return traversals > state->dimension * 2;
	//return traversals > 2;
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

double mcts_node::win_rate(void){
	return (double)wins / (double)traversals;
}

void mcts_node::update_rave(board *state, point::color winner) {
	for (move::moveptr foo = state->move_list; foo; foo = foo->previous) {
		for (mcts_node *ptr = this; ptr; ptr = ptr->parent) {
			// node rave maps track the best moves for the oppenent
			if (ptr->color == foo->color) {
				continue;
			}

			bool won = foo->color == winner;

			(*ptr->rave)[foo->coord].wins += won;
			(*ptr->rave)[foo->coord].traversals++;
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

	fprintf(stderr, "%s: %s coord (%u, %u), winrate: %g, rave: %g, traversals: %u\n",
		(color == point::color::Black)? "black" : "white",
		fully_visited(state)? "fully visited" : "visited",
		coord.first, coord.second,
		win_rate(),
		parent? (*parent->rave)[coord].win_rate() : 0, traversals);

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
