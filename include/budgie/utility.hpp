#pragma once

#include <vector>
#include <string>

namespace mcts_thing {

static inline
std::vector<std::string> split_string(const std::string& str,
                                      char delim = ' ')
{
	std::vector<std::string> ret;

	size_t start = 0;
	size_t end   = str.find(delim);

	while (start < str.length()) {
		std::string foo = str.substr(start, end - start);
		ret.push_back(foo);

		if (end == std::string::npos)
			break;

		start = end + 1;
		end   = str.find(delim, start);
	}

	return ret;
}

// namespace mcts_thing
}
