#pragma once

#include <budgie/mcts.hpp>

namespace bdg::policies {

maybe_nodeptr uct_tree_policy(board *state, mcts_node *ptr);
maybe_nodeptr uct_rave_tree_policy(board *state, mcts_node *ptr);
maybe_nodeptr mcts_tree_policy(board *state, mcts_node *ptr);

// bdg::playouts
}
