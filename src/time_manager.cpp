#include <algorithm>

#include "search.hpp"

int Search::countPieces(const chess::Board &board) {
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
    softTime = INFINITE_TIME;
    hardTime = INFINITE_TIME;
    return;
  }

  // Movetime given
  if (movetime > 0) {
    softTime = movetime;
    hardTime = movetime;
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
    softTime = hardTime = MIN_SEARCH_TIME;
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
  softTime = std::min(soft_time, max_time);
  hardTime = std::min(hard_time, max_time);
}

// Estimate moves remaining based on game phase
int Search::estimateMovesToGo(const chess::Board &board) {
  int piece_count = countPieces(board);

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
    this->timeEnabled = false;
  } else {
    this->timeEnabled = true;
  }
}

bool Search::manageTime(const long long elapsedTime) {
  if (!timeEnabled) {
    return false;
  }

  if (elapsedTime >= softTime) {
    if (moveChanges >= 2 && elapsedTime < hardTime / 3) {
      softTime += softTime * 0.3;
      moveChanges = 0;
      return false;  // Continue searching
    }
    return true;  // Stop searching
  }
  return false;  // Don't stop yet
}

bool Search::checkHardTimeLimit() {
  if (timeEnabled) {
    if (getElapsedTime() >= hardTime) {
      stopSearchFlag = true;
      return true;
    }
  }
  return false;
}

long long Search::getElapsedTime() {
  auto current = std::chrono::steady_clock::now();
  auto elapsedTime =
      std::chrono::duration_cast<std::chrono::milliseconds>(current - startTime)
          .count();
  return elapsedTime;
}