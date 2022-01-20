#pragma once

#include <budgie/mcts.hpp>
#include <budgie/move_queue.hpp>

namespace bdg::playouts {

void capture_weighted_playout(board *state, move_queue& queue);
void local_weighted_playout(board *state, move_queue& queue);
void random_playout(board *state, move_queue& queue);
void adjacent_3x3_playout(board *state, move_queue& queue);
void adjacent_5x5_playout(board *state, move_queue& queue);
void attack_enemy_groups_playout(board *state, move_queue& queue);
void save_atari_playout(board *state, move_queue& queue);

// bdg::playouts
}
