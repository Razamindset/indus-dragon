#include "search.hpp"

#include <poll.h>
#include <unistd.h>

#include <fstream>
#include <iostream>

#include "constants.hpp"
#include "nnue/nnue.h"

// Check if there is any input waiting to be processed
void Search::uci_input_handler() {
  struct pollfd fds[1];
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;

  if (poll(fds, 1, 0) > 0) {
    std::string line;
    std::getline(std::cin, line);
    if (line == "stop") {
      stopSearchFlag = true;
    } else if (line == "quit") {
      tt_helper.clear_table();
      clearKiller();
      exit(0);
    }
  }
}

Search::Search(Board &board, TranspositionTable &tt_helper)
    : board(board), tt_helper(tt_helper) {
  pvTable.resize(MAX_SEARCH_DEPTH);
  for (auto &pv : pvTable) {
    pv.reserve(MAX_SEARCH_DEPTH);
  }
}

void Search::searchBestMove() {
  stopSearchFlag = false;
  if (isGameOver(board)) {
    return;
  }

  calculateSearchTime();
  clearKiller();

  search_start_time = std::chrono::steady_clock::now();

  positionsSearched = 0;

  best_move_changes = 0;

  Move last_iteration_best_move = Move::NULL_MOVE;

  Move bestMove = Move::NULL_MOVE;
  std::vector<Move> bestLine;

  // Iterative Deepening Loop
  for (int currentDepth = 1; currentDepth <= MAX_SEARCH_DEPTH; ++currentDepth) {
    int bestEval = negamax(currentDepth, -MATE_SCORE, MATE_SCORE, 0);

    // If the hard time limit was hit, stop searching immediately.
    if (stopSearchFlag) {
      break;
    }

    const auto &currentBestLine = pvTable[0];
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
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                            current_time - search_start_time)
                            .count();

    long long nps = 0;

    // Calculate nodes per second as (nodes / milliseconds) * 1000
    if (elapsed_time > 0) {
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
    bestMove = moves[0];
  }

  const std::string bestmove_str = "bestmove " + uci::moveToUci(bestMove);
  std::cout << bestmove_str << std::endl;
  logMessage(bestmove_str);
}

int Search::negamax(int depth, int alpha, int beta, int ply) {
  if ((positionsSearched & 2047) == 0) {
    uci_input_handler();
  }

  if (stopSearchFlag) {
    return 0;
  }

  if (checkHardTimeLimit()) {
    return 0;
  }

  pvTable[ply].clear();

  positionsSearched++;

  if (isGameOver(board)) {
    return -MATE_SCORE + ply;
  }

  if (depth == 0) {
    return quiescenceSearch(alpha, beta, ply);
  }

  if (board.isRepetition(1)) {
    return 0;
  }

  if (board.isHalfMoveDraw() &&
      board.getHalfMoveDrawType().second == GameResult::DRAW) {
    return 0;
  }

  uint64_t boardhash = board.hash();
  int ttScore = 0;
  Move ttMove = Move::NULL_MOVE;
  int originalAlpha = alpha;

  if (tt_helper.probeTT(boardhash, depth, ttScore, alpha, beta, ttMove, ply) &&
      ply > 0) {
    return ttScore;
  }

  Movelist moves;
  movegen::legalmoves(moves, board);

  orderMoves(moves, ttMove, ply, false);

  Move bestMove = Move::NULL_MOVE;
  int bestScore = -MATE_SCORE;

  for (int i = 0; i < moves.size(); ++i) {
    Move move = moves[i];
    board.makeMove(move);

    int eval = -negamax(depth - 1, -beta, -alpha, ply + 1);
    board.unmakeMove(move);

    if (eval > bestScore) {
      bestScore = eval;
      bestMove = move;

      pvTable[ply].clear();
      pvTable[ply].push_back(move);
      if (ply + 1 < MAX_SEARCH_DEPTH && !pvTable[ply + 1].empty()) {
        pvTable[ply].insert(pvTable[ply].end(), pvTable[ply + 1].begin(),
                            pvTable[ply + 1].end());
      }
    }
    alpha = std::max(bestScore, alpha);

    if (alpha >= beta) {
      if (!board.isCapture(move) && move.typeOf() != Move::PROMOTION) {
        killerMoves[ply][1] = killerMoves[ply][0];
        killerMoves[ply][0] = move;
      }
      tt_helper.storeTT(boardhash, depth, beta, TTEntryType::LOWER, bestMove,
                        ply);
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

  tt_helper.storeTT(boardhash, depth, bestScore, entryType, bestMove, ply);

  return bestScore;
}

/* Order moves based on their priority */
void Search::orderMoves(Movelist &moves, Move ttMove, int ply,
                        bool isQuiescence) {
  const Move killer1 = killerMoves[ply][0];
  const Move killer2 = killerMoves[ply][1];

  for (Move &move : moves) {
    int score = 0;

    if (ttMove != Move::NULL_MOVE && move == ttMove) {
      score += 10000;
    }

    // Prioritize captures using MVV-LVA
    if (board.isCapture(move)) {
      Piece attacker = board.at(move.from());
      Piece victim = board.at(move.to());
      score += 3000 + getPieceValue(victim) - getPieceValue(attacker);
    } else {
      if (move == killer1) {
        score += 500;  // High score for primary killer
      } else if (move == killer2) {
        score += 400;  // Slightly lower score for secondary killer
      }
    }

    // Prioritize promotions
    if (move.typeOf() == Move::PROMOTION) {
      if (move.promotionType() == QUEEN)
        score += 900;
      else if (move.promotionType() == ROOK)
        score += 500;
      else if (move.promotionType() == BISHOP)
        score += 320;
      else if (move.promotionType() == KNIGHT)
        score += 300;
    }

    move.setScore(static_cast<int16_t>(score));
  }

  // Sort moves in descending order of score
  std::sort(moves.begin(), moves.end(),
            [](const Move &a, const Move &b) { return a.score() > b.score(); });
}

/* Reach a stable quiet pos before evaluating */
int Search::quiescenceSearch(int alpha, int beta, int ply) {
  if ((positionsSearched & 2047) == 0) {
    uci_input_handler();
  }

  if (stopSearchFlag) {
    return 0;
  }
  positionsSearched++;

  if (isGameOver(board)) {
    return -MATE_SCORE + ply;
  }

  int standPat = evaluate(ply);

  if (standPat >= beta) {
    return beta;
  }
  alpha = std::max(alpha, standPat);

  Movelist moves;
  movegen::legalmoves<movegen::MoveGenType::CAPTURE>(moves, board);
  orderMoves(moves, Move::NULL_MOVE, ply, true);

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
      return 0;  // King has no material value
  }
}

int Search::evaluate(int ply) {
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

void Search::printInfoLine(int bestEval, std::vector<Move> bestLine,
                           int currentDepth, long long nps,
                           long long elapsed_time) {
  std::stringstream info_ss;
  info_ss << "info depth " << currentDepth << " nodes " << positionsSearched
          << " time " << elapsed_time << " nps " << nps << " score ";

  if (std::abs(bestEval) > (MATE_SCORE - MATE_THRESHHOLD)) {
    int movesToMate;
    if (bestEval > 0) {  // White is mating
      movesToMate = MATE_SCORE - bestEval;
    } else {
      movesToMate = MATE_SCORE + bestEval;
    }
    int fullMovesToMate = (movesToMate + 1) / 2;
    info_ss << "mate " << (bestEval > 0 ? fullMovesToMate : -fullMovesToMate)
            << " pv ";
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
  if (!storeLogs) return;

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