#include <budgie/args_parser.hpp>

namespace bdg {

void args_parser::parse_args(int argc, char *argv[]){
	for (int i = 1; i < argc; i++) {
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

// namespace bdg
}
