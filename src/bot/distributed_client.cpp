#include <budgie/distributed_client.hpp>
#include <budgie/budgie.hpp>
#include <unistd.h>
#include <iostream>

namespace mcts_thing {

distributed_client::distributed_client(args_parser::option_map& args) {
	bot = std::unique_ptr<budgie>(new budgie(args));
	playouts = stoi(args["playouts"]);

	ctx = std::unique_ptr<zmq::context_t>(new zmq::context_t);
	socket = std::unique_ptr<zmq::socket_t>(new zmq::socket_t(*ctx, ZMQ_REQ));
	socket->connect(args["server-host"].c_str());

	std::cerr << "client initialized" << std::endl;
}

void distributed_client::run() {
	board state;
	std::unique_ptr<mcts> sync_tree(new mcts);
	sync_tree->id = bot->tree->id;

	// NOTE: when we get here, both search_tree and sync_tree should be empty

	while (true) {
		// do tree diff and send results to the server
		// (when first starting, this is empty, so the server should send a full
		// tree back)
		auto difftree = mcts_diff(sync_tree.get(), bot->tree.get());
		auto temp = difftree->serialize(&state, 0);
		zmq::message_t request(temp.size() * 4);

		memcpy(request.data(), temp.data(), temp.size() * 4);
		//memset(request.data(), 'A', 1024);
		std::cerr << " <-- sending " << request.size() << " bytes" << std::endl;
		socket->send(request);
		std::cerr << '.' << std::flush;

		// merge updates into the sync tree
		//sync_tree->sync(difftree.get());
		//sync_tree->sync(search_tree.get());
		sync_tree->merge(difftree.get());
		// XXX : we should remove update increment in merge()
		sync_tree->updates--;

		// get reply from server, deserialize
		zmq::message_t reply;
		socket->recv(&reply);

		uint32_t *datas = static_cast<uint32_t*>(reply.data());

		anserial::deserializer der(datas, reply.size() / 8);
		anserial::s_tree bar(&der);
		std::vector<uint32_t> vec(datas, datas + reply.size()/4);
		//bar.dump_nodes(bar.data());

		std::cerr << "trying to deserialize..." << std::endl;

		// update tree structures
		//difftree->deserialize(vec, &state);
		mcts update_tree;
		update_tree.deserialize(vec, &state);

		std::cerr << " --> recieved tree: "
			<< "bytes: " << reply.size() << ", "
			<< "id: " << std::hex << update_tree.id << std::dec << ", "
			<< "updates: " << update_tree.updates
			<< std::endl;

		if (sync_tree->id != update_tree.id) {
			// TODO: we should have this at the budgie class level...
			sync_tree->reset();
			bot->tree->reset();

			sync_tree->deserialize(vec, &state);
			bot->tree->deserialize(vec, &state);

		} else {
			sync_tree->sync(&update_tree);
			bot->tree->sync(&update_tree);
		}

		// finally, explore on the working tree
		bot->tree->do_search(&state, bot->pool, bot->tree->root->traversals + playouts);
	}
}

// namespace mcts_thing
}
