#pragma once

#include <mcts-gb/game.hpp>
#include <mcts-gb/pattern_db.hpp>
#include <list>
#include <utility>
#include <unordered_map>
#include <map>
#include <memory>
#include <bitset>

namespace mcts_thing {

// TODO: maybe move this to the 'board' class
struct coord_hash {
	std::size_t operator () (const coordinate& coord) const {
		return (coord.first * 3313) + (coord.second * 797);
	}
};

class mcts_node {
	public:
		class stats {
			public:
				unsigned wins = 0;
				unsigned traversals = 0;
				double win_rate(void) {
					// TODO: we shouldn't have a state where traversals is 0, right?
					return traversals? (double)wins / (double)traversals : 0.5;
				};
		};

		typedef std::unique_ptr<mcts_node> nodeptr;
		typedef std::unordered_map<coordinate, stats, coord_hash> ravestats;
		typedef std::shared_ptr<ravestats> raveptr;

		mcts_node(mcts_node *n_parent = nullptr,
		          point::color player = point::color::Empty)
		{
			color = player;
			parent = n_parent;
			traversals = wins = 0;
		};

		void update(board *state);
		void update_rave(board *state, point::color winner);
		void new_node(board *state, coordinate& coord);

		double win_rate(void);
		coordinate best_move(void);
		bool fully_visited(board *state);

		unsigned terminal_nodes(void);
		unsigned nodes(void);
		void dump_node_statistics(const coordinate& coord, board *state,
		                          unsigned depth=0);
		void dump_best_move_statistics(board *state);

		std::unordered_map<coordinate, nodeptr, coord_hash> leaves;

		// rave stats for this level
		raveptr rave;
		// rave stats for children
		raveptr child_rave;

		mcts_node *parent;
		unsigned color;
		unsigned traversals;
		unsigned wins;
};

// TODO: Maybe make this a part of the board class somewhere, since it's game-specific
coordinate pick_random_leaf(board *state);

// TODO: maybe move this to pattern_db.hpp
typedef std::shared_ptr<pattern_db> pattern_dbptr;

class tree_policy {
	public:
		tree_policy(pattern_dbptr db) { patterns = db; };
		virtual mcts_node* search(board *state, mcts_node *ptr) = 0;

	protected:
		pattern_dbptr patterns;
};

class playout_policy {
	public:
		playout_policy(pattern_dbptr db) { patterns = db; }
		virtual mcts_node* playout(board *state, mcts_node *ptr) = 0;

	protected:
		pattern_dbptr patterns;
};

// MC+UCT+RAVE tree exploration policy
class uct_rave_tree_policy : public tree_policy {
	public:
		uct_rave_tree_policy(
			pattern_dbptr db,
			// UCT exploration weight
			double uct_c=0.05,
			// RAVE estimation weight
			double rave_c=2000.0) : tree_policy(db)
		{
			uct_weight  = uct_c;
			rave_weight = rave_c;
		}

		virtual mcts_node* search(board *state, mcts_node *ptr);

	private:
		double uct_weight;
		double rave_weight;

		coordinate max_utc(board *state, mcts_node *ptr);
		double uct(const coordinate& coord, board *state, mcts_node *ptr);
};

class random_playout : public playout_policy {
	public:
		random_playout(pattern_dbptr db) : playout_policy(db) { };
		virtual mcts_node* playout(board *state, mcts_node *ptr);
};

class local_weighted_playout : public playout_policy {
	public:
		local_weighted_playout(pattern_dbptr db) : playout_policy(db) { };
		virtual mcts_node* playout(board *state, mcts_node *ptr);
		coordinate local_best(board *state);
};

class mcts {
	public:
		mcts(tree_policy *tp, playout_policy *pp)
		{
			tree = tp;
			policy = pp;
			reset();
		};

		~mcts(){};

		coordinate do_search(board *state, unsigned playouts=10000);
		double win_rate(coordinate& coord);

		void explore(board *state);

		tree_policy *tree;
		playout_policy *policy;

		void reset() {
			delete root;
			root = new mcts_node(nullptr, point::color::Empty);
			//root->rave = rave_map::ptr(new rave_map(point::color::Empty, nullptr));
			root->rave = mcts_node::raveptr(new mcts_node::ravestats);
			root->child_rave = mcts_node::raveptr(new mcts_node::ravestats);
		}

		mcts_node *root = nullptr;
};

// namespace mcts_thing
}
