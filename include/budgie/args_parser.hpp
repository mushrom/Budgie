#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <list>

namespace bdg {

class args_parser {
	public:
		typedef struct {
			std::string def_value;
			std::string info;
			std::list<std::string> available;
		} option;

		typedef std::unordered_map<std::string, option> default_map;
		typedef std::unordered_map<std::string, std::string> option_map;

		args_parser(int argc, char *argv[], default_map defaults) {
			for (const auto& x : defaults) {
				options[x.first] = x.second.def_value;
			}

			parse_args(argc, argv);
		}

		void parse_args(int argc, char *argv[]);

		// fully-populated option map, with defaults set
		option_map options;

		// in-order, unprocessed arguments
		std::vector<std::string> arguments;
};

// namespace bdg
}
