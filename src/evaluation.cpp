#include "evaluation.hpp"
#include "nnue/nnue.h"

Evaluation::Evaluation() { nnue_init("nn-04cf2b4ed1da.nnue"); }

int Evaluation::evaluate(const chess::Board &board) {
  return nnue_evaluate_fen(board.getFen().c_str());
}