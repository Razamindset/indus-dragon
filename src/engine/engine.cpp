#include "engine.hpp"
#include "piece-maps.hpp"

void Engine::printBoard() { std::cout << board; }

void Engine::setPosition(const std::string& fen) { board.setFen(fen); }

void Engine::initilizeEngine() {
  board.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Engine::makeMove(std::string move) {
  Move parsedMove = uci::uciToMove(board, move);
  board.makeMove(parsedMove);
}

int Engine::getPieceValue(Piece piece) {
  switch (piece) {
    case PieceGenType::PAWN:
      return 100;
    case PieceGenType::KNIGHT:
      return 300;
    case PieceGenType::BISHOP:
      return 320;
    case PieceGenType::ROOK:
      return 500;
    case PieceGenType::QUEEN:
      return 900;
    default:
      return 0;  // King has no material value
  }
}