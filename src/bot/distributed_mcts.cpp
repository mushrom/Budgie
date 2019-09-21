#include <budgie/game.hpp>
#include <budgie/mcts.hpp>
#include <budgie/distributed_mcts.hpp>

namespace mcts_thing {

distributed_mcts::distributed_mcts(tree_policy *tp, playout_policy *pp)
	: mcts(tp, pp)
{
	ctx = std::unique_ptr<zmq::context_t>(new zmq::context_t);
	socket = std::unique_ptr<zmq::socket_t>(new zmq::socket_t(*ctx, ZMQ_REP));

	// TODO: use port specified in options
	socket->bind("tcp://*:5555");

	std::cerr << "initialized zmq stuff" << std::endl;
}

distributed_mcts::~distributed_mcts() {
	puts("deconstructing distributed mcts");
	// TODO: zmq cleanup
}

void distributed_mcts::explore(board *state) {
	// request counter, just for debugging output
	static unsigned counter = 0;

	std::cerr << "waiting for client..." << std::endl;

	zmq::message_t request;

	socket->recv(&request);
	std::cerr << "recieved request #" << counter++
		<< ", bytes: " << request.size() << std::endl;

	// TODO: merge tree
	uint32_t *dat = static_cast<uint32_t*>(request.data());
	std::vector<uint32_t> vec(dat, dat + request.size()/4);
	board temp;
	deserialize(vec, &temp);

	auto cur_tree = serialize(state);

	zmq::message_t reply(4*cur_tree.size());
	memcpy(reply.data(), cur_tree.data(), 4*cur_tree.size());
	socket->send(reply);

	std::cerr << "sent ~" << 4*cur_tree.size() << " bytes" << std::endl;
}

// namespace mcts_thing
}
