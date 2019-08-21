#include <budgie/mcts.hpp>
#include <budgie/distributed_mcts.hpp>
#include <budgie/gtp.hpp>
#include <budgie/args_parser.hpp>
#include <stdio.h>
#include <iostream>

using namespace mcts_thing;

args_parser::default_map default_options = {
	{"mode",
		{"gtp", "Interface mode to use",
			{"gtp", "distributed-gtp", "distributed-worker"}}},
	{"playouts",
		{"10000", "Number of playouts to use per move",
			{"<any unsigned integer>"}}},
	{"use_patterns",
		{"1", "Boolean to enable/disable the pattern database",
			{"0", "1"}}},
	{"tree_policy",
		{"uct-rave", "Policy to use for tree traversal",
			{"uct-rave"}}},
	{"playout_policy",
		{"random", "Policy to use when playing out games",
			{"random", "local_weighted"}}},
	{"patterns",
		{"patterns.txt", "Local pattern database file (gnugo format)",
			{"patterns.txt"}}},
	{"server-host",
		{"localhost", "Master server using the distributed mode",
			{"<Valid hostname/IP address"}}},
	{"server-port",
		{"5555", "Master server port when using distributed mode",
			{"<TCP port>"}}},
	{"worker-threads",
		{"0", "Number of workers to start as a distributed worker",
			{"0 for auto", "<any unsigned integer>"}}},
};

void print_help(void) {
	printf("Usage: ./budgie [options]\n");

	for (auto& x : default_options) {
		printf("    --%s %*s %s\n",
		       x.first.c_str(),
		       (int)(18 - x.first.length()),
		       " ",
		       x.second.info.c_str());

		printf("         %*s Default: %s, available: ",
		       16, " ",
		       x.second.def_value.c_str());

		for (const auto& opt : x.second.available) {
			printf("%s, ", opt.c_str());
		}

		printf("\n");
	}
}

// XXX
std::unique_ptr<mcts> init_mcts(args_parser::option_map& options) {
	// TODO: we should have an AI instance class that handles
	//       all of the initialization,, board/mcts interaction (make_move, ...)
	pattern_dbptr db = pattern_dbptr(new pattern_db(options["patterns"]));

	// only have this tree policy right now
	tree_policy *tree_pol = new uct_rave_tree_policy(db);
	playout_policy *playout_pol = (options["playout_policy"] == "local_weighted")
		? (playout_policy *)(new local_weighted_playout(db))
		: (playout_policy *)(new random_playout(db));

	if (options["mode"] == "distributed-gtp") {
		return std::unique_ptr<mcts>(new distributed_mcts(tree_pol, playout_pol));

	} else {
		return std::unique_ptr<mcts>(new mcts(tree_pol, playout_pol));
	}
}

int main(int argc, char *argv[]) {
	args_parser args(argc, argv, default_options);

	for (auto& arg : args.arguments) {
		if (arg == "--help" || arg == "-h") {
			print_help();
			return 0;
		}
	}

	if (args.options["mode"] == "gtp"
	    || args.options["mode"] == "distributed-gtp")
	{
		gtp_client gtp;
		gtp.repl(init_mcts(args.options), args.options);
	}

	else if (args.options["mode"] == "distributed-worker") {
		std::cout << "TODO: worker mode" << std::endl;
	}

	else {
		std::cerr << "Unknown mode " << args.options["mode"] << std::endl;
		return 1;
	}

	return 0;
}
