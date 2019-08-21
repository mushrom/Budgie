#pragma once

#include <zmq.hpp>
#include <budgie/game.hpp>
#include <budgie/mcts.hpp>
#include <budgie/args_parser.hpp>

namespace mcts_thing {

class distributed_client {
	public:
		distributed_client(std::unique_ptr<mcts> tree,
		                   args_parser::option_map& args);

		void run();

	private:
		std::unique_ptr<zmq::context_t> ctx;
		std::unique_ptr<zmq::socket_t> socket;
		std::unique_ptr<mcts> search_tree;
};

// namespace mcts_thing
}
