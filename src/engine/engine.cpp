#include "engine.hpp"

void Engine::printBoard() { std::cout << board; }

void Engine::setPosition(const std::string& fen) { board.setFen(fen); }

void Engine::initilizeEngine() {
  board.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Engine::makeMove(std::string move) {
  board.makeMove(uci::uciToMove(board, move));
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

int Engine::evaluate() {
  if (isGameOver()) {
    if (getGameOverReason() == GameResultReason::CHECKMATE) {
      if (board.sideToMove() == Color::WHITE) {
        return -MATE_SCORE;
      } else {
        return MATE_SCORE;
      }
    }
    return DRAW_SCORE;
  }

  int eval = 0;

  //* Counting the pieces for now simply
  constexpr int PAWN_VALUE = 100;
  constexpr int KNIGHT_VALUE = 300;
  constexpr int BISHOP_VALUE = 320;
  constexpr int ROOK_VALUE = 500;
  constexpr int QUEEN_VALUE = 900;

  auto countMaterial = [&](Color color) {
    return board.pieces(PieceType::PAWN, color).count() * PAWN_VALUE +
           board.pieces(PieceType::KNIGHT, color).count() * KNIGHT_VALUE +
           board.pieces(PieceType::BISHOP, color).count() * BISHOP_VALUE +
           board.pieces(PieceType::ROOK, color).count() * ROOK_VALUE +
           board.pieces(PieceType::QUEEN, color).count() * QUEEN_VALUE;
  };

  eval += countMaterial(Color::WHITE) - countMaterial(Color::BLACK);

  return eval;
}

int Engine::minmax(int depth, int alpha, int beta, bool isMaximizing, std::vector<Move>& pv) {

  if (depth == 0 || isGameOver()) {
    return evaluate();
  }

  positionsSearched++;

  Movelist moves;
  movegen::legalmoves(moves, board);
  Move bestMove = Move::NULL_MOVE;

  if (isMaximizing) {
    int maxEval = -MATE_SCORE;

    for (Move move : moves) {
      board.makeMove(move);
      std::vector<Move> childPv;
      int eval = minmax(depth - 1, alpha, beta, false, childPv);
      board.unmakeMove(move);

      if (eval > maxEval) {
        maxEval = eval;
        bestMove = move;
        pv = {move};
        pv.insert(pv.end(), childPv.begin(), childPv.end());
      }

      alpha = std::max(maxEval, alpha);

      if (beta <= alpha) break;
    }

    return maxEval;

  } else {
    int minEval = MATE_SCORE;

    for (Move move : moves) {
      board.makeMove(move);
      std::vector<Move> childPv;
      int eval = minmax(depth - 1, alpha, beta, true, childPv);
      board.unmakeMove(move);

      if (eval < minEval) {
        minEval = eval;
        bestMove = move;
        pv = {move};
        pv.insert(pv.end(), childPv.begin(), childPv.end());
      }

      beta = std::min(minEval, beta);

      if (beta <= alpha) break;
    }

    return minEval;
  }
}

std::string Engine::getBestMove(int depth) {
  if (isGameOver()) {
    return "";
  }

  Movelist moves;
  movegen::legalmoves(moves, board);

  bool isMaximizing = (board.sideToMove() == Color::WHITE); 

  int bestEval = isMaximizing ? -MATE_SCORE : MATE_SCORE;
  Move bestMove = Move::NULL_MOVE;
  std::vector<Move> bestLine;

  for (Move move : moves) {
    board.makeMove(move);
    std::vector<Move> MovePv;
    int eval = minmax(depth - 1, -MATE_SCORE, MATE_SCORE, !isMaximizing, MovePv);
    board.unmakeMove(move);

    if (isMaximizing && eval > bestEval) {
      bestEval = eval;
      bestMove = move;
      bestLine = {move};
      bestLine.insert(bestLine.end(), MovePv.begin(), MovePv.end());
    }
    if (!isMaximizing && eval < bestEval) {
      bestEval = eval;
      bestMove = move;
      MovePv = {move};
      bestLine.insert(bestLine.end(), MovePv.begin(), MovePv.end());
    }
  }

  if (bestMove == Move::NULL_MOVE) {
    return "";
  }

  // Print the info
  std::cout << "info depth "<< depth << " nodes " << positionsSearched << " score cp "<< bestEval << " pv ";
  for (Move move : bestLine) {
    std::cout << move << " ";
  }
  std::cout<<"\n";

  return uci::moveToUci(bestMove);
}
