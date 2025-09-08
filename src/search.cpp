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
      clearHistory();
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
  clearHistory();

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
    if (getGameOverReason(board) == GameResultReason::CHECKMATE)
      return -MATE_SCORE + ply;
    else
      return 0;
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

        // History score
        historyTable[board.sideToMove()][move.from().index()]
                    [move.to().index()] = depth * depth;
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

      // History score
      score += historyTable[board.sideToMove()][move.from().index()]
                           [move.to().index()];
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
    if (getGameOverReason(board) == GameResultReason::CHECKMATE)
      return -MATE_SCORE + ply;
    else
      return 0;
  }

  int standPat = evaluate();

  if (standPat >= beta) {
    return beta;
  }
  alpha = std::max(alpha, standPat);

  Movelist allMoves;
  movegen::legalmoves<movegen::MoveGenType::ALL>(allMoves, board);

  Movelist moves;
  for (Move m : allMoves) {
    if (board.isCapture(m) || m.typeOf() == Move::PROMOTION) {
      moves.add(m);
    }
  }

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

void Search::clearHistory() {
  for (int c = 0; c < 2; c++) {
    for (int from = 0; from < 64; from++) {
      for (int to = 0; to < 64; to++) {
        historyTable[c][from][to] = 0;
      }
    }
  }
}

int Search::getPieceValue(Piece piece) {
  switch (piece.type().internal()) {
    case chess::PieceType::PAWN:
      return 100;
    case chess::PieceType::KNIGHT:
      return 300;
    case chess::PieceType::BISHOP:
      return 320;
    case chess::PieceType::ROOK:
      return 500;
    case chess::PieceType::QUEEN:
      return 900;
    default:
      return 0;  // King has no material value
  }
}

int Search::evaluate() {
  int pieces[33];
  int squares[33];

  int player = (board.sideToMove() == chess::Color::WHITE) ? 0 : 1;

  int index = 2;

  pieces[0] = squares[0] = 0;
  pieces[1] = squares[1] = 0;

  for (Square sq = 0; sq < 64; ++sq) {
    chess::Piece piece = board.at(sq);

    if (piece != Piece::NONE) {
      int nnue_piece = piece_to_nnue(piece);

      if (nnue_piece == 1) {
        // WHite king
        pieces[0] = nnue_piece;
        squares[0] = sq.index();

      } else if (nnue_piece == 7) {
        // black king
        pieces[1] = nnue_piece;
        squares[1] = sq.index();

      } else {
        pieces[index] = nnue_piece;
        squares[index] = sq.index();
        index++;
      }
    }
  }

  // Null-terminate the arrays
  pieces[index] = 0;
  squares[index] = 0;

  assert(pieces[0] != 0);
  assert(pieces[1] != 0);

  // TODO: This is a huge performance bottleneck.
  // Every call to evaluate() creates a new NNUEdata object, which forces a
  // full recalculation of the accumulator. The correct way to do this is to
  // use nnue_evaluate_incremental() and pass the NNUEdata pointer down the
  // search tree. This requires significant changes to the search logic and
  // Board class to store and update the NNUEdata pointer.

  // ! The current code gives a nps of about 150K + in starting postion.
  // if i set the eval to constant of 0 then the nps is about 700K which means
  // that this part of the engine is expensive as above comments
  return nnue_evaluate(player, pieces, squares);
}

int Search::piece_to_nnue(chess::Piece piece) {
  static const int p_map[] = {6, 5, 4, 3, 2, 1};
  int p_type = static_cast<int>(piece.type().internal());
  if (p_type > 5) return 0;
  int nnue_p = p_map[p_type];
  if (piece.color() == chess::Color::BLACK) {
    nnue_p += 6;
  }
  return nnue_p;
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