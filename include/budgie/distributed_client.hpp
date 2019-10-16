#pragma once

#include <zmq.hpp>
#include <budgie/game.hpp>
#include <budgie/mcts.hpp>
#include <budgie/args_parser.hpp>
#include <budgie/budgie.hpp>

namespace mcts_thing {

class distributed_client {
	public:
		distributed_client(args_parser::option_map& args);

		void run();

	private:
		// zmq state
		std::unique_ptr<zmq::context_t> ctx;
		std::unique_ptr<zmq::socket_t> socket;

		// bot state
		std::unique_ptr<budgie> bot;
		unsigned playouts;
};

// namespace mcts_thing
}
