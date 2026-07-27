#pragma once

#include <string>
#include <vector>

#include "utils.hpp"
#include "chess.hpp"
#include "constants.hpp"
#include "tt.hpp"
#include "nnue.hpp"
#include "time_manager.hpp"

class Search {
 public:
  Search(chess::Board &board, TranspositionTable &tt_helper);

  void searchBestMove();

  void stopSearch() { stopSearchFlag = true; }

  void setTimeValues(const GoOptions& options);

  void logMessage(const std::string &message);

  void toggleLogs() { storeLogs = !storeLogs; }

  void communicate();

  long long benchSearch(int depth);

 private:
  chess::Board &board;

  TranspositionTable &tt_helper;

  NNUE::Accumulator accStack[MAX_SEARCH_DEPTH];
  NNUE::Network nnue;

  bool stopSearchFlag = false;

  long long positionsSearched = 0;

  // Heuristics
  chess::Move killerMoves[MAX_SEARCH_DEPTH][2];

  void clearKiller();

  int historyTable[2][64][64];

  void clearHistory();

  // Search
  std::vector<std::vector<chess::Move>> pvTable;

  int negamax(int depth, int alpha, int beta, int ply, bool is_null);

  int qsearch(int alpha, int beta, int ply);

  void orderMoves(chess::Movelist &moves, chess::Move tt_move, int ply, bool isQuiescence);

  int evaluate(int ply);

  bool isGameOver(const chess::Board &board);

  chess::GameResultReason getGameOverReason(const chess::Board &board);

  int getPieceValue(chess::Piece piece);

  void printInfoLine(int eval, std::vector<chess::Move> pv, int currentDepth,
                     long long nps, long long elapsedTime);

  bool storeLogs = false;

  TimeManager timeManager;
  bool manageTime(long long elapsedTime, bool bestMoveChanged);
  bool checkHardTimeLimit();
  long long getElapsedTime();
};
