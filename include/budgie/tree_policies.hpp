#pragma once

#include <budgie/mcts.hpp>

namespace mcts_thing::policies {

maybe_nodeptr uct_tree_policy(board *state, mcts_node *ptr);
maybe_nodeptr uct_rave_tree_policy(board *state, mcts_node *ptr);
maybe_nodeptr mcts_tree_policy(board *state, mcts_node *ptr);

// mcts_thing::playouts
}
