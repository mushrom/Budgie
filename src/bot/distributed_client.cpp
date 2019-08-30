#include <budgie/distributed_client.hpp>
#include <anserial/anserial.hpp>
#include <anserial/s_tree.hpp>
#include <unistd.h>

namespace mcts_thing {

distributed_client::distributed_client(std::unique_ptr<mcts> tree,
                                       args_parser::option_map& args)
{
	ctx = std::unique_ptr<zmq::context_t>(new zmq::context_t);
	socket = std::unique_ptr<zmq::socket_t>(new zmq::socket_t(*ctx, ZMQ_REQ));
	search_tree = std::move(tree);

	socket->connect(args["server-host"].c_str());

	std::cerr << "client initialized" << std::endl;
}

void distributed_client::run() {
	while (true) {
		zmq::message_t request(1024);

		memset(request.data(), 'A', 1024);
		socket->send(request);
		std::cout << '.' << std::flush;

		zmq::message_t reply;
		socket->recv(&reply);

		uint32_t *datas = static_cast<uint32_t*>(reply.data());

		anserial::deserializer der(datas, reply.size() / 8);
		anserial::s_tree bar(&der);
		bar.dump_nodes();
		//auto foo = der.deserialize(datas, reply.size() / 8);
		//anserial::dump_nodes(foo, 0);

		std::cout << reply.size() << " bytes\n";

		usleep(100000);
	}
}

// namespace mcts_thing
}
