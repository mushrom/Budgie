#include <budgie/budgie.hpp>
#include <budgie/pattern_db.hpp>
#include <budgie/distributed_mcts.hpp>
#include <budgie/distributed_client.hpp>
// TODO: only included for 'split_string()', should split that in a util file
#include <budgie/gtp.hpp>
#include <iostream>

#include <budgie/parameters.hpp>

namespace mcts_thing {

budgie::budgie(args_parser::option_map& opts)
	: pool(stoi(opts["worker-threads"]))
{
	// cache options
	options = opts;

	// initlaize tree state
	tree = init_mcts(opts);

	// initialize game state
	komi = stof(opts["komi"]);
	boardsize = stoi(opts["boardsize"]);
	playouts = stoi(opts["playouts"]);
	//full_traversals = stoi(opts["node_expansion_threshold"]);

	game.reset(boardsize, komi);
	game.loadJosekis(opts["joseki_db"]);

	// parse parameter key=value pairs
	std::string& params = opts["parameters"];
	size_t start = 0;
	size_t end   = params.find(";");

	while (start < params.length()) {
		std::string foo = params.substr(start, end);
		size_t eq   = foo.find("=");
		size_t type = foo.find(":");

		if (eq == std::string::npos || type == std::string::npos)
			continue;

		std::string typestr = foo.substr(0, type);
		std::string key     = foo.substr(type + 1, eq - type - 1);
		std::string val     = foo.substr(eq + 1);

		std::cerr << "have parameter: "
			<< foo << ": "
			<< "key: " << key << ", "
			<< "type: " << typestr << ", "
			<< "value: " << val << ", "
			<< std::endl;

		auto unknown = [&] () {
			std::cerr << "Unknown key value: " << key
				<< " (for type " << typestr << ")"
				<< std::endl;
		};

		if (typestr == "int") {
			if (int idx = findParam(intParamNames, key.c_str())) {
				setInt(idx, int(stoi(val)));
			} else unknown();

		} else if (typestr == "bool") {
			if (int idx = findParam(boolParamNames, key.c_str())) {
				bool asdf = val == "true";
				setBool(idx, asdf);
			} else unknown();

		} else if (typestr == "float") {
			if (int idx = findParam(boolParamNames, key.c_str())) {
				setFloat(idx, stof(val));
			} else unknown();

		} else {
			std::cerr << "Unknown type: " << typestr << std::endl;
		}

		if (end == std::string::npos)
			break;

		start = end + 1;
		end   = params.find(",", start);
	}
}

bool budgie::make_move(move m) {
	switch (m.type) {
		case budgie::move::types::Pass:
			passed = true;
			break;

		case budgie::move::types::Move:
			passed = false;
			game.current_player = m.player;
			return game.make_move(m.coord);
			break;

		default:
			break;
	}

	return true;
}

bool budgie::make_move(const coordinate& c) {
	return make_move(move(move::types::Move, c, game.current_player));
}

budgie::move budgie::genmove(void) {
	// TODO: passing heuristics
	if (passed) {
		return move(budgie::move::types::Pass);
	}

	tree->reset();
	coordinate coord = tree->do_search(&game, pool, playouts);
	float winrate = tree->win_rate(coord);

	// TODO: make resign threshold configurable
	if (winrate < 0.15) {
		return move(move::types::Resign);

	} else if ((woncount >= 3 && winrate >= 1.0) || tree->ownership_settled(&game)) {
		return move(move::types::Pass);

	// TODO: option to pass or play out to the bitter end
	} else if (!game.is_valid_move(coord)) {
		return move(move::types::Pass);

	} else {
		woncount = (winrate >= 1.0)? woncount + 1 : 0;

		return move(move::types::Move,
		            coord,
		            game.current_player);
	}
}

void budgie::reset(void) {
	tree->reset();
	game.reset(boardsize, komi);
	passed = false;
}

void budgie::set_player(point::color p) {
	game.current_player = p;
}

std::unique_ptr<mcts> budgie::init_mcts(args_parser::option_map& options) {
	pattern_dbptr db = pattern_dbptr(new pattern_db(options["patterns"]));

	tree_policy *tree_pol;
	if (options["tree_policy"] == "mcts") {
		tree_pol = new mcts_tree_policy(db);
	} else if (options["tree_policy"] == "uct") {
		tree_pol = new uct_tree_policy(db);
	} else {
		tree_pol = new uct_rave_tree_policy(db);
	}

	std::list<playout_strategy*> strats;
	auto policies = mcts_thing::split_string(options["playout_policy"]);

	for (auto policy : policies) {
		if (policy == "local_weighted") {
			strats.push_back(new local_weighted_playout(db));

		} else if (policy == "adjacent-3x3") {
			strats.push_back(new adjacent_3x3_playout(db));

		} else if (policy == "adjacent-5x5") {
			strats.push_back(new adjacent_5x5_playout(db));

		} else if (policy == "capture_enemy_ataris") {
			strats.push_back(new capture_weighted_playout(db));

		} else if (policy == "attack_enemy_groups") {
			strats.push_back(new attack_enemy_groups_playout(db));

		} else if (policy == "save_own_ataris") {
			strats.push_back(new save_atari_playout(db));

		} else if (policy == "random"){
			strats.push_back(new random_playout(db));

		} else {
			std::cerr << __func__ << ": unknown playout policy \""
				<< policy << "\"" << std::endl;
		}
	}

	if (options["mode"] == "distributed-gtp") {
		return std::unique_ptr<mcts>(new distributed_mcts(tree_pol, strats));

	} else {
		return std::unique_ptr<mcts>(new mcts(tree_pol, strats));
	}
}

// namespace mcts_thing
}
