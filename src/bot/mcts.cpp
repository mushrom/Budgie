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
#include <string.h>

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

		if (c == coordinate {0, 0})
			// avoid passing in playouts
			continue;

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

	// XXX: avoid reinitializing root node for incremental searches
	if (root->traversals == 0) {
		root->try_expanding(state);
		init_node_root(root.get(), state);
	}

	//root->try_expanding(state);

	pool.run_all(
		[&] () {
			while (root->traversals < playouts) {
				board scratch(state);
				explore(&scratch);
				//fprintf(stderr, "%u\n", (unsigned)root->traversals);
			}
		}
	);

	pool.wait();

	fprintf(stderr, "# %u playouts\n", (unsigned)root->traversals);
	std::cerr << "# predicted playout: ";
	root->dump_best_move_statistics(state);
	std::cerr << std::endl;

	return root->best_move();
}

double mcts::win_rate(coordinate& coord) {
	// XXX: O(N), should just return mcts_node* or winrate from best_move()
	for (mcts_node *leaf : root->leaves_alive) {
		if (leaf->coord == coord) {
			return leaf->win_rate();
		}
	}

	return 0;

	/*
	unsigned hash = coord_hash_v2(coord);

	if (hash < 660 && root->leaves[hash]) {
		return root->leaves[hash]->win_rate();

	} else {
		return 0;
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
	//mcts_node* ptr = tree.search(state, root.get());
	maybe_nodeptr ptr = tree(state, root.get());

	if (ptr) {
		playout(state, *ptr);
	}
}

void mcts::playout(board *state, mcts_node *ptr) {
	if (!ptr) {
		if (finished_probe) {
			(*finished_probe)(state, ptr);
		}
		return;
	}

	// terminates when no strategies find a valid move to play
	for (;;) {
		//coordinate next;
		maybe_coord next;

		/*
		if (state->moves > 4*state->dimension*state->dimension) {
			ptr->update(state);
			return;
		}
		*/


		for (const auto& strat : playout_strats) {
			if (next = strat(state)) {
				break;
			}
		}

		if (!next) {
			//getchar();
			ptr->update(state);

			if (finished_probe) {
				(*finished_probe)(state, ptr);
			}
			return;
		}

		if (playout_probe) {
			(*playout_probe)(state, ptr, *next);
		}

		state->make_move(*next);

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

void init_node(mcts_node *ptr, board *state) {
	init_node_heuristics(ptr, state);
	//init_joseki_coord(ptr, state, ptr->coord, ptr->color);
	init_joseki_hash(ptr, state, state->hash);
}

void init_node_root(mcts_node *ptr, board *state) {
	init_node_heuristics(ptr, state);
	init_joseki_hash(ptr, state, state->hash);
}

#undef M
#define M 50
void init_node_heuristics(mcts_node *ptr, board *state) {
	point::color grid[9];

	for (mcts_node *leaf : ptr->leaves_alive) {
		if (leaf->coord == coordinate {0, 0}) {
			leaf->prior_wins = 10;
			leaf->prior_traversals = 100;
			continue;
		}
		unsigned hash = coord_hash_v2(leaf->coord);

		if (getBool(PARAM_BOOL_MCTS_INIT_PATTERNS)) {
			unsigned pat = get_pattern_db().search(state, leaf->coord);

			// don't ignore zero-weighted patterns (eye avoidance patterns),
			// just start with a significantly less than average weight
			// TODO: tweakable weight
			pat = (pat == 0)? 70 : pat;

			unsigned weight = M * pat/200.f;

			leaf->prior_wins       = weight;
			leaf->prior_traversals = M;

		// game-winning move
		} else if (getBool(PARAM_BOOL_MCTS_INIT_100PCT)) {
			leaf->prior_wins = M/2;
			leaf->prior_traversals = M/2;

		// even game
		} else {
			// TODO: M as parameter
			leaf->prior_wins = M/2;
			leaf->prior_traversals = M;
		}
	}
}

// XXX: TODO:
static unsigned coord_to_index(board *state, const coordinate& coord) {
	return state->dimension*(coord.second - 1) + (coord.first - 1);
}

void init_joseki_hash(mcts_node *ptr, board *state, uint64_t boardhash) {
	const auto& josekis = state->josekis->search(state->dimension, boardhash);

	//printf("have hash: %016lx\n", boardhash);

	if (josekis.size() > 0) {
		for (mcts_node *leaf : ptr->leaves_alive) {
			//fprintf(stderr, "lookup success! (%d, %d)\n", leaf->coord.first, leaf->coord.second);
			// TODO: tweak the knobs, expose this as configuration
			if (josekis.find(leaf->coord) != josekis.end()) {
				leaf->prior_wins       = 1000;
				leaf->prior_traversals = 1000;
			}
		}
	}
}

// TODO: not needed anymore
void mcts_node::new_node(board *state, coordinate& coord, point::color color) {
#if 0
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
		init_node(node, state);

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


	}
#endif
}

bool mcts_node::half_visited(board *state) {
	//fprintf(stderr, "%p: %u\n", this, (unsigned)traversals);
	return traversals > getUInt(PARAM_INT_NODE_EXPANSION_THRESHOLD)/2;
}

bool mcts_node::fully_visited(board *state) {
	//fprintf(stderr, "%p: %u\n", this, (unsigned)traversals);
	return traversals > getUInt(PARAM_INT_NODE_EXPANSION_THRESHOLD);
}

bool mcts_node::can_traverse(board *state) {
	return half_visited(state)
	    && try_half_init(state)
	    && fully_visited(state)
	    && try_expanding(state);
}

bool mcts_node::try_half_init(board *state) {
	if (half_finished) {
		// node is currently being expanded,
		// return to continue doing playouts from this node
		// (need to avoid subnodes being lopsidedly explored before
		//  the node is finished being expanded)
		return true;
	}

	bool temp = false;
	if (!half_lock.compare_exchange_strong(temp, true)) {
		return false;
	}

	this->rave = std::make_unique<ravemap>();

	// strongly discourage passing, otherwise tree search spends
	// a lot (a /lot/) of time exploring passes for no good reason
	(*this->rave)[0].wins       = 10;
	(*this->rave)[0].traversals = 100;

	// indicate that other threads can now traverse this node
	half_finished = true;
	return true;
}

bool mcts_node::try_expanding(board *state) {
	if (init_finished) {
		// node is currently being expanded,
		// return to continue doing playouts from this node
		// (need to avoid subnodes being lopsidedly explored before
		//  the node is finished being expanded)
		return true;
	}

	bool temp = false;
	if (!init_lock.compare_exchange_strong(temp, true)) {
		return false;
	}

	for (const auto& coord : state->available_moves) {
		if (!state->is_valid_move(coord)) {
			continue;
		}

		/*
		if (get_pattern_db().search(state, coord) == 0) {
			continue;
		}
		*/

		unsigned index = coord_hash_v2(coord);
		mcts_node *node = new mcts_node(this, state->current_player);

		node->criticality = criticality;
		node->coord = coord;

		// should be no other threads writing here
		//leaves[index] = node;
		leaves_alive.push_front(node);
	}

	init_node(this, state);

	// indicate that other threads can now traverse this node
	init_finished = true;
	return true;
}

bool mcts::ownership_settled(board *state) {
	return false;

	// TODO:
#if 0
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
#endif
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
		if (foo->coord == coordinate{0,0}) {
			// pass, just continue
			continue;
		}

		// update criticality maps
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
			// node rave maps track the best moves for the oppenent
			if (ptr->color == foo->color) {
				continue;
			}

			bool won = foo->color == winner;
			if (ptr->rave) {
				(*ptr->rave)[hash].wins += foo->color == winner;
				(*ptr->rave)[hash].traversals++;
			}
		}
	}
}

void mcts_node::update(board *state) {
	//point::color winner = state->determine_winner();
	float score = state->calculate_final_score();
	point::color winner = (score > 0)? point::color::Black : point::color::White;

	/*
	if (state->moves >= 4*state->dimension*state->dimension) {
		winner = point::color::Invalid;
		//return;
	}
	*/


	update_stats(state, winner);

	for (mcts_node *ptr = this; ptr; ptr = ptr->parent) {
		bool won = ptr->color == winner;
		unsigned hash = coord_hash_v2(ptr->coord);

		/*
		if (ptr->parent) {
			ptr->parent->expected_score[hash] += score;
			ptr->parent->score_counts[hash] += 1;
		}
		*/

		//ptr->expected_score[hash] += score;
		//ptr->score_counts[hash] += 1;
		ptr->wins += won;
		ptr->traversals++;
	}
}

float mcts_node::win_rate(void) {
	return float(wins + prior_wins) / (traversals + prior_traversals);
}

// TODO: where is this being used?
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

void mcts_node::dump_node_statistics(unsigned indent) {
	if (traversals == 0) return;
	for (unsigned i = 0; i < indent*4; i++) {
		putchar(' ');
	}

	std::cout
		<< ((color == point::color::Black)? "Black: " : "White: ")
		<< coord.first << "," << coord.second
		<< ": wins: " << wins
		<< ", traversals: " << traversals
		<< ", winrate: " << win_rate();

	if (parent && parent->rave) {
		unsigned hash = coord_hash_v2(coord);
		std::cout << ", rave: " << (*parent->rave)[hash].win_rate();
	}

	std::cout << std::endl;

	for (mcts_node *leaf : leaves_alive) {
		leaf->dump_node_statistics(indent + 1);
	}

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

// TODO: rewrite for no leaves
void mcts_node::dump_best_move_statistics(board *state) {
#if 0
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
#endif
}

// namespace mcts_thing
}
