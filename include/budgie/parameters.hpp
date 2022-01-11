#pragma once

#include <cstring>

namespace mcts_thing {

enum {
	PARAM_INT_NONE = 0,
	PARAM_INT_RAVE_WEIGHT,
	PARAM_INT_NODE_EXPANSION_THRESHOLD,
	PARAM_INT_KO_DEPTH,

	PARAM_BOOL_NONE = 0,
	PARAM_BOOL_USE_WINRATE,
	PARAM_BOOL_USE_RAVE,
	PARAM_BOOL_USE_UCT,
	PARAM_BOOL_USE_CRITICALITY,
	PARAM_BOOL_USE_EXP_SCORE,
	PARAM_BOOL_USE_PATTERNS,
	PARAM_BOOL_VERY_VERBOSE,
	PARAM_BOOL_MCTS_INIT_PATTERNS,
	PARAM_BOOL_MCTS_INIT_100PCT,

	PARAM_FLOAT_NONE = 0,
	PARAM_FLOAT_UCT_WEIGHT,
	PARAM_FLOAT_SCORE_WEIGHT,
};

inline const char *intParamNames[] = {
	// integer parameter names
	"none",
	"rave_weight",
	"node_expansion_threshold",
	"ko_depth",
	nullptr
};

inline const char *boolParamNames[] = {
	// bool parameter names
	"none>",
	// tree search policy flags, determine what information to consider
	// when determining what nodes to explore
	"use_winrate",
	"use_rave",
	"use_uct",
	"use_criticality",
	"use_expected_score",
	"use_patterns",
	"very_verbose",
	"mcts_init_patterns",
	"mcts_init_100pct",
	nullptr
};

inline const char *floatParamNames[] = {
	// float parameter names
	"none",
	"uct_weight",
	"score_weight",
	nullptr
};

static inline int findParam(const char *names[], const char *target) {
	for (size_t i = 1; names[i]; i++) {
		if (strcmp(names[i], target) == 0) {
			return i;
		}
	}

	return 0;
}

inline int intParameters[] = {
	0,    // none
	1000, // rave weight
	8,    // node expansion threshold
	9,    // ko violation search depth
	      // large numbers search all move history (implementing a superko rule),
	      // 9 or so approximates a superko rule,
	      // 2 implements a simple ko rule
	      // 1 and 0 will hang during playouts, not recommended
};

inline bool boolParameters[] = {
	false, // none
	true,  // use winrate
	true,  // use RAVE
	true,  // use UCT
	false, // use criticality
	false, // use expected score
	true,  // use patterns
	false, // very verbose
	false, // initialize new MCTS nodes with the pattern database
	       // (initializes with even stats for every move otherwise)
	false, // initialize new MCTS nodes with 100% winrate
};

inline float floatParameters[] = {
	0.f,   // none
	0.10f, // UCT weight
	0.05f, // expected score weight
};

inline void setInt(unsigned which, int value)   { intParameters[which] = value; }
inline void setBool(unsigned which, bool value)  { boolParameters[which] = value; }
inline void setFloat(unsigned which, float value) { floatParameters[which] = value; }

inline int      getInt(unsigned which)   { return intParameters[which]; }
inline unsigned getUInt(unsigned which)  { return intParameters[which]; }
inline bool     getBool(unsigned which)  { return boolParameters[which]; }
inline float    getFloat(unsigned which) { return floatParameters[which]; }

// namespace mcts_thing
}
