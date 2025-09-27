#pragma once

#include "chess.hpp"

using namespace chess;

// Forward declaration
class Evaluation;

struct EvalContext {
  const chess::Board &board;
  Bitboard whitePawns, blackPawns;
  Bitboard whiteKnights, blackKnights;
  Bitboard whiteBishops, blackBishops;
  Bitboard whiteRooks, blackRooks;
  Bitboard whiteQueens, blackQueens;
  int whiteMaterial, blackMaterial;
  bool isEndgame;
};

class Evaluation {
 public:
  Evaluation() {};
  int evaluate(const Board &board);

 private:
  void evaluatePST(int &score, const EvalContext &ctx);
  void evaluatePawns(int &score, const EvalContext &ctx);
  void evaluateRooks(int &score, const EvalContext &ctx);
  void evaluateKingEndgameScore(int &score, const EvalContext &ctx);
  int manhattanDistance(Square s1, Square s2) {
    return std::abs(s1.file() - s2.file()) + std::abs(s1.rank() - s2.rank());
  }
};