#include <budgie/mcts.hpp>
#include <budgie/distributed_mcts.hpp>
#include <budgie/distributed_client.hpp>
#include <budgie/gtp.hpp>
#include <budgie/args_parser.hpp>
#include <iostream>
#include <thread>
#include <time.h>
#include <stdio.h>

#include <budgie/budgie.hpp>
#include <budgie/parameters.hpp>

using namespace bdg;

/*
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
			{"uct-rave", "uct", "mcts"}}},
	{"playout_policy",
		{"random", "Policy to use when playing out games",
			{"random", "local_weighted"}}},
	{"patterns",
		{"patterns.txt", "Local pattern database file (gnugo format)",
			{"patterns.txt"}}},
	{"server-host",
		{"tcp://localhost:5555", "Master server using the distributed mode",
			{"<ZMQ socket string>"}}},
	{"worker-threads",
		{"0", "Number of workers to start as a distributed client",
			{"0 for auto", "<any unsigned integer>"}}},
};
*/

void print_help(void) {
	printf("Usage: ./budgie [options]\n");

	for (auto& x : budgie_options) {
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

	printf("\n");
	printf("Available value parameters: (type:key=default value)\n");

	for (int i = 1; intParamNames[i]; i++) {
		printf("    int:%s=%d\n", intParamNames[i], intParameters[i]);
	}

	for (int i = 1; boolParamNames[i]; i++) {
		const char *val = boolParameters[i]? "true" : "false";
		printf("    bool:%s=%s\n", boolParamNames[i], val);
	}

	for (int i = 1; floatParamNames[i]; i++) {
		printf("    float:%s=%g\n", floatParamNames[i], floatParameters[i]);
	}
}

void run_workers(args_parser::option_map& options) {
	unsigned concurrency = stoi(options["worker-threads"]);

	if (concurrency == 0) {
		unsigned supported = std::thread::hardware_concurrency();
		concurrency = (supported > 0)? supported : 1;
	}

	std::thread workers[concurrency];
	std::cerr << "run_workers(): starting " << concurrency
	          << " threads" << std::endl;

	for (unsigned i = 0; i < concurrency; i++) {
		workers[i] = std::thread(
			[&] () {
				std::cerr << "run_workers(): thread " << i
				          << ": Hello, world!" << std::endl;
				//distributed_client client(init_mcts(options), options);
				distributed_client client(options);
				client.run();
			}
		);
	}

	for (auto& t : workers) {
		t.join();
	}
}

int main(int argc, char *argv[]) {
	srand(time(NULL));
	args_parser args(argc, argv, budgie_options);

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
		gtp.repl(args.options);
	}

	else if (args.options["mode"] == "distributed-worker") {
		distributed_client client(args.options);
		client.run();
	}

	else {
		std::cerr << "Unknown mode " << args.options["mode"] << std::endl;
		return 1;
	}

	return 0;
}
