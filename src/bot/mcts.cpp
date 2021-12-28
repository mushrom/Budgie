#include <budgie/mcts.hpp>
#include <budgie/pattern_db.hpp>
#include <budgie/parameters.hpp>
#include <anserial/anserial.hpp>
#include <random>
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <assert.h>
#include <float.h>

#include <budgie/thread_pool.hpp>

#define MIN(a, b) ((a < b)? a : b)
#define MAX(a, b) ((a > b)? a : b)

namespace mcts_thing {

coordinate random_coord(board *b) {
	//std::uniform_int_distribution<int> distribution(1, b->dimension);
	std::uniform_int_distribution<int> distribution(1, b->dimension);

	return coordinate(distribution(b->randomgen), distribution(b->randomgen));
}

// TODO: this could use more optimization
coordinate pick_random_leaf(board *state, pattern_db *patterns) {
	std::uniform_int_distribution<unsigned> udist(0, state->available_moves.size()-1);
	std::bitset<384> map;
	unsigned tried = 0;

	while (tried < state->available_moves.size()) {
		unsigned foo = udist(state->randomgen);
		if (map[foo]) continue;

		tried++;
		map[foo] = true;
		coordinate c = *std::next(state->available_moves.begin(), foo);

		if (!state->is_valid_move(c) || patterns->search(state, c) == 0)
			continue;

		return c;
	}

	return {0, 0};
}

coordinate mcts::do_search(board *state,
                           thread_pool& pool,
                           unsigned playouts)
{
	root->color = other_player(state->current_player);
	root->coord = state->last_move;

	root->init_joseki_root(state);

	pool.run_all(
		[&] () {
			while (root->traversals < playouts) {
				board scratch(state);
				explore(&scratch);
			}
		}
	);

	pool.wait();

	//fprintf(stderr, "# %u playouts\n", root->traversals);
	/*
	std::cerr << "# predicted playout: ";
	root->dump_best_move_statistics(state);
	std::cerr << std::endl;
	*/

	return root->best_move();
}

double mcts::win_rate(coordinate& coord) {
	unsigned hash = coord_hash_v2(coord);
	return root->nodestats[hash].win_rate();

	/*
	auto it = root->nodestats.find(coord);

	if (it != root->nodestats.end()) {
		return it->second.win_rate();

	} else {
		return 0.5;
	}
	*/
}

void mcts::reset() {
	static uint32_t temp = 0;

	id = temp++;
	// TODO: this might be needed for distributed stuff
	//while ((id = rand()) == temp);
	updates = 0;

	root = std::move(mcts_node::nodeptr(new mcts_node(nullptr, point::color::Empty)));
	root->criticality = mcts_node::critptr(new mcts_node::critmap);
}

void mcts::explore(board *state)
{
	mcts_node* ptr = tree->search(state, root.get());
	playout(state, ptr);
}

void mcts::playout(board *state, mcts_node *ptr) {
	if (!ptr) {
		return;
	}

	// terminates when no strategies find a valid move to play
	for (;;) {
		coordinate next;

		for (auto strat : playout_strats) {
			next = strat->apply(state);

			if (next != coordinate(0, 0)) {
				break;
			}
		}

		if (next == coordinate(0 ,0)) {
			//printf("#  no valid moves!\n");
			ptr->update(state);
			//getchar();
			return;
		}

		state->make_move(next);

		//state->print();
		//usleep(250000);
	}
}

uint32_t mcts::serialize_node(anserial::serializer& ser,
                              uint32_t parent,
                              const mcts_node* ptr,
                              uint32_t since)
{
	if (ptr == nullptr) {
		return 0;
	}

	if (ptr->updates < since) {
		return 0;
	}

	/*
	uint32_t self = ser.add_entities(parent,
		{"node",
			{"color", ptr->color},
			{"coordinate", {ptr->coord.first, ptr->coord.second}},
			{"traversals", ptr->traversals},
			{"updates", ptr->updates}});

	uint32_t leaves = ser.add_entities(self, {"leaves"});
	uint32_t leaves_cont = ser.add_entities(leaves, {});

	for (const auto& x : ptr->leaves) {
		serialize_node(ser, leaves_cont, x.second.get(), since);
	}

	uint32_t nodestats = ser.add_entities(self, {"node-stats"});
	uint32_t nodestats_cont = ser.add_entities(nodestats, {});

	for (const auto& x : ptr->nodestats) {
		coordinate coord = x.first;
		stats sts = x.second;

		ser.add_entities(nodestats_cont,
			{"leaf",
				{"traversals", sts.traversals},
				{"wins", sts.wins},
				{"coordinate", {coord.first, coord.second}}});
	}

	uint32_t ravestats = ser.add_entities(self, {"rave-stats"});
	uint32_t ravestats_cont = ser.add_entities(ravestats, {});

#ifdef SERIALIZE_RAVE
	for (const auto& x : ptr->rave) {
		//if (x.second.traversals > 50) {
			ser.add_entities(ravestats_cont,
				{"stats",
					{"traversals", x.second.traversals},
					{"wins", x.second.wins},
					{"coordinate", {x.first.first, x.first.second}}});
		//}
	}
#endif

	*/
	return 0;
};

mcts_node *mcts::deserialize_node(anserial::s_node *node, mcts_node *ptr) {
	/*
	point::color color;
	coordinate coord;
	uint32_t node_traversals;
	uint32_t node_updates;

	anserial::s_node *leaves;
	anserial::s_node *node_stats;
	anserial::s_node *rave;

	if (!node) {
		return nullptr;
	}

	if (!ptr) {
		return nullptr;
	}

	if (!anserial::destructure(node,
		{"node",
			{"color", (uint32_t*)&color},
			{"coordinate", {&coord.first, &coord.second}},
			{"traversals", &node_traversals},
			{"updates", &node_updates},
			{"leaves", &leaves},
			{"node-stats", &node_stats},
			{"rave-stats", &rave}}))
	{
		throw std::logic_error("mcts::deserialize_node(): invalid tree structure!");
	}

	ptr->color = color;
	ptr->coord = coord;
	ptr->traversals = node_traversals;
	ptr->updates = node_updates;

	if (leaves) {
		for (auto leaf : leaves->entities()) {
			coordinate leaf_coord;
			point::color leaf_color;

			if (!anserial::destructure(leaf,
				{"node",
					{"color", (uint32_t*)&leaf_color},
					{"coordinate", {&leaf_coord.first, &leaf_coord.second}}}))
			{
				std::cerr << "mcts::deserialize_node(): couldn't load leaf coordinate"
					<< std::endl;
				continue;
			}

			// XXX: point::color::Empty since it should be overwritten
			//      in the next level down
			ptr->new_node(leaf_coord, leaf_color);
			deserialize_node(leaf, ptr->leaves[leaf_coord].get());
		}
	}

	if (node_stats) {
		for (auto leaf : node_stats->entities()) {
			coordinate leaf_coord;
			stats leaf_stats;
			anserial::s_node *leaf_node;

			if (!anserial::destructure(leaf,
				{"leaf",
					{"traversals", &leaf_stats.traversals},
					{"wins", &leaf_stats.wins},
					{"coordinate", {&leaf_coord.first, &leaf_coord.second}},
					&leaf_node}))
			{
				std::cerr << "mcts::deserialize_node(): asdf should throw here"
					<< std::endl;
			}

			ptr->nodestats[leaf_coord] = leaf_stats;
		}
	}

#ifdef SERIALIZE_RAVE
	if (rave) {
		for (auto stat : rave->entities()) {
			coordinate leaf_coord;
			stats leaf_stats;

			if (!anserial::destructure(stat,
				{"stats",
					{"traversals", &leaf_stats.traversals},
					{"wins", &leaf_stats.wins},
					{"coordinate", {&leaf_coord.first, &leaf_coord.second}}}))
			{
				std::cerr << "mcts::deserialize_node(): asdf rave stats"
					<< std::endl;

				continue;
			}

			ptr->rave[leaf_coord] = leaf_stats;
		}
	}
#endif
	*/

	return nullptr;
}

std::vector<uint32_t> mcts::serialize(board *state, uint32_t since) {
	/*
	anserial::serializer ret;

	uint32_t data_top = ret.default_layout();
	uint32_t tree_top = ret.add_entities(data_top, {"budgie-tree"});
	ret.add_entities(tree_top, {"id", id});
	ret.add_entities(tree_top, {"updates", updates});
	state->serialize(ret, ret.add_entities(tree_top, {"board"}));
	uint32_t nodes = ret.add_entities(tree_top, {"nodes"});
	serialize_node(ret, nodes, root.get(), since);
	ret.add_symtab(0);

	return ret.serialize();
	*/

	return {};
}

void mcts::deserialize(std::vector<uint32_t>& serialized, board *state) {
	/*
	anserial::s_node *nodes;
	anserial::s_node *board_nodes;

	anserial::deserializer der(serialized);
	anserial::s_tree tree(&der);

	if (!anserial::destructure(tree.data(),
		{{"budgie-tree",
			{"id", &id},
			{"updates", &updates},
			{"board", &board_nodes},
			{"nodes", &nodes}}}))
	{
		tree.dump_nodes();
		throw std::logic_error("mcts::deserialize(): invalid tree structure!");
	}

	state->deserialize(board_nodes);
	//tree.dump_nodes(nodes);
	
	if (nodes) {
		deserialize_node(nodes, root.get());
	}
	*/
}

// merges nodes in a differential tree generated by mcts_diff()
bool mcts::merge(mcts *other) {
	if (!other || other->id != id) {
		return false;
	}

	// keep track of the number of tree updates so we can keep clients
	// in sync with the master
	updates++;
	merge_node(root.get(), other->root.get());

	return true;
}

// similar to merge, except it simply loads new nodes generated by mcts::serialize().
bool mcts::sync(mcts *other) {
	if (!other || other->id != id) {
		return false;
	}

	updates = other->updates;
	sync_node(root.get(), other->root.get());

	return true;
}


mcts_node *mcts::merge_node(mcts_node *own, mcts_node *other) {
#if 0
	if (!own || !other) {
		return nullptr;
	}

	own->updates = updates;
	own->traversals += other->traversals;
	own->color = other->color;

	for (const auto& leaf : other->leaves) {
		coordinate coord = leaf.first;
		own->new_node(coord, leaf.second->color);

		merge_node(own->leaves[coord].get(),
		           other->leaves[coord].get());
	}

	for (const auto& stat : other->nodestats) {
		/*
		if (stat.second.traversals > 50) {
			fprintf(stderr, "got diff: %u\n", stat.second.traversals);
		}

		if (stat.second.traversals > 1000) {
			fprintf(stderr, "  ^ probably bogus, ignoring...\n");
			continue;
		}
		*/

		own->nodestats[stat.first] += stat.second;
	}

#ifdef SERIALIZE_RAVE
	for (const auto& rave : other->rave) {
		own->rave[rave.first] += rave.second;
	}
#endif

	return own;
#endif

	return nullptr;
}

mcts_node *mcts::sync_node(mcts_node *own, mcts_node *other) {
	/*
	if (!own || !other) {
		return nullptr;
	}

	own->updates = 0;
	own->traversals = other->traversals;

	for (const auto& leaf : other->leaves) {
		coordinate coord = leaf.first;
		own->new_node(coord, leaf.second->color);

		sync_node(own->leaves[coord].get(),
		          other->leaves[coord].get());
	}

	for (const auto& stat : other->nodestats) {
		own->nodestats[stat.first] = stat.second;
	}

#ifdef SERIALIZE_RAVE
	for (const auto& rave : other->rave) {
		own->rave[rave.first] = rave.second;
	}
#endif

	return own;
}

static void mcts_diff_iter(mcts_node *result, mcts_node *a, mcts_node *b) {
	if (a && b) {
		mcts_node *older = (a->traversals <= b->traversals)? a : b;
		mcts_node *newer = (older == a)? b : a;

		result->traversals = newer->traversals - older->traversals;
		result->color = newer->color;

		for (const auto& leaf : newer->leaves) {
			coordinate coord = leaf.first;

			if (older->leaves[coord] == nullptr
				|| older->leaves[coord]->traversals < leaf.second->traversals)
			{
				result->new_node(coord, leaf.second->color);
				
				mcts_diff_iter(result->leaves[coord].get(),
				               older->leaves[coord].get(),
				               newer->leaves[coord].get());
			}
		}

		for (const auto& stat : newer->nodestats) {
			stats sts = stat.second - older->nodestats[stat.first];

			if (sts.traversals > 0) {
				result->nodestats[stat.first] = sts;
			}
		}

#ifdef SERIALIZE_RAVE
		for (const auto& rave : newer->rave) {
			stats rsts = rave.second - older->rave[rave.first];

			if (rsts.traversals > 0) {
				result->rave[rave.first] = rave.second;
			}
		}
#endif
	}
	
	else if (a || b) {
		// TODO: maybe we should move sync_node out of mcts
		//       so we can reuse it here, that way we can avoid duplicating
		//       all this syncing code...
		mcts_node *node = a? a : b;
		result->traversals = node->traversals;

		for (const auto& leaf : node->leaves) {
			coordinate coord = leaf.first;

			result->new_node(coord, leaf.second->color);
			mcts_diff_iter(result->leaves[coord].get(),
			               nullptr,
			               node->leaves[coord].get());
		}

		for (const auto& stat : node->nodestats) {
			result->nodestats[stat.first] = stat.second;
		}

#ifdef SERIALIZE_RAVE
		for (const auto& rave : node->rave) {
			result->rave[rave.first] = rave.second;
		}
#endif
	}
	*/

	return nullptr;
}

std::shared_ptr<mcts> mcts_diff(mcts *a, mcts *b) {
	/*
	assert(a && b);
	assert(a->id == b->id);

	mcts *result = new mcts();
	result->id = a->id;
	// TODO: send max, although they should be the same for now
	result->updates = a->updates;
	result->root = mcts_node::nodeptr(new mcts_node);
	mcts_diff_iter(result->root.get(), a->root.get(), b->root.get());

	return std::shared_ptr<mcts>(result);
	*/

	return nullptr;
}

void mcts_node::init_joseki_root(board *state) {
	init_joseki_hash(state, state->hash);
	//uint64_t boardhash = state->hash;
}

#include <string.h>

// XXX
static unsigned coord_to_index(board *state, const coordinate& coord) {
	return state->dimension*(coord.second - 1) + (coord.first - 1);
}


void mcts_node::init_joseki_coord(board *state,
                                  const coordinate& coord,
                                  point::color color)
{
	//uint64_t boardhash = board::gen_hash(coord, color, state->hash);
	uint64_t boardhash = state->hash;
	//init_joseki_hash(state, boardhash);

	const auto& josekis = state->josekis->search(state->dimension, boardhash);

	//printf("have hash: %016lx\n", boardhash);

	// can't have a hash if the current hash isn't in the thing
	if (josekis.size() > 0) {
		uint8_t tempgrid[384];
		memcpy(tempgrid, &state->grid, sizeof(state->grid));

		tempgrid[coord_to_index(state, coord)] = color;
		boardhash = state->regen_hash(tempgrid);
		init_joseki_hash(state, boardhash);
	}
}

void mcts_node::init_joseki_hash(board *state, uint64_t boardhash) {
	const auto& josekis = state->josekis->search(state->dimension, boardhash);

	//printf("have hash: %016lx\n", boardhash);

	if (josekis.size() > 0) {
		//printf("lookup success! (%d, %d)\n", coord.first, coord.second);
		for (const auto& c : josekis) {
			unsigned index = coord_hash_v2(c);

			// just for testing
			// TODO: tweak the knobs, expose this as configuration
			nodestats[index].traversals = 1000;
			nodestats[index].wins       = 1000;
		}
	}
}

void mcts_node::new_node(board *state, coordinate& coord, point::color color) {
	unsigned hash = coord_hash_v2(coord);

	if (leaves[hash] == nullptr) {
		// this node is unvisited, set up a new node
		mcts_node *node = new mcts_node(this, color);

		// XXX: need to initialize the node as normal
		//      before trying to swap with leaves[hash],
		//      can't place an incomplete node into the
		//      tree before it's ready
		node->criticality = criticality;
		node->coord = coord;
		node->init_joseki_coord(state, coord, color);

		mcts_node *nullnode = nullptr; // XXX: need a T& type
		if (!leaves[hash].compare_exchange_strong(nullnode, node)) {
			// TODO: performance counters to track stuff like this
			//std::cerr << "nevermind" << std::endl;
			// another thread got here first
			delete node;
			return;
		}

		leaves_alive.push_front(node);

		// TODO: flags to specify how the stats should be initialized,
		//       default to even

		// inherit rave stats from grandparent (of the leaf)
		/*
		if (parent) {
			for (int i = 0; i < 660; i++) {
				// could copy rave, nodestats, initialize rave with nodestats or vice versa...
				//leaves[hash]->rave[i] = parent->rave[i];
				leaves[hash]->nodestats[i] = parent->nodestats[i];
			}
		}
		*/


#if 0
		if (parent && parent->parent) {
			for (int i = 0; i < 660; i++) {
				leaves[hash]->rave[i] = parent->parent->rave[i];
			}

			/*
			for (const auto& x : parent->rave) {
				leaves[coord]->rave[x.first] = x.second;
			}
			*/
		}
#endif
	}
}

bool mcts_node::fully_visited(board *state) {
	return traversals > getUInt(PARAM_INT_NODE_EXPANSION_THRESHOLD);
}

bool mcts::ownership_settled(board *state) {
	if (!root || !root->criticality) {
		// can't determine if there's no critmap
		return false;
	}

	float sum = 0;
	unsigned critsize = 1;
	float maxscore = -FLT_MAX;
	float minscore =  FLT_MAX;

	for (unsigned y = 1; y <= state->dimension; y++) {
		for (unsigned x = 1; x <= state->dimension; x++) {
			unsigned hash = coord_hash_v2({x, y});
			crit_stats& g = (*root->criticality)[hash];

			if (g.traversals > 0) {
				sum += g.settlement();
				critsize += 1;
			}
		}
	}

	for (unsigned y = 1; y <= state->dimension; y++) {
		for (unsigned x = 1; x <= state->dimension; x++) {
			unsigned index = coord_hash_v2({x, y});
			unsigned count = root->score_counts[index];
			float scoresum = root->expected_score[index];

			// coordinate has no score samples
			if (count == 0)
				continue;

			float k = scoresum / count;
			maxscore = std::max(k, maxscore);
			minscore = std::min(k, minscore);

			if (getBool(PARAM_BOOL_VERY_VERBOSE)) {
				fprintf(stderr, "(%u, %u) %g\n", x, y, k);
			}
		}
	}

	float avgown = sum/critsize;

	if (getBool(PARAM_BOOL_VERY_VERBOSE)) {
		fprintf(stderr, "testing: %d, %d\n", PARAM_BOOL_VERY_VERBOSE, getBool(PARAM_BOOL_VERY_VERBOSE));
		fprintf(stderr, "avgown: %g\n", avgown);
		fprintf(stderr, "maxscore: %g\n", maxscore);
		fprintf(stderr, "minscore: %g\n", minscore);
	}

	auto sign = [](float x) { return (x > 0)? 1 : -1; };
	bool allfavor = sign(minscore) == sign(maxscore);

	// avgown approaches 1 as the board becomes completely filled,
	// allfavor is true if all moves favor the same player.
	//
	// TODO: tweakable
	return avgown > 0.81 && allfavor;
}

// TODO: could return hash index here, 0 is the hash for an invalid move
coordinate mcts_node::best_move(void) {
	coordinate ret = {0, 0};
	mcts_node *max = nullptr;
	
	for (mcts_node *it : leaves_alive) {
		if (max == nullptr) {
			max = it;
			ret = it->coord;

		} else if (it->traversals > max->traversals) {
			max = it;
			ret = it->coord;
		}
	}

	return ret;
}

void mcts_node::update_stats(board *state, point::color winner) {
	for (move::moveptr foo = state->move_list; foo; foo = foo->previous) {
		// update criticality maps
		// TODO: plain array criticality map
		//auto& x = (*criticality)[foo->coord];
		auto& x = (*criticality)[coord_hash_v2(foo->coord)];
		x.traversals++;

		x.total_wins += state->owns(foo->coord, winner);
		x.black_wins += winner == point::color::Black;
		x.black_owns += state->owns(foo->coord, point::color::Black);
		x.white_wins += winner == point::color::White;
		x.white_owns += state->owns(foo->coord, point::color::White);

		// update RAVE maps
		unsigned hash = coord_hash_v2(foo->coord);
		for (mcts_node *ptr = this; ptr; ptr = ptr->parent) {
			bool won = foo->color == winner;

			// node rave maps track the best moves for the oppenent
			if (ptr->color == foo->color) {
				continue;
			}

			ptr->rave[hash].wins += won;
			ptr->rave[hash].traversals++;
		}
	}
}

void mcts_node::update(board *state) {
	//point::color winner = state->determine_winner();
	float score = state->calculate_final_score();
	point::color winner = (score > 0)? point::color::Black : point::color::White;

	update_stats(state, winner);

	for (mcts_node *ptr = this; ptr; ptr = ptr->parent) {
		bool won = ptr->color == winner;
		unsigned hash = coord_hash_v2(ptr->coord);

		if (ptr->parent) {
			ptr->parent->nodestats[hash].wins += won;
			ptr->parent->nodestats[hash].traversals++;
			ptr->parent->expected_score[hash] += score;
			ptr->parent->score_counts[hash] += 1;
		}

		ptr->traversals++;
	}
}

unsigned mcts_node::terminal_nodes(void) {
	if (leaves_alive.size() == 0) {
		return 1;

	} else {
		unsigned ret = 0;

		for (mcts_node *leaf : leaves_alive) {
			ret += leaf->terminal_nodes();
		}

		return ret;
	}
}

unsigned mcts_node::nodes(void){
	unsigned ret = 1;

	for (mcts_node *leaf : leaves_alive) {
		ret += leaf->nodes();
	}

	return ret;
}

// defined in gtp.cpp
// TODO: maybe move this to a utility function
std::string coord_string(const coordinate& coord);


void mcts_node::dump_node_statistics(const coordinate& coord,
                                     board *state,
                                     unsigned depth)
{
	// TODO: maybe fix this for atomics
#if 0
	unsigned hash = coord_hash_v2(coord);

	auto print_spaces = [&](){
		for (unsigned i = 0; i < depth*2; i++) {
			std::cerr << ' ';
		}
	};

	print_spaces();

	fprintf(stderr, "%s: %s, winrate: %.2f, rave: %.2f, criticality: %.2f, traversals: %u\n",
		(color == point::color::Black)? "B" : "W",
		coord_string(coord).c_str(),
		parent? parent->nodestats[hash].win_rate() : 0,
		parent? parent->rave[hash].win_rate() : 0,
		// TODO: criticality as an array
		parent? (*criticality)[coord].win_rate() : 0,
		traversals);

	for (mcts_node *leaf : leaves_alive) {
		if (!leaf->fully_visited(state)) {
			continue;
		}

		leaf->dump_node_statistics(leaf->coord, state, depth + 1);
	}
#endif
}

void mcts_node::dump_best_move_statistics(board *state) {
	coordinate coord = best_move();

	if (coord == coordinate(0, 0)) {
		return;
	}

	std::cerr << coord_string(coord);
	std::cerr << ((color == point::color::Black)? " (W)" : " (B)");

	unsigned hash = coord_hash_v2(coord);
	if (fully_visited(state) && leaves[hash]) {
		std::cerr << ", ";
		leaves[hash]->dump_best_move_statistics(state);
	}
}

// namespace mcts_thing
}
