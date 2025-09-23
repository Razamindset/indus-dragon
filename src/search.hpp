#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "chess.hpp"
#include "constants.hpp"
#include "evaluation.hpp"
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

  void communicate();

 private:
  Board &board;

  TranspositionTable &tt_helper;

  Evaluation evaluator;

  bool stopSearchFlag = false;

  long long positionsSearched = 0;

  // Heuristics
  chess::Move killerMoves[MAX_SEARCH_DEPTH][2];

  void clearKiller();

  int historyTable[2][64][64];

  void clearHistory();

  // Search
  std::vector<std::vector<Move>> pvTable;

  int negamax(int depth, int alpha, int beta, int ply, bool is_null);

  int quiescenceSearch(int alpha, int beta, int ply);

  void orderMoves(Movelist &moves, Move tt_move, int ply, bool isQuiescence);

  int evaluate();

  bool isGameOver(const chess::Board &board);

  GameResultReason getGameOverReason(const chess::Board &board);

  int getPieceValue(Piece piece);

  void printInfoLine(int eval, std::vector<Move> pv, int currentDepth,
                     long long nps, long long elapsedTime);

  bool storeLogs = false;

  // Time management
  bool timeEnabled = false;

  long long softTime = 0;

  long long hardTime = 0;

  int moveChanges = 0;

  std::chrono::steady_clock::time_point startTime;

  bool manageTime(const long long elapsedTime);

  bool checkHardTimeLimit();

  void calculateSearchTime();

  int estimateMovesToGo(const chess::Board &board);

  int countPieces(const chess::Board &board);

  long long getElapsedTime();

  long long wtime = 0;
  long long btime = 0;
  long long winc = 0;
  long long binc = 0;
  long long movestogo = 0;
  long long movetime = 0;
};
