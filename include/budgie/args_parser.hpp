#pragma once
#include <vector>
#include <map>
#include <string>

namespace mcts_thing {

class args_parser {
	public:
		args_parser(int argc, char *argv[], std::map<std::string, std::string> defaults) {
			options = defaults;
			parse_args(argc, argv);
		}

		void parse_args(int argc, char *argv[]);

		std::map<std::string, std::string> options;
		std::vector<std::string> arguments;
};

// namespace mcts_thing
}
