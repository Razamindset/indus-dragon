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

// History Table
// 12 pieces 6 of each type and 64 squares.
int historyTable[12][64];
void clearHistoryTable() {
  for (int i = 0; i < 12; ++i) {
    for (int j = 0; j < 64; ++j) {
      historyTable[i][j] = 0;
    }
  }
}

void updateHistoryScore(chess::Piece piece, chess::Square to, int depth) {
  int pieceIndex = static_cast<int>(piece.type());
  historyTable[pieceIndex][to.index()] += depth * depth;
}