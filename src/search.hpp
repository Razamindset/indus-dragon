#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <vector>

#include "chess.hpp"
#include "constants.hpp"
#include "tt.hpp"

using namespace chess;

class Search {
 public:
  Search(Board &board, TranspositionTable &tt_helper);

  void searchBestMove();

  void stopSearch() { stopSearchFlag = true; }

  void setTimevalues(int wtime, int btime, int winc, int binc, int movestogo,
                     int movetime);

  void logMessage(const std::string &message);

  void toggleLogs() { storeLogs = !storeLogs; }

 private:
  Board &board;

  TranspositionTable &tt_helper;

  std::atomic<bool> stopSearchFlag{false};

  long long positionsSearched = 0;

  // Heuristics
  chess::Move killerMoves[MAX_SEARCH_DEPTH][2];

  void clearKiller();

  // Search
  std::vector<std::vector<Move>> pvTable;

  int negamax(int depth, int alpha, int beta, int ply);

  int quiescenceSearch(int alpha, int beta, int ply);

  void orderMoves(Movelist &moves, Move tt_move, int ply, bool isQuiescence);

  int evaluate(int ply);

  bool isGameOver(const chess::Board &board);

  GameResultReason getGameOverReason(const chess::Board &board);

  int getPieceValue(Piece piece);

  void printInfoLine(int eval, std::vector<Move> pv, int currentDepth,
                     long long nps, long long elapsed_time);

  bool storeLogs = false;

  // Time management
  bool time_controls_enabled = false;

  long long soft_time_limit = 0;

  long long hard_time_limit = 0;

  int best_move_changes = 0;

  std::chrono::steady_clock::time_point search_start_time;

  bool manageTime(long long elapsed_time);

  bool checkHardTimeLimit();

  void calculateSearchTime();

  int estimateMovesToGo(const chess::Board &board);

  double getPositionFactor(const chess::Board &board);

  double getEvaluationFactor(const chess::Board &board);

  int count_pieces(const chess::Board &board);

  long long wtime = 0;
  long long btime = 0;
  long long winc = 0;
  long long binc = 0;
  long long movestogo = 0;
  long long movetime = 0;

  static constexpr long long INFINITE_TIME = 1000000000LL;  // 1 billion ms
  static constexpr double SOFT_TIME_FACTOR =
      0.4;  // Use 40% of allocated time normally
  static constexpr double HARD_TIME_FACTOR = 2.5;  // Maximum 2.5x soft time
  static constexpr double PANIC_THRESHOLD =
      0.1;  // Panic mode when <10% time left
  static constexpr double ENDGAME_FACTOR = 1.2;   // Spend more time in endgame
  static constexpr double IN_CHECK_FACTOR = 1.2;  // Spend more time in endgame
  static constexpr long long MIN_SEARCH_TIME = 10;  // Minimum 10ms search
  static constexpr long long SAFETY_BUFFER = 100;   // Keep 100ms safety buffer
};
