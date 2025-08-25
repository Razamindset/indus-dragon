#include "search.hpp"
#include "constants.hpp"
#include "nnue/nnue.h"
#include <fstream>
#include <iostream>

Search::Search(Board &board, TimeManager &time_manager, TranspositionTable &tt_helper,
               bool time_controls_enabled)
    : board(board), time_manager(time_manager), tt_helper(tt_helper),
      time_controls_enabled(time_controls_enabled) {}

void Search::searchBestMove() {
  stopSearchFlag = false;
  if (isGameOver(board)) {
    return;
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

  Move bestMove = Move::NULL_MOVE;
  std::vector<Move> bestLine;

  // Iterative Deepening Loop
  for (int currentDepth = 1; currentDepth <= maxDepth; ++currentDepth) {
    std::vector<Move> currentBestLine;

    int bestEval = negamax(currentDepth, -MATE_SCORE, MATE_SCORE, currentBestLine, 0);

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
    printInfoLine(bestEval, bestLine, currentDepth, nps, elapsed_time);

    // Check if we should stop.
    if (manageTime(elapsed_time)) {
      break;
    }
  }

  if (bestMove == Move::NULL_MOVE) {
    Movelist moves;
    movegen::legalmoves(moves, board);
    if (!moves.empty()) {
      bestMove = moves[0];
    } else {
      const std::string bestmove_str = "bestmove (none)";
      std::cout << bestmove_str << std::endl;

      logMessage(bestmove_str);
      return;
    }
  }

  const std::string bestmove_str = "bestmove " + uci::moveToUci(bestMove);
  std::cout << bestmove_str << std::endl;
  logMessage(bestmove_str);
}

int Search::negamax(int depth, int alpha, int beta, std::vector<Move> &pv, int ply) {
  if (stopSearchFlag) {
    return 0;
  }

  if (checkHardTimeLimit()) {
    return 0;
  }

  pv.clear();

  positionsSearched++;

  if (isGameOver(board)) {
    return evaluate(ply);
  }

  if (depth == 0) {
    return quiescenceSearch(alpha, beta, ply);
  }

  // This position has appeared second time
  if (board.isRepetition(1)) {
    return 0;
  }

  uint64_t boardhash = board.hash();
  int ttScore = 0;
  Move ttMove = Move::NULL_MOVE;
  TTEntryType entry_type;
  int originalAlpha = alpha;

  // See if a score exists in tt.
  if (tt_helper.probeTT(boardhash, depth, ttScore, alpha, beta, ttMove, ply, entry_type) &&
      ply > 0) {
    if (ttMove != Move::NULL_MOVE) {
      pv.push_back(ttMove);
    }
    return ttScore;
  }

  Movelist moves;
  movegen::legalmoves(moves, board);

  // FIX: Check for no legal moves (checkmate/stalemate)
  if (moves.size() == 0) {
    if (board.inCheck()) {
      return -MATE_SCORE + ply; // Checkmate Score
    } else {
      return 0; // Draw
    }
  }

  orderMoves(moves, ttMove, ply);

  Move bestMove = Move::NULL_MOVE;
  int bestScore = -MATE_SCORE;

  for (int i = 0; i < moves.size(); ++i) {
    Move move = moves[i];
    board.makeMove(move);

    std::vector<Move> childPv;
    int eval = -negamax(depth - 1, -beta, -alpha, childPv, ply + 1);
    board.unmakeMove(move);

    if (eval > bestScore) {
      bestScore = eval;
      bestMove = move;
      pv = {move};
      pv.insert(pv.end(), childPv.begin(), childPv.end());
    }
    alpha = std::max(bestScore, alpha);

    if (alpha >= beta) {
      // Store killer moves BEFORE breaking
      if (!board.isCapture(move) && move.typeOf() != Move::PROMOTION) {
        killerMoves[ply][1] = killerMoves[ply][0];
        killerMoves[ply][0] = move;
      }
      // FIX: Still store in TT even with beta cutoff
      tt_helper.storeTT(boardhash, depth, bestScore, TTEntryType::LOWER, bestMove, ply);
      break;
    }
  }

  // Store in transposition table
  TTEntryType entryType;
  if (bestScore <= originalAlpha) {
    entryType = TTEntryType::UPPER;
  } else if (bestScore >= beta) {
    entryType = TTEntryType::LOWER;
  } else {
    entryType = TTEntryType::EXACT;
  }

  tt_helper.storeTT(boardhash, depth, bestScore, entryType, bestMove, ply);

  return bestScore;
}

/* Order moves based on their priority */
void Search::orderMoves(Movelist &moves, Move ttMove, int ply) {
  std::vector<std::pair<Move, int>> scoredMoves;
  scoredMoves.reserve(moves.size());

  // Get the killer moves
  Move killer1 = killerMoves[ply][0];
  Move killer2 = killerMoves[ply][1];

  // Loop through each move and assign it a score
  // TTmove, Captues, Promotions, Killer MOves, Castling, History heuristics
  for (Move move : moves) {
    int score = 0;

    // Prioritize ttMove and break after putting on top
    if (ttMove != Move::NULL_MOVE && move == ttMove) {
      scoredMoves.emplace_back(move, 10000);
      continue;
    }

    // Prioritize captures using MVV-LVA
    if (board.isCapture(move)) {
      Piece attacker = board.at(move.from());
      Piece victim = board.at(move.to());
      score += 1000 + 100 * getPieceValue(victim) - getPieceValue(attacker);
    }

    // ! Expensive For normal search But donot skip in qsearch
    // Prioritize checks - FIX: Only if not a capture (to avoid double bonus)
    // if (!board.isCapture(move)) {
    //   board.makeMove(move);
    //   if (board.inCheck()) {
    //     score += 1000; // High bonus for giving check
    //   }
    //   board.unmakeMove(move);
    // }

    // Prioritize promotions
    if (move.promotionType() == QUEEN)
      score += 900;
    else if (move.promotionType() == ROOK)
      score += 500;
    else if (move.promotionType() == BISHOP)
      score += 320;
    else if (move.promotionType() == KNIGHT)
      score += 300;

    // Killer moves score
    if (!board.isCapture(move)) {
      if (move == killer1) {
        score = 500; // High score for primary killer
      } else if (move == killer2) {
        score = 400; // Slightly lower score for secondary killer
      }
    }

    // Give a slight edge to castling
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

/* Reach a stable quiet pos before evaluating */
int Search::quiescenceSearch(int alpha, int beta, int ply) {
  if (stopSearchFlag) {
    return 0;
  }
  positionsSearched++;

  if (isGameOver(board)) {
    return evaluate(ply);
  }

  int standPat = evaluate(ply);

  if (standPat >= beta) {
    return beta;
  }
  alpha = std::max(alpha, standPat);

  Movelist moves;
  movegen::legalmoves(moves, board);
  orderQuiescMoves(moves);

  for (Move move : moves) {

    board.makeMove(move);
    int score = -quiescenceSearch(-beta, -alpha, ply + 1);
    board.unmakeMove(move);

    alpha = std::max(alpha, score);
    if (alpha >= beta) {
      break;
    }
  }

  return alpha;
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

    // ! Expensive But we cannot miss checks in q search
    // Check for checks
    board.makeMove(move);
    if (board.inCheck()) {
      isInteresting = true;
      score += 100; // Bonus for giving check
    }
    board.unmakeMove(move);

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

// Heuristics
void Search::clearKiller() {
  for (int i = 0; i < MAX_SEARCH_DEPTH; ++i) {
    killerMoves[i][0] = chess::Move::NULL_MOVE;
    killerMoves[i][1] = chess::Move::NULL_MOVE;
  }
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
      return -MATE_SCORE + ply;
    }
    return DRAW_SCORE;
  }
  return nnue_evaluate_fen(board.getFen().c_str());
}

bool Search::isGameOver(const chess::Board &board) {
  auto result = board.isGameOver();
  return result.second != chess::GameResult::NONE;
}

GameResultReason Search::getGameOverReason(const chess::Board &board) {
  auto result = board.isGameOver();
  return result.first;
}

bool Search::manageTime(long long elapsed_time) {
  if (!time_controls_enabled) {
    return false; // Don't stop if time controls are off
  }

  if (elapsed_time >= soft_time_limit) {
    if (best_move_changes >= 2 && elapsed_time < hard_time_limit / 3) {
      soft_time_limit += soft_time_limit * 0.3;
      best_move_changes = 0;
      return false; // Continue searching
    }
    return true; // Stop searching
  }
  return false; // Don't stop yet
}

bool Search::checkHardTimeLimit() {
  if (time_controls_enabled) {
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(current_time - search_start_time)
            .count();
    if (elapsed_time >= hard_time_limit) {
      stopSearchFlag = true;
      return true;
    }
  }
  return false;
}

void Search::printInfoLine(int bestEval, std::vector<Move> bestLine, int currentDepth,
                           long long nps, long long elapsed_time) {
  std::stringstream info_ss;
  info_ss << "info depth " << currentDepth << " nodes " << positionsSearched << " time "
          << elapsed_time << " nps " << nps << " score ";

  if (std::abs(bestEval) > (MATE_SCORE - MATE_THRESHHOLD)) {
    int movesToMate;
    if (bestEval > 0) { // White is mating
      movesToMate = MATE_SCORE - bestEval;
    } else {
      movesToMate = MATE_SCORE + bestEval;
    }
    int fullMovesToMate = (movesToMate + 1) / 2;
    info_ss << "mate " << (bestEval > 0 ? fullMovesToMate : -fullMovesToMate) << " pv ";
  } else {
    info_ss << "cp " << bestEval << " pv ";
  }

  for (const auto &move : bestLine) {
    info_ss << move << " ";
  }
  std::string info_str = info_ss.str();
  std::cout << info_str << std::endl;
  logMessage(info_str);
}

void Search::logMessage(const std::string &message) {
  if (!storeLogs)
    return;

  try {
    std::ofstream logFile("uci_log.txt", std::ios_base::app);
    if (logFile.is_open()) {
      logFile << message << std::endl;
      logFile.close();
    }
  } catch (...) {
    // Silently ignore logging errors to prevent crashes
  }
}