#pragma once

#include <atomic>
#include <chrono>
#include <string>

#include "chess.hpp"
#include "constants.hpp"
#include "evaluation.hpp"
#include "time_manager.hpp"
#include "tt.hpp"

using namespace chess;

class Engine {
private:
  Board board;

  Evaluation evaluator;

  TranspositionTable tt_helper;

  TimeManager time_manager;

  std::atomic<bool> stopSearchFlag{false};
  int getPieceValue(Piece piece);
  long long positionsSearched = 0;

  // Time management
  bool time_controls_enabled = false;
  long long soft_time_limit = 0;
  long long hard_time_limit = 0;
  int best_move_changes = 0;
  Move last_iteration_best_move = Move::NULL_MOVE;
  std::chrono::steady_clock::time_point search_start_time;

  int evaluate(int ply);

  // Search
  int minmax(int depth, int alpha, int beta, bool isMaximizing, std::vector<Move> &pv, int ply);
  int quiescenceSearch(int alpha, int beta, bool isMaximizing, int ply);

  // Move ordering and heuristics
  void orderMoves(Movelist &moves, Move ttMove, int ply);
  void orderQuiescMoves(Movelist &moves);

public:
  void setPosition(const std::string &fen);
  void printBoard();
  void initilizeEngine();
  void setSearchLimits(int wtime, int btime, int winc, int binc, int movestogo, int movetime);

  // Client accessible get best move function
  std::string getBestMove();

  //* Game state related
  bool isGameOver(const chess::Board &board) {
    auto result = board.isGameOver();
    return result.second != chess::GameResult::NONE;
  }

  GameResultReason getGameOverReason(const chess::Board &board) {
    auto result = board.isGameOver();
    return result.first;
  }
  // Move making used when the chess gui gives a list of move history. use this
  // to set the board state correctly
  void makeMove(std::string move);

  void stopSearch() { stopSearchFlag = true; }
};
