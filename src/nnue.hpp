#pragma once

#include "chess.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <algorithm>
#include <cmath>

namespace NNUE {
  // First define some constants
  constexpr int INPUT_FEATURES = 768;
  constexpr int HIDDEN_SIZE = 256;
  constexpr int SCALE = 255;

  alignas(32) extern int16_t FEATURE_WEIGHTS[INPUT_FEATURES][HIDDEN_SIZE];
  alignas(32) extern int16_t FEATURE_BIAS[HIDDEN_SIZE];
  alignas(32) extern int16_t OUTPUT_WEIGHTS[HIDDEN_SIZE];
  alignas(32) extern int16_t OUTPUT_BIAS;
    
  struct alignas(32) Accumulator {
    std::array<int16_t, HIDDEN_SIZE>values;

    Accumulator() {
        values.fill(0);
    }
  };

  class Network {
  public:
    void load_network(const std::string& path);
    void refreshAccumulator(const chess::Board& board, Accumulator& acc);
    // NEW: Efficiently updatable refresher
    void updateAccumulator(const chess::Board& board, chess::Move move, Accumulator& acc);
    void updateAccumulator();
    int evaluate(chess::Color stm, const Accumulator& acc) const;

  private:
    static int sigmoidToCp(float prob);

  };

  inline int getPieceIndex(chess::Color c, chess::PieceType pt, chess::Square sq){
    // Python dataset reads FEN from A8 -> H1.
    // chess libs use A1 = 0, so we flip ranks.
    return  (static_cast<int>(c) * 6 + static_cast<int>(pt)) * 64 + (sq.index() ^ 56);
  }


} // namespace NNUE
