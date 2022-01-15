#include <budgie/game.hpp>
#include <budgie/mcts.hpp>
#include <budgie/distributed_mcts.hpp>
#include <budgie/serialize.hpp>

namespace mcts_thing {

distributed_mcts::distributed_mcts(tree_policy tp, std::list<playout_strategy> strats)
	: mcts(tp, strats)
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
	zmq::message_t request;
	socket->recv(&request);

	uint8_t *dat = static_cast<uint8_t*>(request.data());
	//std::vector<uint32_t> vec(dat, dat + request.size()/4);
	board temp;

	kj::ArrayPtr asdf(dat, request.size());
	auto temp_tree = deserializeTree(asdf);
	//temp_tree.deserialize(vec, &temp);

	std::cerr << " --> recieved a tree: "
		<< "bytes: " << request.size() << ", "
		<< "id: " << std::hex << temp_tree->id << std::dec << ", "
		<< "updates: " << temp_tree->updates << " (current: " << updates << ")"
		<< std::endl;

	bool merged;
	kj::VectorOutputStream outstream;

	if ((merged = merge(temp_tree.get()))) {
		//cur_tree = serialize(state, temp_tree->updates + 2);
		serializeTree(outstream, this, temp_tree->updates + 2);

	} else {
		//serialize(state, 0);
		serializeTree(outstream, this, 0);
	}

	//std::cerr << "current playouts: " << root->traversals << std::endl;

	auto array = outstream.getArray();
	zmq::message_t reply(array.size());
	memcpy(reply.data(), array.begin(), array.size());
	socket->send(reply);

	std::cerr << " <-- "
		<< (merged? "merged, " : "rejected, ")
		<< "sent " << array.size() << " bytes" << std::endl;
}

// namespace mcts_thing
}
