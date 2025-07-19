#include "search.hpp"
#include "constants.hpp"
#include "evaluation.hpp"
#include "heuristics.hpp"
#include "utils.hpp"

Search::Search(Board &board, TimeManager &time_manager, TranspositionTable &tt_helper,
               Evaluation &evaluator)
    : board(board), time_manager(time_manager), tt_helper(tt_helper), evaluator(evaluator) {}

/* Order moves based on their priority */
void Search::orderMoves(Movelist &moves, Move ttMove, int ply) {
  std::vector<std::pair<Move, int>> scoredMoves;
  scoredMoves.reserve(moves.size());

  // Get the killer moves
  Move killer1 = killerMoves[ply][0];
  Move killer2 = killerMoves[ply][1];

  // Loop through each move and assign it a score
  for (Move move : moves) {
    int score = 0;

    // Priorotize ttMove and break after putthing on top
    if (ttMove != Move::NULL_MOVE && move == ttMove) {
      scoredMoves.emplace_back(move, 10000);
      continue;
    }

    // KIller moves score
    if (move == killer1) {
      score = 9000; // High score for primary killer
    } else if (move == killer2) {
      score = 8500; // Slightly lower score for secondary killer
    }

    // Todo Find a good way to check for checks because can't use board.makemove() and

    // Prioritize captures using MVV-LVA
    if (board.isCapture(move)) {
      Piece attacker = board.at(move.from());
      Piece victim = board.at(move.to());
      score += 100 * getPieceValue(victim) - getPieceValue(attacker);
    } else {
      Piece movingPiece = board.at(move.from());
      int pieceIndex = static_cast<int>(movingPiece);
      int squareIndex = move.to().index();
      score += historyTable[pieceIndex][squareIndex];
    }

    // Prioritize promotions
    if (move.promotionType() == QUEEN)
      score += 900;
    if (move.promotionType() == ROOK)
      score += 500;
    if (move.promotionType() == BISHOP)
      score += 320;
    if (move.promotionType() == KNIGHT)
      score += 300;

    // Give a slight edge castling
    if (move.typeOf() == Move::CASTLING) {
      score += 300;
    }

    // Add the move to the list along with its score
    scoredMoves.emplace_back(move, score);
  }

  // Sort the moves based on scores
  std::sort(scoredMoves.begin(), scoredMoves.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });

  // Replace original move list with sorted moves
  moves.clear();
  for (const auto &[move, score] : scoredMoves) {
    moves.add(move);
  }
}

int Search::minmax(int depth, int alpha, int beta, bool isMaximizing, std::vector<Move> &pv,
                   int ply) {
  if (stopSearchFlag) {
    return isMaximizing ? beta : alpha;
  }

  if (time_controls_enabled) {
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(current_time - search_start_time)
            .count();
    if (elapsed_time >= hard_time_limit) {
      stopSearchFlag = true;
      return isMaximizing ? beta : alpha;
    }
  }

  positionsSearched++;
  pv.clear();

  if (isGameOver(board)) {
    return evaluate(ply);
  }

  if (depth == 0) {
    return quiescenceSearch(alpha, beta, isMaximizing, ply);
  }

  uint64_t boardhash = board.hash();
  int ttScore = 0;
  Move ttMove = Move::NULL_MOVE;
  TTEntryType entry_type;
  int originalAlpha = alpha;

  // Look at this tommorrow
  if (tt_helper.probeTT(boardhash, depth, ttScore, alpha, beta, ttMove, ply, entry_type)) {
    if (entry_type == TTEntryType::EXACT && ttMove != Move::NULL_MOVE) {
      pv.push_back(ttMove);
      board.makeMove(ttMove);
      std::vector<Move> childPv;
      minmax(depth - 1, alpha, beta, !isMaximizing, childPv, ply + 1);
      board.unmakeMove(ttMove);
      pv.insert(pv.end(), childPv.begin(), childPv.end());
    }
    return ttScore;
  }

  Movelist moves;
  movegen::legalmoves(moves, board);
  orderMoves(moves, ttMove, ply);

  Move bestMove = Move::NULL_MOVE;
  int bestScore = isMaximizing ? -MATE_SCORE : MATE_SCORE;

  for (int i = 0; i < moves.size(); ++i) {
    Move move = moves[i];
    board.makeMove(move);

    std::vector<Move> childPv;
    int eval = minmax(depth - 1, alpha, beta, !isMaximizing, childPv, ply + 1);
    board.unmakeMove(move);

    if (isMaximizing) {
      if (eval > bestScore) {
        bestScore = eval;
        bestMove = move;
        pv = {move};
        pv.insert(pv.end(), childPv.begin(), childPv.end());
      }
      alpha = std::max(bestScore, alpha);
    } else {
      if (eval < bestScore) {
        bestScore = eval;
        bestMove = move;
        pv = {move};
        pv.insert(pv.end(), childPv.begin(), childPv.end());
      }
      beta = std::min(bestScore, beta);
    }

    if (alpha >= beta) {
      if (!board.isCapture(move)) {
        killerMoves[ply][1] = killerMoves[ply][0];
        killerMoves[ply][0] = move;
        Piece movingPiece = board.at(move.from());
        updateHistoryScore(movingPiece, move.to(), depth);
      }
      break;
    }
  }

  TTEntryType entryType;
  if (bestScore <= originalAlpha) {
    entryType = TTEntryType::UPPER;
  } else if (bestScore >= beta) {
    entryType = TTEntryType::LOWER;
  } else {
    entryType = TTEntryType::EXACT;
  }

  if (bestMove != Move::NULL_MOVE) {
    tt_helper.storeTT(boardhash, depth, bestScore, entryType, bestMove, ply);
  }

  return bestScore;
}

/* Reach a stable quiet pos before evaluating */
int Search::quiescenceSearch(int alpha, int beta, bool isMaximizing, int ply) {
  if (stopSearchFlag) {
    return isMaximizing ? beta : alpha;
  }
  positionsSearched++;

  if (isGameOver(board)) {
    return evaluate(ply);
  }

  int standPat = evaluate(ply);

  if (isMaximizing) {
    if (standPat >= beta) {
      return beta;
    }
    alpha = std::max(alpha, standPat);
  } else {
    if (standPat <= alpha) {
      return alpha;
    }
    beta = std::min(beta, standPat);
  }

  Movelist moves;
  movegen::legalmoves(moves, board);
  orderQuiescMoves(moves);

  for (Move move : moves) {
    board.makeMove(move);
    int score = quiescenceSearch(alpha, beta, !isMaximizing, ply + 1);
    board.unmakeMove(move);

    if (isMaximizing) {
      alpha = std::max(alpha, score);
      if (alpha >= beta) {
        break;
      }
    } else {
      beta = std::min(beta, score);
      if (alpha >= beta) {
        break;
      }
    }
  }

  return isMaximizing ? alpha : beta;
}

void Search::orderQuiescMoves(Movelist &moves) {
  std::vector<std::pair<Move, int>> scoredMoves;
  scoredMoves.reserve(moves.size());

  for (Move move : moves) {
    int score = 0;
    bool isInteresting = false;

    if (board.isCapture(move)) {
      isInteresting = true;
      Piece attacker = board.at(move.from());
      Piece victim = board.at(move.to());
      score += 100 * getPieceValue(victim) - getPieceValue(attacker);
    }

    if (move.typeOf() == Move::PROMOTION) {
      isInteresting = true;
      if (move.promotionType() == QUEEN) {
        score += 900;
      } else if (move.promotionType() == ROOK) {
        score += 500;
      } else if (move.promotionType() == BISHOP) {
        score += 320;
      } else if (move.promotionType() == KNIGHT) {
        score += 300;
      }
    }

    if (isInteresting) {
      scoredMoves.emplace_back(move, score);
    }
  }

  std::sort(scoredMoves.begin(), scoredMoves.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });

  moves.clear();
  for (const auto &[move, score] : scoredMoves) {
    moves.add(move);
  }
}

std::string Search::searchBestMove() {
  stopSearchFlag = false;
  if (isGameOver(board)) {
    return "";
  }
  int maxDepth = MAX_SEARCH_DEPTH;

  if (time_controls_enabled) {
    CalculatedTime times = time_manager.calculateSearchTime(board);
    soft_time_limit = times.soft_time;
    hard_time_limit = times.hard_time;
  } else {
    soft_time_limit = 1000000000LL;
    hard_time_limit = 1000000000LL;
  }

  search_start_time = std::chrono::steady_clock::now();

  positionsSearched = 0;
  best_move_changes = 0;
  last_iteration_best_move = Move::NULL_MOVE;
  clearKiller();
  clearHistoryTable();

  bool isMaximizing = (board.sideToMove() == Color::WHITE);
  Move bestMove = Move::NULL_MOVE;
  std::vector<Move> bestLine;

  // Iterative Deepening Loop
  for (int currentDepth = 1; currentDepth <= maxDepth; ++currentDepth) {
    int bestEval = isMaximizing ? -MATE_SCORE : MATE_SCORE;
    std::vector<Move> currentBestLine;

    bestEval = minmax(currentDepth, -MATE_SCORE, MATE_SCORE, isMaximizing, currentBestLine, 0);

    // If the hard time limit was hit, stop searching immediately.
    if (stopSearchFlag && currentDepth > 1) {
      break;
    }

    if (!currentBestLine.empty()) {
      if (last_iteration_best_move != Move::NULL_MOVE &&
          currentBestLine.front() != last_iteration_best_move) {
        best_move_changes++;
      }
      bestMove = currentBestLine.front();
      bestLine = currentBestLine;
      last_iteration_best_move = bestMove;
    }

    // Elapsed Time and NPS
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(current_time - search_start_time)
            .count();

    long long nps = 0;

    if (elapsed_time > 0) {
      // Calculate nodes per second as (nodes / milliseconds) * 1000
      nps = (positionsSearched * 1000) / elapsed_time;
    }

    // UCI output
    std::cout << "info depth " << currentDepth << " nodes " << positionsSearched << " time "
              << elapsed_time << " nps " << nps << " score ";

    if (std::abs(bestEval) > (MATE_SCORE - MATE_THRESHHOLD)) {
      int movesToMate;
      if (bestEval > 0) { // White is mating
        movesToMate = MATE_SCORE - bestEval;
      } else {
        movesToMate = MATE_SCORE + bestEval;
      }
      int fullMovesToMate = (movesToMate + 1) / 2;

      // Perspective based mate and eval scores
      if ((board.sideToMove() == Color::BLACK && bestEval > 0) ||
          (board.sideToMove() == Color::WHITE && bestEval < 0)) {
        fullMovesToMate = -fullMovesToMate;
      }
      std::cout << "mate " << fullMovesToMate << " pv ";
    } else {
      if (board.sideToMove() == Color::BLACK) {
        bestEval = -bestEval;
      }
      std::cout << "cp " << bestEval << " pv ";
    }

    for (const auto &move : bestLine) {
      std::cout << move << " ";
    }
    std::cout << std::endl;

    // Check if we should stop.
    if (elapsed_time >= soft_time_limit) {
      // If the best move has been unstable, give it a bit more time,
      // but only if we are not close to our hard limit.
      if (best_move_changes >= 2 && elapsed_time < hard_time_limit / 3) {
        // Extend time by a small, fixed amount (e.g., 30% of the original soft
        // limit)
        soft_time_limit += soft_time_limit * 0.3;
        best_move_changes = 0; // Reset counter for the next potential extension
      } else {
        // Otherwise, the search is stable enough or we are out of time, so
        // stop.
        break;
      }
    }
  }

  if (bestMove == Move::NULL_MOVE) {
    Movelist moves;
    movegen::legalmoves(moves, board);
    if (!moves.empty()) {
      bestMove = moves[0];
    } else {
      return "";
    }
  }

  return uci::moveToUci(bestMove);
}

int Search::getPieceValue(Piece piece) {
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

int Search::evaluate(int ply) {
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

bool Search::isGameOver(const chess::Board &board) {
  auto result = board.isGameOver();
  return result.second != chess::GameResult::NONE;
}

GameResultReason Search::getGameOverReason(const chess::Board &board) {
  auto result = board.isGameOver();
  return result.first;
}