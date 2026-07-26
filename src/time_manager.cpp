#include <algorithm>

#include "search.hpp"

int Search::countPieces(const chess::Board &board) const {
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

SearchTime Search::calculateSearchTime() const {
  if (wtime <= 0 && btime <= 0 && movetime <= 0) {
    return {INFINITE_TIME, INFINITE_TIME};
  }

  if (movetime > 0) {
    return {movetime, movetime};
  }

  long long remainingTime =
      board.sideToMove() == chess::Color::WHITE ? wtime : btime;

  long long increment =
      board.sideToMove() == chess::Color::WHITE ? winc : binc;

  int movesRemaining = movestogo > 0 ? movestogo : estimateMovesToGo(board);

  if (remainingTime < SAFETY_BUFFER) {
    return {MIN_SEARCH_TIME, MIN_SEARCH_TIME};
  }

  long long effectiveTime =
      remainingTime - SAFETY_BUFFER + increment * (movesRemaining - 1);

  long long baseTime = effectiveTime / movesRemaining;

  long long calculatedSoft =
      std::max<long long>(baseTime * SOFT_TIME_FACTOR, MIN_SEARCH_TIME);

  long long calculatedHard =
      std::max<long long>(calculatedSoft * HARD_TIME_FACTOR, MIN_SEARCH_TIME);

  long long maxTime = remainingTime - SAFETY_BUFFER;

  return {
      std::min(calculatedSoft, maxTime),
      std::min(calculatedHard, maxTime)
  };
}

// Estimate moves remaining based on game phase
int Search::estimateMovesToGo(const chess::Board &board) const {
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

void Search::setTimeValues(const GoOptions& options) {
  this->wtime = static_cast<long long>(options.wtime);
  this->btime = static_cast<long long>(options.btime);
  this->winc = static_cast<long long>(options.winc);
  this->binc = static_cast<long long>(options.binc);
  this->movestogo = static_cast<long long>(options.movestogo);
  this->movetime = static_cast<long long>(options.movetime);

  timeEnabled = options.hasTimeLimit();
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