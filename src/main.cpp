#include "engine.hpp"

int main() {
  Engine engine;

  engine.initilizeEngine();

  engine.uciLoop();

  return 0;
}

// #include "nnue.hpp"
// #include "chess.hpp"
// #include <iostream>

// int main() {
//     chess::Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB1KBNR w KQkq - 0 1");

//     NNUE::Accumulator acc;
//     NNUE::Network nnue;

//     nnue.load_network("./indus_dragon_v3.bin");

//     nnue.refreshAccumulator(board, acc);

//     std::cout << nnue.evaluate(board.sideToMove(), acc) << "\n";

//     return 0;
// }