#pragma once

#include "chess.hpp"
#include "constants.hpp"

// This will keep track of 2 good moves at each depth that cause beta cutoffs.
chess::Move killerMoves[MAX_SEARCH_DEPTH][2];
void clearKiller() {
  for (int i = 0; i < MAX_SEARCH_DEPTH; ++i) {
    killerMoves[i][0] = chess::Move::NULL_MOVE;
    killerMoves[i][1] = chess::Move::NULL_MOVE;
  }
}