#pragma once

#include <stdio.h>
#include <vector>
#include <string>
#include <optional>
#include <ctype.h>

namespace mcts_thing::sgf {

enum tokenType {
	LEFT_BRACKET,
	RIGHT_BRACKET,
	LEFT_PAREN,
	RIGHT_PAREN,
	SEMICOLON,
	PERIOD,
	LETTER,
	DIGIT,
	FILE_END,
};

struct token {
	enum tokenType type;
	char value;
};

enum parseType {
	NONE,
	VALUE,
	IDENTIFIER,
	PROPERTY,
	NODE,
	SEQUENCE,
	GAME_TREE,
	COLLECTION,
	SGF,
};

static const char *parseStrings[] = {
	"NONE",
	"VALUE",
	"IDENTIFIER",
	"PROPERTY",
	"NODE",
	"SEQUENCE",
	"GAME""_TREE",
	"COLLECTION",
	"SGF",
};

struct parseNode {
	enum parseType type;
	std::string data;
	std::vector<parseNode> leaves;

	parseType getLeafType(unsigned i) {
		if (i < leaves.size()) {
			return leaves[i].type;
		} else {
			return NONE;
		}
	}
};

typedef int (*tokenTest)(int c);
typedef std::optional<parseNode> parseResult;
typedef parseResult (*parseFunction)(FILE *fp);

parseResult parseSGF(FILE *fp);
void dump(parseNode res, int indent = 0);

// namespace mcts_thing::sgf
}
