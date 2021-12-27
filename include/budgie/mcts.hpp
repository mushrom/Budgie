#pragma once

#include <budgie/game.hpp>
#include <budgie/pattern_db.hpp>
#include <budgie/ts_forward_list.hpp>
#include <budgie/thread_pool.hpp>
#include <anserial/anserial.hpp>
#include <stdint.h>
#include <list>
#include <utility>
#include <unordered_map>
#include <map>
#include <memory>
#include <bitset>
#include <atomic>

// extending std::atomic to provide dereferencing for pointer types
template <typename T>
struct atomptr : public std::atomic<T> {
	T operator->(void) {
		return this->load();
	}

	T operator=(const T& value) {
		this->store(value);
		return value;
	}
};

namespace mcts_thing {

struct coord_hash {
	std::size_t operator () (const coordinate& coord) const {
		return (coord.first << 5) | coord.second;
	}
};

static inline unsigned coord_hash_v2(const coordinate& coord) {
	return (coord.first << 5) | coord.second;
}

// TODO: tweakable, what's a good value for this?
//       think I got 50 from the mogo paper?
#define M 50

// TODO: rename stats and crit_stats
class stats {
	public:
		std::atomic<unsigned> wins = M/2;
		std::atomic<unsigned> traversals = M;

		double win_rate(void) const {
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
		std::atomic<unsigned> total_wins = 0;
		std::atomic<unsigned> traversals = 0;

		std::atomic<unsigned> black_wins = 0;
		std::atomic<unsigned> white_wins = 0;

		std::atomic<unsigned> black_owns = 0;
		std::atomic<unsigned> white_owns = 0;

		double win_rate(void) const {
			// not enough games using this point to say one way or another
			if (traversals < M) return 0;

			double uwin = total_wins / (1. * traversals);
			double ubx  = black_owns / (1. * traversals);
			double ub   = black_wins / (1. * traversals);
			double uwx  = white_owns / (1. * traversals);
			double uw   = white_wins / (1. * traversals);

			double ret = uwin - (ubx*ub + uwx*uw);

			// when ret < 0, the move is anticorrelated with winning
			return (ret > 0)? ret : 0;
		};

		// get a score of how "settled" this point is
		float settlement(void) const {
			// probably an eye if the point has no/very few traversals
			if (traversals < M) return 1;

			float ubx = black_owns / (1. * traversals);
			float uwx = white_owns / (1. * traversals);

			float ret = (ubx > uwx)? ubx : uwx;

			// still probably an eye but with some playouts using it
			if (ret < 0.05) return 1;
			else return ret;
		}
};

class mcts_node {
	public:
		typedef std::unique_ptr<mcts_node> nodeptr;
		typedef std::array<crit_stats, 660> critmap;
		typedef std::shared_ptr<critmap> critptr;

		mcts_node(mcts_node *n_parent = nullptr,
		          point::color player = point::color::Empty)
		{
			color = player;
			parent = n_parent;
			traversals = 0;

			for (int i = 0; i < 660; i++) {
				leaves[i] = nullptr;
				expected_score[i] = 0;
				score_counts[i] = 0;
			}
		};

		~mcts_node() {
			leaves_alive.clear();

			for (int i = 0; i < 660; i++) {
				if (leaves[i]) {
					delete leaves[i];
				}
			}
		}

		void update(board *state);
		void update_stats(board *state, point::color winner);
		void new_node(board *state, coordinate& coord, point::color color);

		coordinate best_move(void);
		bool fully_visited(board *state);

		unsigned terminal_nodes(void);
		unsigned nodes(void);
		void dump_node_statistics(const coordinate& coord, board *state,
		                          unsigned depth=0);
		void dump_best_move_statistics(board *state);

		void init_joseki_root(board *state);
		void init_joseki_coord(board *state,
		                       const coordinate& coord,
		                       point::color color);
		void init_joseki_hash(board *state, uint64_t boardhash);

		std::atomic<unsigned> traversals;
		mcts_node *parent;
		point::color color;
		coordinate coord;

		atomptr<mcts_node*> leaves[660];
		stats     nodestats[660];
		stats     rave[660];
		float     expected_score[660];
		unsigned  score_counts[660];

		// for iterating over current valid leaves
		ts_forward_list<mcts_node*> leaves_alive;

		critptr criticality;
		//ravestats rave;

		// synced with mcts tree updates when merged, so we can
		// send differential updates (in distributed mode)
		unsigned updates;
};

// TODO: Maybe make this a part of the board class somewhere, since it's game-specific
coordinate pick_random_leaf(board *state, pattern_db *patterns);

// TODO: maybe move this to pattern_db.hpp
typedef std::shared_ptr<pattern_db> pattern_dbptr;

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
		uct_tree_policy(pattern_dbptr db)
			: tree_policy(db) {}

		virtual mcts_node* search(board *state, mcts_node *ptr);

	private:
		coordinate max_utc(board *state, mcts_node *ptr);
		double uct(const coordinate& coord, board *state, mcts_node *ptr);
};

// MC+UCT+RAVE tree exploration policy
class uct_rave_tree_policy : public tree_policy {
	public:
		uct_rave_tree_policy(pattern_dbptr db)
			: tree_policy(db) {}

		virtual mcts_node* search(board *state, mcts_node *ptr);

	private:
		coordinate max_utc(board *state, mcts_node *ptr);
		double uct(const coordinate& coord, board *state, mcts_node *ptr);
};

class random_playout : public playout_strategy {
	public:
		random_playout(pattern_dbptr db) : playout_strategy(db) { };
		virtual coordinate apply(board *state);
};

class capture_weighted_playout : public playout_strategy {
	public:
		capture_weighted_playout(pattern_dbptr db) : playout_strategy(db) { };
		virtual coordinate apply(board *state);
};

class save_atari_playout : public playout_strategy {
	public:
		save_atari_playout(pattern_dbptr db) : playout_strategy(db) { };
		virtual coordinate apply(board *state);
};

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

class adjacent_3x3_playout : public playout_strategy {
	public:
		adjacent_3x3_playout(pattern_dbptr db) : playout_strategy(db) { };
		virtual coordinate apply(board *state);
};

class adjacent_5x5_playout : public playout_strategy {
	public:
		adjacent_5x5_playout(pattern_dbptr db) : playout_strategy(db) { };
		virtual coordinate apply(board *state);
};

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

		coordinate do_search(board *state,
		                     thread_pool& pool,
		                     unsigned playouts);
		double win_rate(coordinate& coord);
		void reset();

		bool ownership_settled(board *state);

		// TODO: we should use board hashes rather than tree IDs
		uint32_t id = 0;
		// number of updates (merges) to this tree, used to do
		// efficient delta updates in distributed mode
		uint32_t updates = 0;

		std::vector<uint32_t> serialize(board *state, uint32_t since);
		void serialize(anserial::serializer& ser, uint32_t parent);
		void deserialize(std::vector<uint32_t>& serialized, board *state);
		bool merge(mcts *other);
		bool sync(mcts *other);

		tree_policy *tree;
		std::list<playout_strategy*> playout_strats;
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
