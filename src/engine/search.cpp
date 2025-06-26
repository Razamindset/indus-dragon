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

// Search related functions
int Engine::quiesce(int alpha, int beta, int ply){
  positionsSearched++;
    
  const int MAX_QUIESCE_DEPTH = 16;
  if (ply >= MAX_QUIESCE_DEPTH) {
    return evaluate(ply);
  }

  if (isGameOver()) {
    if (getGameOverReason() == GameResultReason::CHECKMATE){
      return -MATE_SCORE + ply;
    }
    return DRAW_SCORE;
  }

  int standpat = evaluate(ply);

  if (standpat >= beta)
    return beta;
    
  const int DELTA_MARGIN = 900;
  if (standpat + DELTA_MARGIN < alpha) {
    return alpha;
  }
    
  if (standpat > alpha)
    alpha = standpat;
    
  // Generate all legal moves but filter more efficiently
  Movelist moves;
  movegen::legalmoves(moves, board);
    
  // Create a vector of only captures for better cache performance
  std::vector<Move> goodCaptures;
  goodCaptures.reserve(32); // Reserve space to avoid reallocations
    
  for (const Move& move : moves) {
    if (!board.isCapture(move)) continue;
        
    // Quick SEE approximation - skip obviously bad captures
    int victimValue = getPieceValue(board.at(move.to()));
    int attackerValue = getPieceValue(board.at(move.from()));
        
    // Simple SEE: if victim value >= attacker value, it's likely good
    if (victimValue >= attackerValue || victimValue >= 300) { // Always try knight+ captures
      goodCaptures.push_back(move);
    }
  }
    
  // Sort good captures by MVV-LVA
  std::sort(goodCaptures.begin(), goodCaptures.end(), [this](const Move& a, const Move& b) {
    int victimA = getPieceValue(board.at(a.to()));
    int victimB = getPieceValue(board.at(b.to()));
      return victimA > victimB;
  });
    
  for (const Move& move : goodCaptures) {
    board.makeMove(move);
    int score = -quiesce(-beta, -alpha, ply + 1);
    board.unmakeMove(move);

    if (score >= beta)
      return beta;

    if (score > alpha)
      alpha = score;
  }

  return alpha;
}

int Engine::minmax(int depth, int alpha, int beta, bool isMaximizing, std::vector<Move>& pv, int ply) {
  positionsSearched++;

  if(depth == 0){
    // return quiesce(alpha, beta, ply);
    return evaluate(ply);
  }

  if (isGameOver()) {
    return evaluate(ply);
  }

  uint64_t boardhash = board.hash();
  int ttScore = 0;
  Move ttMove = Move::NULL_MOVE;
  int originalAlpha = alpha;

  if (probeTT(boardhash, depth, ttScore, alpha, beta, ttMove)) {
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

std::string Engine::getBestMove(int depth) {
  if (isGameOver()) {
    return "";
  }

  Movelist moves;
  movegen::legalmoves(moves, board);
  orderMoves(moves, Move::NULL_MOVE);

  bool isMaximizing = (board.sideToMove() == Color::WHITE); 

  int bestEval = isMaximizing ? -MATE_SCORE : MATE_SCORE;
  Move bestMove = Move::NULL_MOVE;
  std::vector<Move> bestLine;

  for (Move move : moves) {
    board.makeMove(move);
    std::vector<Move> MovePv;
    int eval = minmax(depth - 1, -MATE_SCORE, MATE_SCORE, !isMaximizing, MovePv, 1);
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
      bestLine = {move};
      bestLine.insert(bestLine.end(), MovePv.begin(), MovePv.end());
    }
  }

  if (bestMove == Move::NULL_MOVE) {
    return "";
  }


  std::cout << "info depth "<< depth << " nodes " << positionsSearched << " score ";

  if (std::abs(bestEval) > (MATE_SCORE - MATE_THRESHHOLD)) { 
    int movesToMate;
    if (bestEval > 0) { // White is mating
      movesToMate = MATE_SCORE - bestEval;
    } else { 
      movesToMate = MATE_SCORE + bestEval;
    }
    int fullMovesToMate = (movesToMate + 1) / 2;

    // Perspective based mate and eval socores
    if((board.sideToMove() == Color::BLACK && bestEval >0 )||
       (board.sideToMove() == Color::WHITE && bestEval <0)){
      fullMovesToMate = -fullMovesToMate;
    }
    std::cout << "mate " << fullMovesToMate << " pv ";
  } else {
     if(board.sideToMove() == Color::BLACK){
      bestEval = - bestEval;
    }
    std::cout << "cp "<< bestEval << " pv ";
  }

  for (Move move : bestLine) {
    std::cout << move << " ";
  }
  std::cout<<"\n";

  // printTTStats();

  return uci::moveToUci(bestMove);
}