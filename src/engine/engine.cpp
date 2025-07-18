#include "engine.hpp"

#include "piece-maps.hpp"

void Engine::printBoard() { std::cout << board; }

void Engine::setPosition(const std::string &fen) { board.setFen(fen); }

void Engine::initilizeEngine() {
  board.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Engine::setSearchLimits(int wtime, int btime, int winc, int binc,
                             int movestogo, int movetime) {
  this->wtime = wtime;
  this->btime = btime;
  this->winc = winc;
  this->binc = binc;
  this->movestogo = movestogo;
  this->movetime = movetime;

  if (wtime > 0 || btime > 0 || movetime > 0) {
    time_controls_enabled = true;
  } else {
    time_controls_enabled = false;
  }
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
    return 0; // King has no material value
  }
}

int Engine::evaluate(int ply) {
  if (isGameOver(board)) {
    if (getGameOverReason(board) == GameResultReason::CHECKMATE) {
      if (board.sideToMove() == Color::WHITE) {
        return -MATE_SCORE + ply;
      } else {
        return MATE_SCORE - ply;
      }
    }
    return DRAW_SCORE;
  }
  return evaluator.evaluate(board);
}