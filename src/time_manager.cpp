#include <algorithm>

#include "search.hpp"

int Search::count_pieces(const chess::Board &board) {
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
void Search::calculateSearchTime() {
  // infinite search
  if (wtime <= 0 && btime <= 0 && movetime <= 0) {
    soft_time_limit = INFINITE_TIME;
    hard_time_limit = INFINITE_TIME;
    return;
  }

  // Movetime given
  if (movetime > 0) {
    soft_time_limit = movetime;
    hard_time_limit = movetime;
    return;
  }

  long long remaining_time =
      (board.sideToMove() == chess::Color::WHITE ? wtime : btime);
  long long increment =
      (board.sideToMove() == chess::Color::WHITE ? winc : binc);

  // Estimate moves left, default: 20
  int moves_remaining = movestogo > 0 ? movestogo : estimateMovesToGo(board);

  // Very low on time
  if (remaining_time < SAFETY_BUFFER) {
    soft_time_limit = hard_time_limit = MIN_SEARCH_TIME;
    return;
  }

  // Distribute remaining time across moves
  long long effective_time =
      remaining_time - SAFETY_BUFFER + increment * (moves_remaining - 1);
  long long base_time = effective_time / moves_remaining;

  // Apply factors
  long long soft_time =
      std::max<long long>(base_time * SOFT_TIME_FACTOR, MIN_SEARCH_TIME);
  long long hard_time =
      std::max<long long>(soft_time * HARD_TIME_FACTOR, MIN_SEARCH_TIME);

  // Don’t exceed safe bound
  long long max_time = remaining_time - SAFETY_BUFFER;
  soft_time_limit = std::min(soft_time, max_time);
  hard_time_limit = std::min(hard_time, max_time);
}

// Estimate moves remaining based on game phase
int Search::estimateMovesToGo(const chess::Board &board) {
  int piece_count = count_pieces(board);

  int full_moves = board.fullMoveNumber();

  // Opening
  if (piece_count >= 24) {
    return std::max(35 - full_moves / 2, 25);
  }
  // Middlegame
  else if (piece_count >= 12) {
    return std::max(25 - full_moves / 3, 15);
  }
  // Endgame
  else {
    return std::max(15 - full_moves / 4, 8);
  }
}

void Search::setTimevalues(int wtime, int btime, int winc, int binc,
                           int movestogo, int movetime) {
  this->wtime = static_cast<long long>(wtime);
  this->btime = static_cast<long long>(btime);
  this->winc = static_cast<long long>(winc);
  this->binc = static_cast<long long>(binc);
  this->movestogo = static_cast<long long>(movestogo);
  this->movetime = static_cast<long long>(movetime);

  if (this->wtime <= 0 && this->btime <= 0 && this->movetime <= 0) {
    this->time_controls_enabled = false;
  } else {
    this->time_controls_enabled = true;
  }
}

bool Search::manageTime(long long elapsed_time) {
  if (!time_controls_enabled) {
    return false;
  }

  if (elapsed_time >= soft_time_limit) {
    if (best_move_changes >= 2 && elapsed_time < hard_time_limit / 3) {
      soft_time_limit += soft_time_limit * 0.3;
      best_move_changes = 0;
      return false;  // Continue searching
    }
    return true;  // Stop searching
  }
  return false;  // Don't stop yet
}

bool Search::checkHardTimeLimit() {
  if (time_controls_enabled) {
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                            current_time - search_start_time)
                            .count();
    if (elapsed_time >= hard_time_limit) {
      stopSearchFlag = true;
      return true;
    }
  }
  return false;
}
