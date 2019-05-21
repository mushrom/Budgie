#include <mcts-gb/mcts.hpp>
#include <mcts-gb/gtp.hpp>
#include <mcts-gb/args_parser.hpp>
#include <stdio.h>
#include <iostream>

using namespace mcts_thing;

std::map<std::string, std::string> default_options = {
	{"playouts",       "10000"},
	{"use_patterns",   "1"},
	{"mode",           "gtp"},
	{"tree_policy",    "uct-rave"},
	{"playout_policy", "random"},
	{"patterns",       "patterns.txt"},
};

int main(int argc, char *argv[]) {
	args_parser args(argc, argv, default_options);

	for (auto& arg : args.arguments){
		if (arg == "--help" || arg == "-h") {
			printf("Usage: ./budgie [options]\n");

			for (auto& x : default_options) {
				printf("    --%s (default: %s)\n", x.first.c_str(), x.second.c_str());
			}

			return 0;
		}
	}

	if (args.options["mode"] == "gtp") {
		gtp_client gtp;
		gtp.repl(args.options);

	} else {
		std::cerr << "Unknown mode " << args.options["mode"] << std::endl;
		return 1;
	}
	
	return 0;
}
