#include <budgie/mcts.hpp>
#include <budgie/gtp.hpp>
#include <budgie/args_parser.hpp>
#include <stdio.h>
#include <iostream>

using namespace mcts_thing;

args_parser::default_map default_options = {
	{"playouts",
		{"10000", "Number of playouts to use per move",
			{"<any unsigned integer>"}}},
	{"use_patterns",
		{"1", "Boolean to enable/disable the pattern database",
			{"0", "1"}}},
	{"mode",
		{"gtp", "Interface mode to use",
			{"gtp"}}},
	{"tree_policy",
		{"uct-rave", "Policy to use for tree traversal",
			{"uct-rave"}}},
	{"playout_policy",
		{"random", "Policy to use when playing out games",
			{"random", "local_weighted"}}},
	{"patterns",
		{"patterns.txt", "Local pattern database file (gnugo format)",
			{"patterns.txt"}}},
};

int main(int argc, char *argv[]) {
	args_parser args(argc, argv, default_options);

	for (auto& arg : args.arguments) {
		if (arg == "--help" || arg == "-h") {
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
