#include <budgie/mcts.hpp>
#include <budgie/pattern_db.hpp>
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
	// XXX: get rid of this eventually
	pattern_db patterns("patterns.txt");

// TODO: Maybe move this
coordinate random_coord(board *b) {
	std::uniform_int_distribution<int> distribution(1, b->dimension);

	return coordinate(distribution(generator), distribution(generator));
}

// TODO: also maybe move this
coordinate pick_random_leaf(board *state) {
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

/*
// keeping this here because we might use it later
auto random_choice(auto& x) {
	std::uniform_int_distribution<int> distribution(0, x.size() - 1);

	return x[distribution(generator)];
}
*/

coordinate mcts::do_search(board *state, unsigned playouts) {
	// XXX: we'll want to check to make sure this is already the case once we
	//      start reusing trees
	root->color = state->other_player(state->current_player);

	while (root->traversals < playouts) {
		board scratch(state);
		explore(&scratch);
	}

	fprintf(stderr, "# %u playouts\n", root->traversals);

	/*
	// TODO: re-add this once we have a logger
	//       Actually, once we have a logger we'll probably also have an
	//       AI instance class so we should just put debugging stuff there.

	coordinate temp = {1, 1};
	root->dump_node_statistics(temp, state);

	std::cerr << "# predicted playout: ";
	root->dump_best_move_statistics(state);
	std::cerr << std::endl;
	*/

	return root->best_move();
}

double mcts::win_rate(coordinate& coord) {
	/*
	if (root->leaves[coord] != nullptr) {
		return root->leaves[coord]->win_rate();

	} else {
		return 0.5;
	}
	*/

	if (root->nodestats.find(coord) != root->nodestats.end()) {
		return root->nodestats[coord].win_rate();

	} else {
		return 0.5;
	}
}

void mcts::explore(board *state)
{
	mcts_node* ptr = tree->search(state, root);
	ptr = ptr? policy->playout(state, ptr) : ptr;
}

std::string mcts::serialize_node(const mcts_node* ptr, unsigned depth) {
	if (ptr == nullptr) {
		return "";
	}

	std::string color = (ptr->color == point::color::Black)? "black" : "white";
	std::string childs = "";

	for (const auto& x : ptr->leaves) {
		childs += serialize_node(x.second.get(), depth + 1);
	}

	return (std::string)
		"(node" +
		" (coordinate (0, 0))" +
		" (color " + color + ")" +
		" (leaves " + childs + ")) ";

	/*
	// XXX: pretty-printed, need to make this toggleable
	std::string indent(depth, '\t');

	return
		indent+"(node\n" +
		indent+"  (coordinate (0, 0))\n" +
		indent+"  (color " + color + ")\n" +
		indent+"  (leaves\n" + childs +
		indent+"  ))\n";
		*/
};


std::string mcts::serialize(void) {
	std::string ret = "";

	//ret = (std::string)"(budgie-tree\n" + serialize_node(root) + ")\n";
	ret = (std::string)"(budgie-tree " + serialize_node(root) + ")";

	return ret;
}

void mcts::deserialize(std::string& serialized) {

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
		leaves[coord]->criticality = criticality;
		leaves[coord]->coord = coord;
	}
}

bool mcts_node::fully_visited(board *state) {
	return traversals >= state->dimension * state->dimension;
	//return traversals >= state->dimension * state->dimension;
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

/*
double mcts_node::win_rate(void){
	return (double)wins / (double)traversals;
}
*/

void mcts_node::update_stats(board *state, point::color winner) {
	for (move::moveptr foo = state->move_list; foo; foo = foo->previous) {
		// update criticality maps
		auto& x = (*criticality)[foo->coord];
		x.traversals++;

		x.total_wins += state->owns(foo->coord, winner);
		x.black_wins += winner == point::color::Black;
		x.black_owns += state->owns(foo->coord, point::color::Black);
		x.white_wins += winner == point::color::White;
		x.white_owns += state->owns(foo->coord, point::color::White);

		for (mcts_node *ptr = this; ptr; ptr = ptr->parent) {
			bool won = foo->color == winner;

			/*
			ptr->criticality[foo->coord].wins += won && state->owns(foo->coord, foo->color);
			ptr->criticality[foo->coord].traversals += 1;
			*/


			// node rave maps track the best moves for the oppenent
			if (ptr->color == foo->color) {
				continue;
			}

			(*ptr->rave)[foo->coord].wins += won;
			(*ptr->rave)[foo->coord].traversals++;
		}
	}
}

void mcts_node::update(board *state) {
	point::color winner = state->determine_winner();

	update_stats(state, winner);

	for (mcts_node *ptr = this; ptr; ptr = ptr->parent) {
		//bool won = ptr->color == winner;
		bool won = ptr->color == winner;

		if (ptr->parent) {
			ptr->parent->nodestats[ptr->coord].wins += won;
			ptr->parent->nodestats[ptr->coord].traversals++;
		}

		ptr->traversals++;
		//ptr->wins += won;
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

// defined in gtp.cpp
// TODO: maybe move this to a utility function
std::string coord_string(const coordinate& coord);


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

	fprintf(stderr, "%s: %s, winrate: %.2f, rave: %.2f, criticality: %.2f, traversals: %u\n",
		(color == point::color::Black)? "B" : "W",
		coord_string(coord).c_str(),
		parent? parent->nodestats[coord].win_rate() : 0,
		parent? (*parent->rave)[coord].win_rate() : 0,
		parent? (*criticality)[coord].win_rate() : 0,
		traversals);

	for (auto& x : leaves) {
		if (x.second == nullptr || !x.second->fully_visited(state)) {
			continue;
		}

		x.second->dump_node_statistics(x.first, state, depth + 1);
	}
}

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
