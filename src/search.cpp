#include "search.hpp"

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <poll.h>
#include <unistd.h>
#endif

#include <cmath>
#include <fstream>

/*
Check if there is any input waiting to be processed.
The api is different for windows and unix based systems.
*/
void Search::communicate() {
#ifdef _WIN32
  static HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
  DWORD available = 0;

  if (PeekNamedPipe(stdin_handle, NULL, 0, NULL, &available, NULL) &&
      available > 0) {
    std::string line;
    std::getline(std::cin, line);
    if (line == "stop") {
      stopSearchFlag = true;
      logMessage("stop");
    } else if (line == "quit") {
      logMessage("quit");
      exit(0);
    } else if (line == "isready") {
      std::cout << "readyok" << std::endl;
      fflush(stdout);
    } else if (line == "ucinewgame") {
      tt_helper.clear_table();
    }
  }
#else
  struct pollfd fds[1];
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;

  if (poll(fds, 1, 0) > 0) {
    std::string line;
    std::getline(std::cin, line);
    if (line == "stop") {
      stopSearchFlag = true;
      logMessage("stop");
    } else if (line == "quit") {
      logMessage("quit");
      exit(0);
    } else if (line == "isready") {
      std::cout << "readyok" << std::endl;
      fflush(stdout);
    } else if (line == "ucinewgame") {
      tt_helper.clear_table();
    }
  }
#endif
}

Search::Search(chess::Board &board, TranspositionTable &tt_helper)
    : board(board), tt_helper(tt_helper) {
  pvTable.resize(MAX_SEARCH_DEPTH);
  for (auto &pv : pvTable) {
    pv.reserve(MAX_SEARCH_DEPTH);
  }

  nnue.load_network();
}

long long Search::benchSearch(int depth) {
  stopSearchFlag = false;
  positionsSearched = 0;

  if (isGameOver(board)) {
    return positionsSearched;
  }

  clearKiller();
  clearHistory();

  chess::Move bestMove = chess::Move::NULL_MOVE;
  int bestScore = 0;

  nnue.refreshAccumulator(board, accStack[0]);

  for (int currentDepth = 1; currentDepth <= depth; ++currentDepth) {
    bestScore = negamax(currentDepth, -MATE_SCORE, MATE_SCORE, 0, false);

    if (!pvTable[0].empty()) {
      bestMove = pvTable[0].front();
    }
  }

  std::cout << "nodes " << positionsSearched << " score " << bestScore
             << " bestmove " << chess::uci::moveToUci(bestMove) << "\n";

  return positionsSearched;
}

void Search::searchBestMove(int depth) {
  stopSearchFlag = false;
  if (isGameOver(board)) {
    lastBestMove = chess::Move::NULL_MOVE;
    lastScore = 0;
    const std::string bestmove_str = "bestmove 0000";
    if (!silent) std::cout << bestmove_str << std::endl;
    logMessage(bestmove_str);
    return;
  }

  clearKiller();
  clearHistory();

  timeManager.start(board);

  positionsSearched = 0;

  chess::Move last_iteration_best_move = chess::Move::NULL_MOVE;

  chess::Move bestMove = chess::Move::NULL_MOVE;
  std::vector<chess::Move> bestLine;

  nnue.refreshAccumulator(board, accStack[0]);

  int depth_to_search = MAX_SEARCH_DEPTH;
  if (depth > 0) depth_to_search = std::min(depth, MAX_SEARCH_DEPTH);

  int previousScore = 0;

  for (int currentDepth = 1; currentDepth <= depth_to_search; ++currentDepth) {
    int bestScore;

    if (currentDepth < ASPIRATION_MIN_DEPTH) {
      bestScore = negamax(currentDepth, -MATE_SCORE, MATE_SCORE, 0, false);
    } else {
      int window = ASPIRATION_WINDOW;
      int alpha = previousScore - window;
      int beta = previousScore + window;

      while (true) {
        bestScore = negamax(currentDepth, alpha, beta, 0, false);

        if (stopSearchFlag) break;

        if (bestScore <= alpha) {
          // Failed low — widen downward and re-search at the same depth
          alpha = std::max(bestScore - window, -MATE_SCORE);
          window *= 2;
          if (!silent) std::cout<<"info string aspiration refail Depth: "<<currentDepth << " Window_size:  "<< window << std::endl;
        } else if (bestScore >= beta) {
          // Failed high — widen upward and re-search at the same depth
          beta = std::min(bestScore + window, MATE_SCORE);
          window *= 2;
          if (!silent) std::cout<<"info string aspiration refail Depth: "<<currentDepth << " Window_size:  "<< window << std::endl;

        } else {
          // Landed inside the window — this iteration is done
          break;
        }
      }
    }

    if (stopSearchFlag) {
      break;
    }

    previousScore = bestScore;

    const auto &currentBestLine = pvTable[0];
    bool bestMoveChanged = false;
    if (!currentBestLine.empty()) {
      bestMoveChanged = (last_iteration_best_move != chess::Move::NULL_MOVE &&
                         currentBestLine.front() != last_iteration_best_move);
      bestMove = currentBestLine.front();
      bestLine = currentBestLine;
      last_iteration_best_move = bestMove;
    }

    long long elapsedTime = getElapsedTime();
    long long nps = 0;

    // Calculate nodes per second as (nodes / milliseconds) * 1000
    if (elapsedTime > 0) {
      nps = (positionsSearched * 1000) / elapsedTime;
    }

    // UCI output
    if (!silent) printInfoLine(bestScore, bestLine, currentDepth, nps, elapsedTime);

    // Check if we should stop.
    if (manageTime(elapsedTime, bestMoveChanged)) {
      break;
    }
  }

  if (bestMove == chess::Move::NULL_MOVE) {
    chess::Movelist moves;
    chess::movegen::legalmoves(moves, board);
    bestMove = moves[0];
  }

  lastBestMove = bestMove;
  lastScore = previousScore;

  const std::string bestmove_str = "bestmove " + chess::uci::moveToUci(bestMove);
  if (!silent) {
    std::cout << bestmove_str << std::endl;
  }
  logMessage(bestmove_str);
}

int Search::negamax(int depth, int alpha, int beta, int ply,
                    bool is_null = false) {
  if (stopSearchFlag || checkHardTimeLimit()) {
    return 0;
  }
  
  if (ply >= MAX_SEARCH_DEPTH - 1) {
    return evaluate(ply);
  }

  if ((positionsSearched & 2047) == 0) {
    communicate();
    if (stopSearchFlag) {
      return 0;
    }
  }

  pvTable[ply].clear();

  positionsSearched++;

  if (isGameOver(board)) {
    if (getGameOverReason(board) == chess::GameResultReason::CHECKMATE)
      return -MATE_SCORE + ply;
    else
      return DRAW_SCORE;
  }

  if (isSearchDraw(board, ply)) {
    return DRAW_SCORE;
  }

  if (depth == 0) {
    // Check extensions. Use if qsearch is not explicitly making sure that
    // checks are handled
    if (board.inCheck() && depth < 20) {
      depth++;
    } else {
      return qsearch(alpha, beta, ply);
    }
  }

  uint64_t boardhash = board.hash();
  int ttScore = 0;
  chess::Move ttMove = chess::Move::NULL_MOVE;
  int originalAlpha = alpha;

  if (tt_helper.probeTT(boardhash, depth, ttScore, alpha, beta, ttMove, ply) &&
      ply > 0) {
    return ttScore;
  }

    // Null move Pruning NMP. Elo: 77.71 +- 30
    if (depth > 3 && !board.inCheck() &&
        board.hasNonPawnMaterial(board.sideToMove()) &&
        depth != MAX_SEARCH_DEPTH && !is_null) {
      accStack[ply + 1] = accStack[ply]; // Copy current accumulator to next ply
      board.makeNullMove();
      int score = -negamax(depth - 2, -beta, -beta + 1, ply + 1, true);
      board.unmakeNullMove();

    if (stopSearchFlag) {
      return 0;
    }

    if (score >= beta) {
      return beta;
    }
  }

  // Static Null Move Pruning
  // Only applies at shallow depths
  // Conditions: not in check, not PV node, depth small, and not close to mate
  // score.
  if (depth <= 3 && !board.inCheck() && alpha < MATE_SCORE - 1000) {
    int staticEval = evaluate(ply);

    int margin = 120 * depth;

    if (staticEval - margin >= beta) {
      return beta;  // Fail high, Hopeless position
    }
  }

  chess::Movelist moves;
  chess::movegen::legalmoves(moves, board);

  orderMoves(moves, ttMove, ply, false);

  chess::Move bestMove = chess::Move::NULL_MOVE;
  int bestScore = -MATE_SCORE;

  for (int i = 0; i < moves.size(); ++i) {
    chess::Move move = moves[i];

    // Capture the flags for later use before making the move
    const bool isCapture = board.isCapture(move);
    const bool isPromotion = move.typeOf() == chess::Move::PROMOTION;
    const bool wasInCheck = board.inCheck();  // side to move, before this move

    // Incremental NNUE update
    accStack[ply + 1] = accStack[ply];
    nnue.updateAccumulator(board, move, accStack[ply + 1]);

    board.makeMove(move);

    const bool givesCheck = board.inCheck();  // opponent, after this move — valid now that move is made

    int score;

    bool skipReduction = i < LMR_FULL_DEPTH_MOVES || depth < LMR_MIN_DEPTH ||
                        isCapture || isPromotion || wasInCheck || givesCheck;

    // Late Move Reductions
    if (skipReduction) {
      // Search first N moves with full depth
      score = -negamax(depth - 1, -beta, -alpha, ply + 1, false);
    } else {
      // Reduce the depth
      int reduction = 1 + std::log(depth) * std::log(i) / LMR_DIVISOR;
      reduction = std::min(reduction, depth - 1);

      score = -negamax(depth - 1 - reduction, -beta, -alpha, ply + 1, false);

      if (score > alpha) {
        score = -negamax(depth - 1, -beta, -alpha, ply + 1, false);
      }
    }

    board.unmakeMove(move);

    // If the search was haulted the score cannot be used, neither the move
    if (stopSearchFlag) {
      return 0;
    }

    alpha = std::max(score, alpha);

    if (score > bestScore) {
      bestScore = score;
      bestMove = move;

      pvTable[ply].clear();
      pvTable[ply].push_back(move);
      if (ply + 1 < MAX_SEARCH_DEPTH && !pvTable[ply + 1].empty()) {
        pvTable[ply].insert(pvTable[ply].end(), pvTable[ply + 1].begin(),
                            pvTable[ply + 1].end());
      }
    }

    if (alpha >= beta) {
      if (!board.isCapture(move) && move.typeOf() != chess::Move::PROMOTION) {
        killerMoves[ply][1] = killerMoves[ply][0];
        killerMoves[ply][0] = move;

        // Add history
        int bonus = depth * depth;
        historyTable[board.sideToMove()][move.from().index()]
                    [move.to().index()] = bonus;
      }
      tt_helper.storeTT(boardhash, depth, score, TTEntryType::LOWER, move, ply);
      return score;  // beta cuttof
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

/*
Returns a bitboard of every piece, either color, that attacks `sq` given a
(possibly hypothetical) occupancy bitboard. Used by see() to walk through a
capture sequence as pieces are removed square by square without touching
the real board.
*/
chess::Bitboard Search::attackersTo(chess::Square sq, chess::Bitboard occupied) const {
  using namespace chess;

  Bitboard attackers = 0ULL;

  // Reverse pawn-attack trick: pawns of `color` that attack `sq` are found
  // by asking "which squares would a pawn of the opposite color standing on
  // sq attack" and intersecting with where our pawns actually are.
  attackers |= attacks::pawn(Color::BLACK, sq) & board.pieces(PieceType::PAWN, Color::WHITE);
  attackers |= attacks::pawn(Color::WHITE, sq) & board.pieces(PieceType::PAWN, Color::BLACK);

  attackers |= attacks::knight(sq) & (board.pieces(PieceType::KNIGHT, Color::WHITE) |
                                      board.pieces(PieceType::KNIGHT, Color::BLACK));

  const Bitboard bishopsQueens = board.pieces(PieceType::BISHOP, Color::WHITE) |
                                 board.pieces(PieceType::BISHOP, Color::BLACK) |
                                 board.pieces(PieceType::QUEEN, Color::WHITE) |
                                 board.pieces(PieceType::QUEEN, Color::BLACK);
  attackers |= attacks::bishop(sq, occupied) & bishopsQueens;

  const Bitboard rooksQueens = board.pieces(PieceType::ROOK, Color::WHITE) |
                               board.pieces(PieceType::ROOK, Color::BLACK) |
                               board.pieces(PieceType::QUEEN, Color::WHITE) |
                               board.pieces(PieceType::QUEEN, Color::BLACK);
  attackers |= attacks::rook(sq, occupied) & rooksQueens;

  attackers |= attacks::king(sq) & (board.pieces(PieceType::KING, Color::WHITE) |
                                    board.pieces(PieceType::KING, Color::BLACK));

  // Occupied restricts this to pieces that are actually still "on the
  // board" in this hypothetical position (relevant once see() starts
  // removing pieces from `occupied` as the exchange progresses).
  return attackers & occupied;
}

/*
Picks the least valuable attacker of `color` out of `attackers`, clears it
from the bitboard, and reports its piece type via `outType`. Returns
Square::underlying::NO_SQ if `color` has no attacker left in the set.
*/
chess::Square Search::popLeastValuableAttacker(chess::Bitboard &attackers, chess::Color color,
                                               chess::PieceType &outType) const {
  using namespace chess;

  static constexpr PieceType::underlying order[6] = {
      PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
      PieceType::ROOK, PieceType::QUEEN,  PieceType::KING};

  for (PieceType::underlying pt : order) {
    const Bitboard bb = attackers & board.pieces(PieceType(pt), color);
    if (bb) {
      const Square sq(bb.lsb());
      attackers.clear(sq.index());
      outType = pt;
      return sq;
    }
  }

  return Square::underlying::NO_SQ;
}

/*
Static Exchange Evaluation.
Simulates the full sequence of captures on move.to() -- both sides always
recapturing with their least valuable attacker -- and returns the net
material result in centipawns from the perspective of the side making
`move`. Doesn't touch the real board; walks a hypothetical occupancy
bitboard instead. Handles en passant and promotion.
*/
int Search::see(chess::Move move) {
  using namespace chess;

  const Square from = move.from();
  const Square to = move.to();

  const Piece attackerPiece = board.at(from);
  const Piece capturedPiece = board.at(to);

  Bitboard occupied = board.occ();

  int gain[32];
  int d = 0;

  if (move.typeOf() == Move::ENPASSANT) {
    // The captured pawn sits beside `to`, not on it.
    const Square epSq(to.file(), from.rank());
    gain[0] = SEE_VALUES[static_cast<int>(PieceType::PAWN)];
    occupied.clear(epSq.index());
  } else {
    gain[0] = SEE_VALUES[capturedPiece.type()];
  }

  PieceType currentAttackerType = attackerPiece.type();

  if (move.typeOf() == Move::PROMOTION) {
    // Credit the pawn -> promoted-piece material swing up front, and the
    // piece now sitting on `to` for the rest of the exchange is the
    // promoted piece, not a pawn.
    gain[0] += SEE_VALUES[move.promotionType()] - SEE_VALUES[static_cast<int>(PieceType::PAWN)];
    currentAttackerType = move.promotionType();
  }

  occupied.clear(from.index());  // the initial attacker has now "moved"

  Bitboard attackers = attackersTo(to, occupied);
  Color side = ~attackerPiece.color();  // opponent recaptures next

  while (true) {
    PieceType nextType;
    const Square attackerSq = popLeastValuableAttacker(attackers, side, nextType);
    if (attackerSq == Square::underlying::NO_SQ) break;

    d++;
    gain[d] = SEE_VALUES[currentAttackerType] - gain[d - 1];

    // Pruning: if neither side would ever choose to continue the exchange
    // from here, the rest of the sequence can't change the result.
    if (std::max(-gain[d - 1], gain[d]) < 0) break;

    occupied.clear(attackerSq.index());
    // Recompute from scratch: removing a piece can reveal a slider
    // (bishop/rook/queen) that was previously blocked from `to`.
    attackers = attackersTo(to, occupied);

    currentAttackerType = nextType;
    side = ~side;
  }

  while (d > 0) {
    gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
    d--;
  }

  return gain[0];
}


/* Order moves based on their priority */
void Search::orderMoves(chess::Movelist &moves, chess::Move ttMove, int ply,
                        bool isQuiescence) {
  const chess::Move killer1 = killerMoves[ply][0];
  const chess::Move killer2 = killerMoves[ply][1];

  for (chess::Move &move : moves) {
    int score = 0;

    if (ttMove != chess::Move::NULL_MOVE && move == ttMove) {
      score += 10000;
    }

    // Order captures by SEE instead of raw MVV-LVA, so a capture that
    // loses material (e.g. QxP defended by a pawn) doesn't get ranked
    // above quiet moves just because it grabs a piece.
    if (board.isCapture(move)) {
      int seeValue = see(move);
      if (seeValue >= 0) {
        score += 8000 + seeValue;  // winning/equal captures: above killers
      } else {
        score += 1000 + seeValue;  // losing captures: still above most quiets, below killers
      }
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
    if (move.typeOf() == chess::Move::PROMOTION) {
      if (move.promotionType() == chess::QUEEN)
        score += 900;
      else if (move.promotionType() == chess::ROOK)
        score += 500;
      else if (move.promotionType() == chess::BISHOP)
        score += 320;
      else if (move.promotionType() == chess::KNIGHT)
        score += 300;
    }

    move.setScore(static_cast<int16_t>(score));
  }

  // Sort moves in descending order of score
  std::sort(moves.begin(), moves.end(),
            [](const chess::Move &a, const chess::Move &b) { return a.score() > b.score(); });
}

/* Reach a stable quiet pos before evaluating */
int Search::qsearch(int alpha, int beta, int ply) {
  if (checkHardTimeLimit()) {
    return 0;
  }

  if (ply >= MAX_SEARCH_DEPTH - 1) {
    return evaluate(ply);
  }

  if ((positionsSearched & 2047) == 0) {
    communicate();
  }

  if (stopSearchFlag) {
    return 0;
  }
  positionsSearched++;

  if (isGameOver(board)) {
    if (getGameOverReason(board) == chess::GameResultReason::CHECKMATE)
      return -MATE_SCORE + ply;
    else
      return DRAW_SCORE;
  }

  if (isSearchDraw(board, ply)) {
    return DRAW_SCORE;
  }

  const bool inCheck = board.inCheck();

  int standPat = 0;
  if (!inCheck) {
    // Stand-pat is only sound when not in check: it assumes "doing
    // nothing" is a reasonable option, which isn't true when you're in
    // check and forced to respond somehow.

     int standPat = evaluate(ply);

    if (standPat >= beta) {
      return beta;
    }
    alpha = std::max(alpha, standPat);
  }

  chess::Movelist allMoves;
  chess::movegen::legalmoves<chess::movegen::MoveGenType::ALL>(allMoves, board);

  chess::Movelist moves;
  for (chess::Move m : allMoves) {
    if (inCheck || board.isCapture(m) || m.typeOf() == chess::Move::PROMOTION) {
      moves.add(m);
    }
  }

  orderMoves(moves, chess::Move::NULL_MOVE, ply, true);

  for (chess::Move move : moves) {
    // SEE pruning: skip captures that lose material outright. Standing
    // pat already covers "this position is fine without capturing", so
    // there's no point recursing into a capture that hands over material
    // for nothing. Skipped while in check, since qsearch doesn't generate
    // evasions here and every capture may be forced.
    if (!inCheck && board.isCapture(move) && see(move) < 0) {
      continue;
    }

    // Incremental NNUE update
    accStack[ply + 1] = accStack[ply];
    nnue.updateAccumulator(board, move, accStack[ply + 1]);

    board.makeMove(move);
    int score = -qsearch(-beta, -alpha, ply + 1);
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

int Search::getPieceValue(chess::Piece piece) {
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

int Search::evaluate(int ply) { return nnue.evaluate(board.sideToMove(), accStack[ply]); }

bool Search::isGameOver(const chess::Board &board) {
  auto result = board.isGameOver();
  return result.second != chess::GameResult::NONE;
}

chess::GameResultReason Search::getGameOverReason(const chess::Board &board) {
  auto result = board.isGameOver();
  return result.first;
}

bool Search::isSearchDraw(const chess::Board &board, int ply) {
  if (ply == 0) {
    return false;
  }

  if (board.isRepetition(1)) {
    return true;
  }

  return board.isHalfMoveDraw() &&
         board.getHalfMoveDrawType().second == chess::GameResult::DRAW;
}

void Search::printInfoLine(int bestScore, std::vector<chess::Move> bestLine,
                           int currentDepth, long long nps,
                           long long elapsedTime) {
  std::stringstream info_ss;
  info_ss << "info depth " << currentDepth << " nodes " << positionsSearched
          << " time " << elapsedTime << " nps " << nps << " score ";

  if (std::abs(bestScore) > (MATE_SCORE - MATE_THRESHHOLD)) {
    int movesToMate;
    if (bestScore > 0) {  // White is mating
      movesToMate = MATE_SCORE - bestScore;
    } else {
      movesToMate = MATE_SCORE + bestScore;
    }
    int fullMovesToMate = (movesToMate + 1) / 2;
    info_ss << "mate " << (bestScore > 0 ? fullMovesToMate : -fullMovesToMate)
            << " pv ";
  } else {
    info_ss << "cp " << bestScore << " pv ";
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

bool Search::manageTime(long long elapsedTime, bool bestMoveChanged) {
  return timeManager.shouldStopAfterIteration(elapsedTime, bestMoveChanged);
}

bool Search::checkHardTimeLimit() {
  if (timeManager.hardLimitReached()) {
    stopSearchFlag = true;
    return true;
  }
  return false;
}

long long Search::getElapsedTime() {
  return timeManager.elapsedMs();
}

void Search::setTimeValues(const GoOptions& options) {
  timeManager.setTimeValues(options);
}
