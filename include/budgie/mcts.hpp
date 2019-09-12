#pragma once

#include <budgie/game.hpp>
#include <budgie/pattern_db.hpp>
#include <anserial/anserial.hpp>
#include <stdint.h>
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

		class crit_stats {
			public:
				unsigned total_wins = 0;
				unsigned traversals = 0;

				unsigned black_wins = 0;
				unsigned white_wins = 0;

				unsigned black_owns = 0;
				unsigned white_owns = 0;

				double win_rate(void) {
					double uwin = total_wins / (1. * traversals);
					double ubx  = black_owns / (1. * traversals);
					double ub   = black_wins / (1. * traversals);
					double uwx  = white_owns / (1. * traversals);
					double uw   = white_wins / (1. * traversals);

					double ret = uwin - (ubx*ub + uwx*uw);

					return (ret < 0)? -ret : ret;
				};
		};

		typedef std::unique_ptr<mcts_node> nodeptr;
		typedef std::unordered_map<coordinate, stats, coord_hash> ravestats;
		typedef std::unordered_map<coordinate, crit_stats, coord_hash> critmap;
		typedef std::shared_ptr<ravestats> raveptr;
		typedef std::shared_ptr<critmap> critptr;

		mcts_node(mcts_node *n_parent = nullptr,
		          point::color player = point::color::Empty)
		{
			color = player;
			parent = n_parent;
			//traversals = wins = 0;
			traversals = 0;
		};

		void update(board *state);
		void update_stats(board *state, point::color winner);
		void new_node(board *state, coordinate& coord);

		//double win_rate(void);
		coordinate best_move(void);
		bool fully_visited(board *state);

		unsigned terminal_nodes(void);
		unsigned nodes(void);
		void dump_node_statistics(const coordinate& coord, board *state,
		                          unsigned depth=0);
		void dump_best_move_statistics(board *state);

		std::unordered_map<coordinate, nodeptr, coord_hash> leaves;
		std::unordered_map<coordinate, stats, coord_hash> ownership;
		std::unordered_map<coordinate, stats, coord_hash> nodestats;
		//std::unordered_map<coordinate, crit_stats, coord_hash> criticality;

		critptr criticality;

		// rave stats for this level
		raveptr rave;
		// rave stats for children
		raveptr child_rave;

		mcts_node *parent;
		point::color color;
		unsigned traversals;
		coordinate coord;
		/*
		unsigned wins;
		*/
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

		virtual ~mcts(){};
		virtual void explore(board *state);

		coordinate do_search(board *state, unsigned playouts=10000);
		double win_rate(coordinate& coord);
		void reset();

		uint32_t id;

		// TODO: should anserial have a typedef for the return type?
		std::vector<uint32_t> serialize(board *state);
		void serialize(anserial::serializer& ser, uint32_t parent);
		void deserialize(std::vector<uint32_t>& serialized, board *state);
		tree_policy *tree;
		playout_policy *policy;
		mcts_node *root = nullptr;

	private:
		uint32_t serialize_node(anserial::serializer& ser,
		                        uint32_t parent,
		                        const mcts_node* ptr,
		                        unsigned depth=1);
};

// namespace mcts_thing
}
