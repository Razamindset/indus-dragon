#include "evaluation.hpp"
#include "nnue/nnue.h"

Evaluation::Evaluation() {}

int Evaluation::evaluate(const chess::Board &board) {
  return nnue_evaluate_fen(board.getFen().c_str());
}