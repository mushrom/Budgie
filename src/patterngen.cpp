#include <budgie/mcts.hpp>
#include <budgie/distributed_mcts.hpp>
#include <budgie/distributed_client.hpp>
#include <budgie/gtp.hpp>
#include <budgie/args_parser.hpp>
#include <iostream>
#include <thread>
#include <time.h>
#include <stdio.h>

#include <budgie/budgie.hpp>
#include <budgie/parameters.hpp>

using namespace mcts_thing;

static args_parser::default_map patgen_options = {
	// general game settings
	{"boardsize",
		{"9", "Initial board size, currently only square boards are supported.",
			{"<any integer greater than 0>"}}},
	{"komi",
		{"6.5", "Komi given to the white player",
			{"<any real number>"}}},

	// general interface/behaviour settings
	{"pass",
		{"false", "Determine whether to detect when to pass. Good for humans, bad for bot matches.",
			{"false", "true"}}},
	{"ogs_output",
		{"false", "Outputs statistics parsed by gtp2ogs for malkovich chat.",
			{"false", "true"}}},

	// AI settings
	{"mode",
		{"gtp", "Interface mode to use",
			{"gtp", "distributed-gtp", "distributed-worker"}}},
	{"playouts",
		{"10000", "Number of playouts to use per move",
			{"<any postive integer>"}}},

	{"joseki_db",
		{"", "List of filenames containing joseki hash tables",
			{"<comma-delimited list of filenames>"}}},
	{"tree_policy",
		{"uct-rave", "Policy to use for tree traversal",
			{"uct-rave", "uct", "mcts"}}},
	{"playout_policy",
		{"capture_enemy_ataris,save_own_ataris,attack_enemy_groups,adjacent-3x3,random",
			"List of strategies to use when playing out games",
			{"random", "adjacent-3x3", "adjacent-5x5", "local_weighted", "save_own_ataris",
				"capture_enemy_ataris", "attack_enemy_groups"}}},
	{"patterns",
		{"patterns.txt", "Local pattern database file (gnugo format)",
			{"patterns.txt"}}},
	{"parameters",
		{"", "comma-separated list of key=value pairs to control AI behaviour.",
			{"key=value,..."}}},

	// distributed mode configuration
	{"server-host",
		{"tcp://localhost:5555", "Master server using the distributed mode",
			{"<ZMQ socket string>"}}},
	{"worker-threads",
		{"1", "Number of workers to start as a distributed client",
			{"0 for auto", "<any unsigned integer>"}}},

	{"outfile",
		{"", "Output file",
			{""}}},

	{"iterations",
		{"20", "Number of games to play",
			{""}}},
};

void print_help(void) {
	printf("Usage: ./budgie [options]\n");

	for (auto& x : patgen_options) {
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

	printf("\n");
	printf("Available value parameters: (type:key=default value)\n");

	for (int i = 1; intParamNames[i]; i++) {
		printf("    int:%s=%d\n", intParamNames[i], intParameters[i]);
	}

	for (int i = 1; boolParamNames[i]; i++) {
		const char *val = boolParameters[i]? "true" : "false";
		printf("    bool:%s=%s\n", boolParamNames[i], val);
	}

	for (int i = 1; floatParamNames[i]; i++) {
		printf("    float:%s=%g\n", floatParamNames[i], floatParameters[i]);
	}
}

int main(int argc, char *argv[]) {
	srand(time(NULL));
	args_parser args(argc, argv, patgen_options);
	std::string patfile = args.options["outfile"];
	std::cerr << args.options["iterations"] << std::endl;
	int iterations = stoi(args.options["iterations"]);


	for (auto& arg : args.arguments) {
		if (arg == "--help" || arg == "-h") {
			print_help();
			return 0;
		}
	}

	if (patfile.empty()) {
		std::cerr << "Need output file" << std::endl;
		return 1;
	}

	FILE *fp = fopen(patfile.c_str(), "w");
	if (!fp) {
		std::cerr << "couldn't open file" << std::endl;
		return 1;
	}

	static uint32_t wins[1 << 18];
	static uint32_t traversals[1 << 18];

	static unsigned blackwins = 0;
	static unsigned games = 0;

	std::vector<std::pair<point::color, uint32_t>> patterns;

	playout_probe_func playfunc = [&] (board *state, mcts_node *ptr, const coordinate& next) {
		point::color grid[9];
		read_grid(state, next, grid);
		uint32_t hash = hash_grid(state, grid);
		patterns.push_back({state->current_player, hash});
	};

	tree_probe_func treefunc = [&] (board *state, mcts_node *ptr) {
		point::color winner = state->determine_winner();

		for (auto& [color, hash] : patterns) {
			wins[hash]       += color == winner;
			traversals[hash] += 1;
		}

		patterns.clear();
	};

	for (int k = 0; k < iterations; k++) {
		point::color winner = point::color::Invalid;

		budgie bot(args.options);
		bot.tree->playout_probe = playfunc;
		//bot.tree->finished_probe = treefunc;

		while (true) {
			budgie::move m = bot.genmove();

			printf("### Iteration %d/%d: move %u, black: %u, white: %u\n",
			       k, iterations, bot.game.moves, blackwins, games-blackwins);

			/*
			if (m.type == budgie::move::types::Pass
			 || m.type == budgie::move::types::Resign)
			{
				break;
			}
			*/

			if (m.type == budgie::move::types::Resign) {
				winner = other_player(bot.game.current_player);
				break;

			} else if (m.type == budgie::move::types::Pass) {
				winner = bot.game.determine_winner();
				break;

			} else {
				bot.game.make_move(m.coord);
				bot.game.print();
				printf("\n");
			}
		}

		for (auto& [color, hash] : patterns) {
			wins[hash]       += color == winner;
			traversals[hash] += 1;
		}

		patterns.clear();

		blackwins += winner == point::color::Black;
		games     += 1;
	}

	for (unsigned i = 0; i < (1 << 18); i++) {
		if (traversals[i] > 0) {
			printf("hash: 0x%06x, wins: %u, traversals: %u\n", i, wins[i], traversals[i]);

			char grid[9];
			const char *asdf = ".OX+";

			for (int k = 0; k < 9; k++) {
				grid[8-k] = asdf[(i >> (2*k)) & 3];
			}

			for (int y = 0; y < 3; y++) {
				for (int x = 0; x < 3; x++) {
					if (y == 1 && x == 1) {
						fputc('*', fp);
						//putchar('*');
					} else {
						//putchar(grid[y*3 + x]);
						fputc(grid[y*3 + x], fp);
					}
				}
				fputc('\n', fp);
				//putchar('\n');
			}

			fprintf(fp, ":%u\n\n", (uint8_t)(200 * (float(wins[i]) / traversals[i])));
		}
	}

	printf("results: black: %u/%u, white: %u/%u\n", blackwins, games, games-blackwins, games);

	/*

	if (args.options["mode"] == "gtp"
	    || args.options["mode"] == "distributed-gtp")
	{
		gtp_client gtp;
		gtp.repl(args.options);
	}

	else if (args.options["mode"] == "distributed-worker") {
		distributed_client client(args.options);
		client.run();
	}

	else {
		std::cerr << "Unknown mode " << args.options["mode"] << std::endl;
		return 1;
	}
	*/

	return 0;
}
