#pragma once

#include "chess.hpp"

using namespace chess;

class Evaluation {
public:
  Evaluation() {};
  int evaluate(const Board &board);

private:
  void evaluatePST(const Board &board, int &eval, bool isEndgame);
  void evaluatePawns(int &eval, const Bitboard &whitePawns,
                     const Bitboard &blackPawns);
  void evaluateKingEndgameScore(const Board &board, int &eval);
  int manhattanDistance(Square s1, Square s2) {
    return std::abs(s1.file() - s2.file()) + std::abs(s1.rank() - s2.rank());
  }
};