#include "engine.hpp"

#include "piece-maps.hpp"

Engine::Engine()
    : board(), evaluator(), tt_helper(), time_manager(),
      search(board, time_manager, tt_helper, evaluator) {}

void Engine::printBoard() { std::cout << board; }

void Engine::setPosition(const std::string &fen) { board.setFen(fen); }

void Engine::initilizeEngine() {
  board.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Engine::setSearchLimits(int wtime, int btime, int winc, int binc,
                             int movestogo, int movetime) {
  time_manager.setTimevalues(wtime, btime, winc, binc, movestogo, movetime,
                             false);
}

void Engine::makeMove(std::string move) {
  Move parsedMove = uci::uciToMove(board, move);
  board.makeMove(parsedMove);
}

std::string Engine::getBestMove() { return search.searchBestMove(); }

void Engine::stopSearch() { search.stopSearch(); }