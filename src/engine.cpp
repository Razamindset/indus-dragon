#include "engine.hpp"

#include "piece-maps.hpp"

Engine::Engine()
    : board(), evaluator(), tt_helper(), time_manager(),
      search(board, time_manager, tt_helper, evaluator, false) {}

void Engine::printBoard() { std::cout << board; }

void Engine::setPosition(const std::string &fen) { board.setFen(fen); }

void Engine::initilizeEngine() {
  board.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Engine::setSearchLimits(int wtime, int btime, int winc, int binc, int movestogo,
                             int movetime) {
  bool time_controls_enabled = (wtime > 0 || btime > 0 || movetime > 0);
  time_manager.setTimevalues(wtime, btime, winc, binc, movestogo, movetime, false);
  search.setTimeControlsEnabled(time_controls_enabled);
}

void Engine::makeMove(std::string move) {
  Move parsedMove = uci::uciToMove(board, move);
  board.makeMove(parsedMove);
}

void Engine::getBestMove() { search.searchBestMove(); }

void Engine::stopSearch() { search.stopSearch(); }