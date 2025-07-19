#pragma once

#include "chess.hpp"
#include "evaluation.hpp"
#include "time_manager.hpp"
#include "tt.hpp"
#include <atomic>
#include <chrono>
#include <string>
#include <vector>

using namespace chess;

class Search {
public:
  Search(Board &board, TimeManager &time_manager, TranspositionTable &tt_helper,
         Evaluation &evaluator, bool time_controls_enabled);
  std::string searchBestMove();
  void stopSearch() { stopSearchFlag = true; }
  void setTimeControlsEnabled(bool enabled) { time_controls_enabled = enabled; }

private:
  Board &board;
  TimeManager &time_manager;
  TranspositionTable &tt_helper;
  Evaluation &evaluator;
  std::atomic<bool> stopSearchFlag{false};
  long long positionsSearched = 0;

  // Time management
  bool time_controls_enabled;
  long long soft_time_limit = 0;
  long long hard_time_limit = 0;
  int best_move_changes = 0;
  Move last_iteration_best_move = Move::NULL_MOVE;
  std::chrono::steady_clock::time_point search_start_time;

  int minmax(int depth, int alpha, int beta, bool isMaximizing,
             std::vector<Move> &pv, int ply);
  int quiescenceSearch(int alpha, int beta, bool isMaximizing, int ply);
  void orderMoves(Movelist &moves, Move tt_move, int ply);
  void orderQuiescMoves(Movelist &moves);
  int evaluate(int ply);
  bool isGameOver(const chess::Board &board);
  GameResultReason getGameOverReason(const chess::Board &board);
  int getPieceValue(Piece piece);
};
