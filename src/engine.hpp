#pragma once

#include <string>

#include "chess.hpp"
#include "constants.hpp"
#include "evaluation.hpp"
#include "search.hpp"
#include "time_manager.hpp"
#include "tt.hpp"

using namespace chess;

class Engine {
public:
  Engine();
  void setPosition(const std::string &fen);
  void printBoard();
  void initilizeEngine();
  void setSearchLimits(int wtime, int btime, int winc, int binc, int movestogo,
                     int movetime);

  void makeMove(std::string move);

  std::string getBestMove();

  void stopSearch();

private:
  Board board;
  Evaluation evaluator;
  TranspositionTable tt_helper;
  TimeManager time_manager;
  Search search;
};
