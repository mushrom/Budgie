#include <budgie/game.hpp>
#include <budgie/mcts.hpp>
#include <budgie/distributed_mcts.hpp>

namespace mcts_thing {

distributed_mcts::distributed_mcts(tree_policy *tp, playout_policy *pp)
	: mcts(tp, pp)
{
	// TODO: zmq init
}

distributed_mcts::~distributed_mcts() {
	puts("deconstructing distributed mcts");
	// TODO: zmq cleanup
}

void distributed_mcts::explore(board *state) {
	puts("got here");
	exit(0);
}

// namespace mcts_thing
}
