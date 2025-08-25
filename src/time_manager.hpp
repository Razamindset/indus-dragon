#pragma once
#include "chess.hpp"
#include "constants.hpp"

struct CalculatedTime {
  long long soft_time;
  long long hard_time;
};

class TimeManager {
 public:
  void setTimevalues(int wtime, int btime, int winc, int binc, int movestogo,
                     int movetime, bool is_infinite);
  CalculatedTime calculateSearchTime(chess::Board &board);

 private:
  int estimateMovesToGo(const chess::Board &board);
  double getPositionFactor(const chess::Board &board);
  double getEvaluationFactor(const chess::Board &board);
  int count_pieces(const chess::Board &board) {
    return board.pieces(chess::PieceType::PAWN, chess::Color::WHITE).count() +
           board.pieces(chess::PieceType::PAWN, chess::Color::BLACK).count() +
           board.pieces(chess::PieceType::KNIGHT, chess::Color::WHITE).count() +
           board.pieces(chess::PieceType::KNIGHT, chess::Color::BLACK).count() +
           board.pieces(chess::PieceType::BISHOP, chess::Color::WHITE).count() +
           board.pieces(chess::PieceType::BISHOP, chess::Color::BLACK).count() +
           board.pieces(chess::PieceType::ROOK, chess::Color::WHITE).count() +
           board.pieces(chess::PieceType::ROOK, chess::Color::BLACK).count() +
           board.pieces(chess::PieceType::QUEEN, chess::Color::WHITE).count() +
           board.pieces(chess::PieceType::QUEEN, chess::Color::BLACK).count();
  }

 private:
  long long wtime = 0;
  long long btime = 0;
  long long winc = 0;
  long long binc = 0;
  long long movestogo = 0;
  long long movetime = 0;
  bool is_infinte = false;

  static constexpr long long INFINITE_TIME = 1000000000LL;  // 1 billion ms
  static constexpr double SOFT_TIME_FACTOR =
      0.4;  // Use 40% of allocated time normally
  static constexpr double HARD_TIME_FACTOR = 2.5;  // Maximum 2.5x soft time
  static constexpr double PANIC_THRESHOLD =
      0.1;  // Panic mode when <10% time left
  static constexpr double ENDGAME_FACTOR = 1.2;   // Spend more time in endgame
  static constexpr double IN_CHECK_FACTOR = 1.2;  // Spend more time in endgame
  static constexpr long long MIN_SEARCH_TIME = 10;  // Minimum 10ms search
  static constexpr long long SAFETY_BUFFER = 50;    // Keep 50ms safety buffer
};
