#pragma once

#include <budgie/mcts.hpp>

namespace bdg::playouts {

maybe_coord capture_weighted_playout(board *state);
maybe_coord local_weighted_playout(board *state);
maybe_coord random_playout(board *state);
maybe_coord adjacent_3x3_playout(board *state);
maybe_coord adjacent_5x5_playout(board *state);
maybe_coord attack_enemy_groups_playout(board *state);
maybe_coord save_atari_playout(board *state);

// bdg::playouts
}
