#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <string>

#include "../chess-library/include/chess.hpp"

constexpr int MATE_SCORE = 1000000;
constexpr int DRAW_SCORE = 0;

using namespace chess;

class Engine {
 private:
  Board board;
  int getPieceValue(Piece piece);
  int positionsSearched = 0;

  int evaluate(int ply);
  int minmax(int depth, int alpha, int beta, bool isMaximizing,
             std::vector<Move>& pv, int ply);
  void orderMoves(Movelist &moves);

 public:
  void setPosition(const std::string& fen);
  void printBoard();
  void initilizeEngine();

  // Client accessible get best move function
  std::string getBestMove(int depth);

  //* Game state related
  bool isGameOver() {
    auto result = board.isGameOver();
    return result.second != GameResult::NONE;
  }

  GameResultReason getGameOverReason() {
    auto result = board.isGameOver();
    return result.first;
  }

  // Move making used when the chess gui gives a list of move history. use this
  // to set the board state correctly
  void makeMove(std::string move);
};

#endif
