#pragma once

#include <string>

#include "chess.hpp"
#include "constants.hpp"
#include "search.hpp"
#include "tt.hpp"

class Engine {
 public:
  Engine();
  void setPosition(const std::string &fen);

  void printBoard();

  void initilizeEngine();

  void makeMove(std::string move);

  void uci_loop();

  void handle_stop();

  void handle_go(std::istringstream &iss);

  void handle_positon(std::istringstream &iss);

 private:
  chess::Board board;
  TranspositionTable tt_helper;
  Search search;
};
