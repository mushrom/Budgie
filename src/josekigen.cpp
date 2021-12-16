#include <stdio.h>
#include <vector>
#include <string>
#include <optional>
#include <ctype.h>

#include <budgie/sgf.hpp>

using namespace mcts_thing::sgf;

struct gameLinear {
	unsigned size = 19;
	char winner = 'U';
	unsigned komi = 5;
	std::string ruleset = "chinese";
	std::vector<std::string> moves;

	void print(void) {
		printf("Game on %dx%d board, komi: %u, ruleset: %s, winner: %c\n",
		       size, size, komi, ruleset.c_str(), winner);

		for (unsigned i = 0; i < moves.size(); i++) {
			printf("\t%c: %s\n", (i&1)? 'W' : 'B', moves[i].c_str());
		}
	}
};

std::optional<gameLinear> extractInfo(parseNode node) {
	if (node.type == SGF
	    && node.getLeafType(0) == GAME_TREE
	    && node.leaves[0].getLeafType(0) == SEQUENCE
		&& node.leaves[0].leaves[0].getLeafType(0) == NODE)
	{
		gameLinear ret;

		parseNode sequence = node.leaves[0].leaves[0];
		parseNode rootnode = sequence.leaves[0];

		for (unsigned i = 0; i < rootnode.leaves.size(); i++) {
			if (rootnode.getLeafType(i) != PROPERTY) {
				continue;
			}

			std::string ident = rootnode.leaves[i].leaves[0].data;
			std::string value = rootnode.leaves[i].leaves[1].data;

			if (ident == "SZ") {
				ret.size = atoi(value.c_str());
			}

			else if (ident == "RE") {
				ret.winner = value[0];
			}

			else if (ident == "KM") {
				ret.komi = atoi(value.c_str());
			}

			else if (ident == "RU") {
				ret.ruleset = value;
			}
		}

		for (unsigned i = 1; i < sequence.leaves.size(); i++) {
			if (sequence.getLeafType(i) != NODE) {
				continue;
			}

			if (sequence.leaves[i].getLeafType(0) != PROPERTY) {
				continue;
			}

			parseNode property = sequence.leaves[i].leaves[0];

			std::string ident = property.leaves[0].data;
			std::string value = property.leaves[1].data;

			if (value == "") {
				break;
			}

			ret.moves.push_back(value);
		}

		return ret;

	} else {
		return {};
	}
}

#include <iostream>
int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s [target boardsize]\n", argv[0]);
		return 1;
	}

	int targetBoardsize = atoi(argv[1]);
	std::string line;

	while (std::getline(std::cin, line)) {
		FILE *in = fopen(line.c_str(), "r");

		if (!in) {
			fprintf(stderr, "Couldn't open file: %s\n", line.c_str());
			//return 2;
			continue;
		}

		parseResult thing = parseSGF(in);
		fclose(in);

		if (thing.has_value()) {
			//dump(*thing);
			if (auto game = extractInfo(*thing)) {
				if (game->size == targetBoardsize) {
					printf("Dumping game %s:\n", line.c_str());
					game->print();
				} else {
					printf("<Skipped %s>\n", line.c_str());
				}
			}
		}
	}

	return 0;
}
