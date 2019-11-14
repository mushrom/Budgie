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

// TODO: trim down this class
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

				stats& operator+=(const stats& other) {
					wins += other.wins;
					traversals += other.traversals;
					return *this;
				}
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
		//typedef std::shared_ptr<ravestats> raveptr;
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
		void new_node(coordinate& coord, point::color color);

		//double win_rate(void);
		coordinate best_move(void);
		bool fully_visited(board *state);

		unsigned terminal_nodes(void);
		unsigned nodes(void);
		void dump_node_statistics(const coordinate& coord, board *state,
		                          unsigned depth=0);
		void dump_best_move_statistics(board *state);

		std::unordered_map<coordinate, nodeptr, coord_hash> leaves;
		//std::unordered_map<coordinate, stats, coord_hash> ownership;
		std::unordered_map<coordinate, stats, coord_hash> nodestats;

		critptr criticality;

		ravestats rave;
		// rave stats for this level
		//raveptr rave;
		// rave stats for children
		//raveptr child_rave;

		mcts_node *parent;
		point::color color;
		unsigned traversals;
		// synced with mcts tree updates when merged, so we can
		// send differential updates (in distributed mode)
		unsigned updates;
		coordinate coord;
		/*
		unsigned wins;
		*/
};

static inline mcts_node::stats
operator-(const mcts_node::stats& a, const mcts_node::stats& b) {
	mcts_node::stats ret;
	ret.wins = a.wins - b.wins;
	ret.traversals = a.traversals - b.traversals;

	return ret;
}


// TODO: Maybe make this a part of the board class somewhere, since it's game-specific
coordinate pick_random_leaf(board *state);

// TODO: maybe move this to pattern_db.hpp
typedef std::shared_ptr<pattern_db> pattern_dbptr;

// TODO: maybe make this pluggable like playout heuristics
// TODO: load pattern db as part of board class so we don't need to initialize
//       a db for every strategy
class tree_policy {
	public:
		tree_policy(pattern_dbptr db) { patterns = db; };
		virtual mcts_node* search(board *state, mcts_node *ptr) = 0;

	protected:
		pattern_dbptr patterns;
};

class playout_strategy {
	public:
		playout_strategy(pattern_dbptr db) { patterns = db; }
		virtual coordinate apply(board *state) = 0;

	protected:
		pattern_dbptr patterns;
};

// plain MCTS tree policy, exploring whichever node seems to have the highest
// win rate
class mcts_tree_policy : public tree_policy {
	public:
		mcts_tree_policy(pattern_dbptr db) : tree_policy(db){};
		virtual mcts_node* search(board *state, mcts_node *ptr);
};

// MC+UCT tree exploration policy
class uct_tree_policy : public tree_policy {
	public:
		uct_tree_policy(
			pattern_dbptr db,
			// UCT exploration weight
			// TODO: config option
			double uct_c=0.20) : tree_policy(db)
		{
			uct_weight  = uct_c;
		}

		virtual mcts_node* search(board *state, mcts_node *ptr);

	private:
		double uct_weight;

		coordinate max_utc(board *state, mcts_node *ptr);
		double uct(const coordinate& coord, board *state, mcts_node *ptr);
};

// MC+UCT+RAVE tree exploration policy
class uct_rave_tree_policy : public tree_policy {
	public:
		uct_rave_tree_policy(
			pattern_dbptr db,
			// TODO: config option
			// UCT exploration weight
			double uct_c=0.15,
			// RAVE estimation weight
			double rave_c=500) : tree_policy(db)
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

class random_playout : public playout_strategy {
	public:
		random_playout(pattern_dbptr db) : playout_strategy(db) { };
		virtual coordinate apply(board *state);
};

// TODO: rename to 'capture enemy ataris' or something like that
class capture_weighted_playout : public playout_strategy {
	public:
		capture_weighted_playout(pattern_dbptr db) : playout_strategy(db) { };
		virtual coordinate apply(board *state);
};

// TODO: rename to 'capture enemy ataris' or something like that
class save_atari_playout : public playout_strategy {
	public:
		save_atari_playout(pattern_dbptr db) : playout_strategy(db) { };
		virtual coordinate apply(board *state);
};

// TODO: rename to 'capture enemy ataris' or something like that
class attack_enemy_groups_playout : public playout_strategy {
	public:
		attack_enemy_groups_playout(pattern_dbptr db) : playout_strategy(db) { };
		virtual coordinate apply(board *state);
};

class local_weighted_playout : public playout_strategy {
	public:
		local_weighted_playout(pattern_dbptr db) : playout_strategy(db) { };
		virtual coordinate apply(board *state);
		coordinate local_best(board *state);
};

// TODO: should consider splitting mcts_node and mcts code
class mcts {
	public:
		mcts() {
			tree = nullptr;
			playout_strats = {};
			reset();
		};

		mcts(tree_policy *tp, std::list<playout_strategy*> strats)
		{
			tree = tp;
			playout_strats = strats;
			reset();
		};

		virtual ~mcts(){};
		virtual void explore(board *state);
		virtual void playout(board *state, mcts_node *ptr);

		coordinate do_search(board *state, unsigned playouts=10000);
		double win_rate(coordinate& coord);
		void reset();

		// TODO: we should use board hashes rather than tree IDs
		uint32_t id = 0;
		// number of updates (merges) to this tree, used to do
		// efficient delta updates in distributed mode
		uint32_t updates = 0;

		// TODO: should anserial have a typedef for the return type?
		std::vector<uint32_t> serialize(board *state, uint32_t since);
		void serialize(anserial::serializer& ser, uint32_t parent);
		void deserialize(std::vector<uint32_t>& serialized, board *state);
		bool merge(mcts *other);
		bool sync(mcts *other);

		tree_policy *tree;
		std::list<playout_strategy*> playout_strats;

		//playout_policy *policy;

		// TODO: we need to change this to a shared pointer
		//mcts_node *root = nullptr;
		mcts_node::nodeptr root = nullptr;

	private:
		uint32_t serialize_node(anserial::serializer& ser,
		                        uint32_t parent,
		                        const mcts_node* ptr,
		                        uint32_t since);

		mcts_node *deserialize_node(anserial::s_node *node, mcts_node *ptr);
		mcts_node *merge_node(mcts_node *own, mcts_node *other);
		mcts_node *sync_node(mcts_node *own, mcts_node *other);
};

std::shared_ptr<mcts> mcts_diff(mcts *a, mcts *b);

// namespace mcts_thing
}
