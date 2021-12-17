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
#include <algorithm>
#include <budgie/budgie.hpp>
using namespace mcts_thing;

int main(int argc, char *argv[]) {
	if (argc < 3) {
		fprintf(stderr, "Usage: %s [target boardsize] [outfile]\n", argv[0]);
		return 1;
	}

	uint32_t targetBoardsize = atoi(argv[1]);
	std::string line;
	size_t numFiles = 0;

	args_parser::option_map options;

	for (const auto& x : budgie_options) {
		options[x.first] = x.second.def_value;
	}

	budgie bot(options);
	bot.boardsize = targetBoardsize;

	//std::map<uint64_t, std::vector<coordinate>> hashes;
	std::map<uint64_t, std::set<coordinate>> hashes;

	while (std::getline(std::cin, line)) {
		FILE *in = fopen(line.c_str(), "r");

		if (!in) {
			fprintf(stderr, "Couldn't open file: %s\n", line.c_str());
			//return 2;
			continue;
		}

		parseResult thing = parseSGF(in);
		fclose(in);
		numFiles++;

		if (thing.has_value()) {
			//dump(*thing);
			if (auto game = extractInfo(*thing)) {
				if (game->size == targetBoardsize) {
					bot.reset();

					point::color player = point::color::Black;
					unsigned count = 0;

					// TODO: add rotations and reflections
					for (auto& move : game->moves) {
						if (move == "") break;
						if (count++ > 20) break;
						coordinate coord = {move[0] - 'a' + 1, move[1] - 'a' + 1};

						//hashes.emplace(bot.game.hash, coord);
						//hashes[bot.game.hash].push_back(coord);
						hashes[bot.game.hash].insert(coord);

						//printf("hash: 0x%016llx\n", bot.game.hash);
						bool valid = bot.make_move(
							budgie::move(budgie::move::types::Move,
							             coord,
							             player));

						player = other_player(player);

						if (!valid) {
							break;
						}
					}

					//bot.game.print();
					//printf(">>> final hash: 0x%016llu\n", bot.game.hash);

					//printf("Dumping game %s:\n", line.c_str());
					//game->print();
				} else {
					printf("<Skipped %s>\n", line.c_str());
				}
			}
		}
	}

	FILE *out = fopen(argv[2], "w");
	uint32_t version = 0x00000001;
	uint32_t magic   = 0xabadc0de;

	fwrite(&magic, 4, 1, out);
	fwrite(&version, 4, 1, out);
	fwrite(&targetBoardsize, 4, 1, out);

	for (auto& [k, v] : hashes) {
		// ignore small sample sizes
		//if (v.size() <= 1) continue;

		//printf("Moves for hash 0x%016llx: ", k);
		fwrite(&k, 8, 1, out);

		//std::sort(v.begin(), v.end());

		for (auto& c : v) {
			uint8_t a = c.first;
			uint8_t b = c.second;
			fwrite(&a, 1, 1, out);
			fwrite(&b, 1, 1, out);
			//printf("(%d, %d) ", c.first, c.second);
		}

		fputc(0, out);

		//printf("\n");
	}
	fclose(out);

	printf("\n\nDone, generated table with %lu hashes from %lu files.\n", hashes.size(), numFiles);

	return 0;
}
