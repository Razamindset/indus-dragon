#include "engine.hpp"

/* Order moves based on their priority */
void Engine::orderMoves(Movelist &moves, Move ttMove){
  std::vector<std::pair<Move, int>> scoredMoves;
  scoredMoves.reserve(moves.size());

  // Loop through each move and assign it a score
  for(Move move:moves){
    int score = 0;
    
    // Priorotize ttMove and break after putthing on top
    if(ttMove != Move::NULL_MOVE && move == ttMove){
      scoredMoves.emplace_back(move, 1000);
      continue;
    }

    // Todo Find a good way to check for checks can't use board.makemove() and then see if in check

    // Prioritize captures using MVV-LVA
    if (board.isCapture(move)) {
      score = 0;
      Piece attacker = board.at(move.from());
      Piece victim = board.at(move.to());
      score += 100 * getPieceValue(victim) - getPieceValue(attacker);
    }

    // Prioritize promotions
    if (move.promotionType() == QUEEN) score += 900;
    if (move.promotionType() == ROOK) score += 500;
    if (move.promotionType() == BISHOP) score += 320;
    if (move.promotionType() == KNIGHT) score += 300;

    // Give a slight edge castling
    if(move.typeOf() == Move::CASTLING){
      score += 300;
    }

    // Add the move to the list along with its score
    scoredMoves.emplace_back(move, score);
  }

  // Sort the moves based on scores
  std::sort(scoredMoves.begin(), scoredMoves.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  // Replace original move list with sorted moves
  moves.clear();
  for (const auto& [move, score] : scoredMoves) {
    moves.add(move);
  }
}

int Engine::minmax(int depth, int alpha, int beta, bool isMaximizing, std::vector<Move>& pv, int ply) {
  if (stopSearchFlag) {
    return 0;  // Return a neutral score if search is stopped
  }
  positionsSearched++;

  if(depth == 0 || isGameOver()){
    // return quiescenceSearch(alpha, beta, isMaximizing, ply);
    return evaluate(ply);
  }

  uint64_t boardhash = board.hash();
  int ttScore = 0;
  Move ttMove = Move::NULL_MOVE;
  int originalAlpha = alpha;

  if (probeTT(boardhash, depth, ttScore, alpha, beta, ttMove, ply)) {
    // If we have an EXACT TT hit, we can reconstruct the PV
    if (transpositionTable.at(boardhash).type == TTEntryType::EXACT && ttMove != Move::NULL_MOVE) {
      pv.clear();
      pv.push_back(ttMove);
      // Recursively get the rest of the PV by making the move and calling minmax
      // with a reduced depth. This is a "PV-reconstruction" search.
      board.makeMove(ttMove);
      std::vector<Move> childPv;
      // Call minmax with depth - 1 and the same alpha/beta window.
      // The score from this recursive call is not used, only the childPv.
      minmax(depth - 1, alpha, beta, !isMaximizing, childPv, ply + 1);
      board.unmakeMove(ttMove);
      pv.insert(pv.end(), childPv.begin(), childPv.end());
    }
    return ttScore;
  }

  Movelist moves;
  movegen::legalmoves(moves, board);

  orderMoves(moves, ttMove);

  Move bestMove = Move::NULL_MOVE;
  int bestScore;

  if (isMaximizing) {
    bestScore = -MATE_SCORE;

    for (Move move : moves) {
      board.makeMove(move);
      std::vector<Move> childPv;
      int eval = minmax(depth - 1, alpha, beta, false, childPv, ply+1);
      board.unmakeMove(move);

      if (eval > bestScore) {
        bestScore = eval;
        bestMove = move;
        pv = {move};
        pv.insert(pv.end(), childPv.begin(), childPv.end());
      }

      alpha = std::max(bestScore, alpha);

      if (beta <= alpha) break;
    }

  } else {
    bestScore = MATE_SCORE;

    for (Move move : moves) {
      board.makeMove(move);
      std::vector<Move> childPv;
      int eval = minmax(depth - 1, alpha, beta, true, childPv, ply+1);
      board.unmakeMove(move);

      if (eval < bestScore) {
        bestScore = eval;
        bestMove = move;
        pv = {move};
        pv.insert(pv.end(), childPv.begin(), childPv.end());
      }

      beta = std::min(bestScore, beta);

      if (beta <= alpha) break;
    }
  }

  // Store the entries in the transposition table
  TTEntryType entryType;
  if (bestScore <= originalAlpha) {
    entryType = TTEntryType::UPPER; // All moves were <= alpha (fail-low)
  } else if (bestScore >= beta) {
    entryType = TTEntryType::LOWER; // bestScore >= beta (fail-high)
  }else {
    entryType = TTEntryType::EXACT; // Exact score within alpha-beta window
  }

  storeTT(boardhash, depth, bestScore, entryType, bestMove);

  return bestScore;
}

std::string Engine::getBestMove(int maxDepth) {
  stopSearchFlag = false;  // Reset the stop flag at the beginning of a new search
  if (isGameOver()) {
    return "";
  }

  positionsSearched = 0;

  bool isMaximizing = (board.sideToMove() == Color::WHITE);
  Move bestMove = Move::NULL_MOVE;
  std::vector<Move> bestLine;

  // Iterative Deepening Loop
  for (int currentDepth = 1; currentDepth <= maxDepth; ++currentDepth) {
    int bestEval = isMaximizing ? -MATE_SCORE : MATE_SCORE;
    std::vector<Move> currentBestLine;

    bestEval = minmax(currentDepth, -MATE_SCORE, MATE_SCORE, isMaximizing, currentBestLine, 0);
    
    // After the search at currentDepth is complete, update our overall best move.
    bestMove = currentBestLine.front();
    bestLine = currentBestLine;

    // Print UCI info for the completed depth
    std::cout << "info depth " << currentDepth << " nodes " << positionsSearched << " score ";

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

    for (const auto& move : bestLine) {
      std::cout << move << " ";
    }
    std::cout << "\n";
  }

  if (bestMove == Move::NULL_MOVE) {
    // This should ideally not happen if there are legal moves.
    // As a fallback, grab the first legal move.
    Movelist moves;
    movegen::legalmoves(moves, board);
    if (!moves.empty()) {
        bestMove = moves[0];
    }
    else {
        return "";
    }
  }

  // printTTStats();

  return uci::moveToUci(bestMove);
}