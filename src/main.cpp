#include <mcts-gb/mcts.hpp>
#include <mcts-gb/gtp.hpp>
#include <stdio.h>
#include <iostream>

std::map<std::string, std::string> default_options = {
	{"playouts",     "10000"},
	{"use_patterns", "1"},
	{"mode",         "gtp"},
};

class args_parser {
	public:
		args_parser() { options = default_options; }
		args_parser(int argc, char *argv[]) : args_parser() {
			parse_args(argc, argv);
		}

		void parse_args(int argc, char *argv[]);

		std::map<std::string, std::string> options;
		std::vector<std::string> arguments;
};

void args_parser::parse_args(int argc, char *argv[]){
	for (int i = 1; i < argc; i++) {
		/*
		if (argv[i][0] != '-') {
		}
		*/
		arguments.push_back(std::string(argv[i]));

		char *arg = argv[i];

		// remove leading hyphens and check to see if there's anything left afterwards
		while (*arg && *arg == '-') arg++;
		if (!*arg) continue;

		for (auto &x : options) {
			if (arg == x.first){
				options[x.first] = argv[++i];
			}
		}
	}
}

int main(int argc, char *argv[]) {
	args_parser args(argc, argv);

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
		mcts_thing::gtp_client gtp;
		gtp.repl(args.options);

	} else {
		std::cerr << "Unknown mode " << args.options["mode"] << std::endl;
		return 1;
	}
	
	return 0;
}
