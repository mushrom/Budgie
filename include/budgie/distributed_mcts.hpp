#pragma once

#include <budgie/game.hpp>
#include <budgie/mcts.hpp>

namespace mcts_thing {

class distributed_mcts : public mcts {
	public:
		distributed_mcts(tree_policy *tp, playout_policy *pp);
		virtual ~distributed_mcts();
		virtual void explore(board *state);
};

// namespace mcts_thing
}
