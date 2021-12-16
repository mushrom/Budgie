#include <stdio.h>
#include <vector>
#include <string>
#include <optional>
#include <ctype.h>

#include <budgie/sgf.hpp>

namespace mcts_thing::sgf {

int iseof(int c) {
	return c == EOF;
}

int expect(FILE *fp, tokenTest test, const char *msg) {
	int c = fgetc(fp);
	while (isspace(c)) c = fgetc(fp);
	
	if (test(c)) {
		//printf("expected '%c'\n", c);
		return c;

	} else {
		ungetc(c, fp);
		fprintf(stderr, "unmet expectation :/ (%s)\n", msg);
		return 0;
	}
}

int accept(FILE *fp, tokenTest test) {
	int c = fgetc(fp);

	while (isspace(c)) c = fgetc(fp);

	if (test(c)) {
		//printf("accepted '%c'\n", c);
		return c;
	} else {
		ungetc(c, fp);
		return 0;
	}
}

unsigned collect(FILE *fp, parseFunction func, parseNode& root) {
	unsigned collected = 0;
	parseResult foo = func(fp);

	while (foo.has_value()) {
		collected++;
		root.leaves.push_back(*foo);
		foo = func(fp);
	}

	return collected;
}

int isLeftParen(int c) { return c == '('; }
int isRightParen(int c) { return c == ')'; }
int isLeftBracket(int c) { return c == '['; }
int isRightBracket(int c) { return c == ']'; }
int isSemicolon(int c) { return c == ';'; }

parseResult parseIdent(FILE *fp) {
	int c = accept(fp, isalpha);
	if (!c) return {};

	parseNode ret = {IDENTIFIER};
	ret.data += c;

	while ((c = accept(fp, isalpha))) {
		ret.data += c;
	}

	return ret;
}

parseResult parseValue(FILE *fp) {
	if (!accept(fp, isLeftBracket)) return {};

	parseNode ret = {VALUE};

	char c = fgetc(fp);
	while (c != ']') {
		if (feof(fp)) {
			fprintf(stderr, "Reached EOF while parsing value (missing a ']')");
			return {};
		}

		ret.data += c;
		c = fgetc(fp);
	}

	//if (!expect(fp, isRightBracket, "value: right bracket")) return {};

	return ret;
}

parseResult parseProperty(FILE *fp) {
	parseResult ident = parseIdent(fp);
	if (!ident.has_value()) return {};

	parseNode ret = {PROPERTY};
	ret.leaves.push_back(*ident);

	// need at least one value for a valid property
	if (collect(fp, parseValue, ret) == 0)
		return {};

	return ret;
}

parseResult parseSGFNode(FILE *fp) {
	if (!accept(fp, isSemicolon)) return {};

	parseNode ret = {NODE};
	collect(fp, parseProperty, ret);

	return ret;
}

parseResult parseSequence(FILE *fp) {
	parseNode ret = {SEQUENCE};
	collect(fp, parseSGFNode, ret);
	return ret;
}

parseResult parseGameTree(FILE *fp) {
	if (!accept(fp, isLeftParen)) return {};

	parseNode ret = {GAME_TREE};
	parseResult temp = parseSequence(fp);

	if (temp.has_value())
		ret.leaves.push_back(*temp);

	collect(fp, parseGameTree, ret);

	if (expect(fp, isRightParen, "game tree: right paren")) {
		return ret;
	} else {
		return {};
	}
}

parseResult parseSGF(FILE *fp) {
	parseNode ret = {SGF};
	collect(fp, parseGameTree, ret);

	return ret;
}

void dump(parseNode res, int indent) {
	for (int i = 0; i < indent * 4; i++) putchar(' ');

	printf("type: %s, data: %s\n", parseStrings[res.type], res.data.c_str());

	for (auto& leaf : res.leaves) {
		dump(leaf, indent + 1);
	}
}

// namespace mcts_thing::sgf
}
