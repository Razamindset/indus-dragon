#include "time_manager.hpp"

#include <algorithm>

#include "constants.hpp"

int TimeManager::countPieces(const chess::Board &board) const {
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

SearchTime TimeManager::calculateSearchTime(const chess::Board &board) const {
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
int TimeManager::estimateMovesToGo(const chess::Board &board) const {
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

void TimeManager::setTimeValues(const GoOptions& options) {
  wtime = options.wtime;
  btime = options.btime;
  winc = options.winc;
  binc = options.binc;
  movestogo = options.movestogo;
  movetime = options.movetime;

  timeEnabled = options.hasTimeLimit();
}

void TimeManager::start(const chess::Board& board) {
  SearchTime budget = calculateSearchTime(board);
  softTime = budget.soft;
  hardTime = budget.hard;
  moveChanges = 0;
  startTime = std::chrono::steady_clock::now();
}

bool TimeManager::shouldStopAfterIteration(long long elapsedTime, bool bestMoveChanged) {
  if (bestMoveChanged) {
    moveChanges++;
  }

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

bool TimeManager::hardLimitReached() const {
  return timeEnabled && elapsedMs() >= hardTime;
}

long long TimeManager::elapsedMs() const {
  auto current = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(current - startTime).count();
}