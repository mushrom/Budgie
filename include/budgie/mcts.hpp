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

#include <functional>
#include <optional>

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
		//std::atomic<unsigned> wins = M/2;
		//std::atomic<unsigned> traversals = M;
		std::atomic<unsigned> wins;
		std::atomic<unsigned> traversals;

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

void init_node(mcts_node *ptr, board *state);
void init_node_root(mcts_node *ptr, board *state);
void init_node_heuristics(mcts_node *ptr, board *state);
void init_joseki_coord(mcts_node *ptr, board *state,
					   const coordinate& coord,
					   point::color color);
void init_joseki_hash(mcts_node *ptr, board *state, uint64_t boardhash);

// TODO: Maybe make this a part of the board class somewhere, since it's game-specific
coordinate pick_random_leaf(board *state, pattern_db *patterns);

typedef std::optional<coordinate> maybe_coord;
typedef std::optional<mcts_node*> maybe_nodeptr;

typedef std::function<maybe_nodeptr(board *state, mcts_node *ptr)> tree_policy;
typedef std::function<maybe_coord(board *state)> playout_strategy;

// callback type for examining tree state at various points
// bit of a XXX
// (honestly this whole codebase needs some serious reworking...)
typedef std::function<void(board *state, mcts_node *ptr)> tree_probe_func;
typedef std::function<void(board *state, mcts_node *ptr, const coordinate& next)>
        playout_probe_func;

class mcts {
	public:
		mcts() {
			// nop tree search policy for data-only trees
			tree = [] (board*, mcts_node*) { return maybe_nodeptr(); };
			playout_strats = {};
			reset();
		};

		mcts(tree_policy tp, std::list<playout_strategy> strats) {
			tree = tp;
			playout_strats = strats;
			reset();
		};

		// overloaded in distributed mcts
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

		tree_policy tree;
		std::list<playout_strategy> playout_strats;
		mcts_node::nodeptr root = nullptr;

		// optional debug/extension callbacks for examining tree and playout state
		std::optional<tree_probe_func>    treesearch_probe;
		std::optional<playout_probe_func> playout_probe;
		std::optional<tree_probe_func>    finished_probe;

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
