#pragma once

#include <chrono>

#include "chess.hpp"
#include "utils.hpp"

// Owns all time-control bookkeeping: parsing "go" time options, deciding
// soft/hard budgets for the current search, and answering "should we stop
// now?" during iterative deepening. Kept separate from Search so the search
// algorithm doesn't need to know how time budgets are computed.
class TimeManager {
 public:
  void setTimeValues(const GoOptions& options);

  // Call once per `go` command, right before the iterative deepening loop.
  void start(const chess::Board& board);

  // Call after each completed iterative-deepening iteration.
  // bestMoveChanged should be true if the best move differs from the
  // previous iteration's best move. Returns true if the search should stop.
  bool shouldStopAfterIteration(long long elapsedTime, bool bestMoveChanged);

  // Cheap check used inside negamax/qsearch to enforce the hard cutoff.
  bool hardLimitReached() const;

  long long elapsedMs() const;

 private:
  bool timeEnabled = false;

  long long softTime = 0;
  long long hardTime = 0;
  int moveChanges = 0;

  std::chrono::steady_clock::time_point startTime;

  long long wtime = 0;
  long long btime = 0;
  long long winc = 0;
  long long binc = 0;
  long long movestogo = 0;
  long long movetime = 0;

  SearchTime calculateSearchTime(const chess::Board& board) const;
  int estimateMovesToGo(const chess::Board& board) const;
  int countPieces(const chess::Board& board) const;
};