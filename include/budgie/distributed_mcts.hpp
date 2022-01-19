#pragma once

#include <zmq.hpp>
#include <budgie/game.hpp>
#include <budgie/mcts.hpp>

namespace bdg {

class distributed_mcts : public mcts {
	public:
		distributed_mcts(tree_policy tp, std::list<playout_strategy> strats);
		virtual ~distributed_mcts();
		virtual void explore(board *state);

	private:
		// XXX: we need unique pointers here because zmq::socket_t requires
		//      an initialized context for it's own construction,
		//      so we can't initialize a regular zmq::socket_t here
		std::unique_ptr<zmq::context_t> ctx;
		std::unique_ptr<zmq::socket_t> socket;
};

// namespace bdg
}
