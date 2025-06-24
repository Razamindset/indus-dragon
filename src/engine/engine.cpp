#include "engine.hpp"
#include "piece-maps.hpp"

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

void Engine::printTTStats() const {
  std::cout << "Transposition Table Stats:\n";
  std::cout << "  TT Hits       : " << ttHits << "\n";
  std::cout << "  TT Collisions : " << ttCollisions << "\n";
  std::cout << "  TT Stores     : " << ttStores << "\n";
  std::cout << "  TT Size       : " << transpositionTable.size() << " / " << MAX_TT_ENTRIES << "\n";
}

bool Engine::probeTT(uint64_t hash, int depth, int& score, int alpha, int beta, Move& bestMove){
  //! The TT stored mate scores are having bugss 

  auto it = transpositionTable.find(hash);

  if(it == transpositionTable.end()){
    return false;
  }

  const TTEntry entry = it->second;
  ttHits++;

  // Always extract the best move for move ordering even if the entry is unusable
  bestMove = entry.bestMove;

  // Check if the depth is acceptable
  if(entry.depth >= depth){
    //! If dealing with mate the already scored valued can cause issues
    // If the prev value was stored at Mate_score-3 but if we are 1 step closer now
    // The tt will still read the old value but we need to deal with it using the depth
    // * This was the issue i could not identify in my pawnstar engine that was causing issues.
    // The goal of the TT is to store a mate score such that when retrieved,
    // it correctly reflects the mate distance from the current node where it's being retrieved.
    int adjustedScore = entry.score;

    // Using 1000 points as threshold as in the getBestMovefunction();
    if(entry.score > MATE_SCORE - MATE_THRESHHOLD){
      adjustedScore = entry.score - (entry.depth - depth);
    }else if(entry.score < -MATE_SCORE + 1000){
      adjustedScore = entry.score + (entry.depth - depth);
    }

    //* Check if:
    // The score is with in the window or
    // The score can cause an alpha or beta cutoff.
    if(entry.type == TTEntryType::EXACT){
      score = adjustedScore;
      return true;
    }
    else if(entry.type == TTEntryType::LOWER && entry.score >= beta){ 
      score = adjustedScore;
      return true;
    }
    else if(entry.type == TTEntryType::UPPER && entry.score <= alpha){
      score = adjustedScore;
      return true;
    }
  }

  return false;
}

// Store an entry in the transposition table
void Engine::storeTT(uint64_t hash, int depth, int score, TTEntryType type, Move bestMove){
  ttStores++;

  auto it = transpositionTable.find(hash);
  if(it != transpositionTable.end()){
    ttCollisions++;
    if(depth >= it->second.depth){
      it->second = {hash, score, depth, type, bestMove};
    }
  }
  else{
    if (transpositionTable.size() >= MAX_TT_ENTRIES) {
      // Simple scheme: randomly erase one
      auto toErase = transpositionTable.begin();
      std::advance(toErase, rand() % transpositionTable.size());
      transpositionTable.erase(toErase);
    }
  }

  // Insert new entry
  transpositionTable[hash] = {hash, score, depth, type, bestMove};
}

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


// Evaluation related functions
bool Engine::hasCastled(Color color) {
  Square kingSq = board.pieces(PieceType::KING, color).lsb();

  if (color == Color::WHITE)
    return kingSq.index() == 6 || kingSq.index() == 2;  // G1 or C1
  else
    return kingSq.index() == 62 || kingSq.index() == 58;  // G8 or C8
}

int Engine::evaluate(int ply) {
  if (isGameOver()) {
    if (getGameOverReason() == GameResultReason::CHECKMATE) {
      if (board.sideToMove() == Color::WHITE) {
        return -MATE_SCORE + ply;
      } else {
        return MATE_SCORE - ply;
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


  // For now endgame if queens are off the board
  bool isEndgame = (board.pieces(PieceType::QUEEN, Color::WHITE).count()  + board.pieces(PieceType::QUEEN, Color::BLACK).count() == 0);

  // Add values from piece square tables
  for (Square sq = 0; sq < 64; sq++) {
    Piece piece = board.at(sq);
    if (piece.type() == PieceType::NONE) continue;

    // This loop starts priniting from white's perspective 
    // White last rook gets an index of 0 which is mirrored so we use the mirrored function for white only and for black we just get value as it is. The only differense is for white +value and for black -ive value 
    int index = (piece.color() == Color::WHITE) ? mirrorIndex(sq.index()) : sq.index();
    int squareValue = 0;

    switch (piece.type()) {
      case PAWN:
        squareValue = PAWN_TABLE[index];
        break;
      case KNIGHT:
        squareValue = KNIGHT_TABLE[index];
        break;
      case BISHOP:
        squareValue = BISHOP_TABLE[index];
        break;
      case ROOK:
        squareValue = ROOK_TABLE[index];
        break;
      case QUEEN:
        squareValue = QUEEN_TABLE[index];
        break;
      case KING:
        squareValue =
            isEndgame ? KING_END_TABLE[index] : KING_MIDDLE_TABLE[index];
        break;
    }

    eval += (piece.color() == Color::WHITE) ? squareValue : -squareValue;
  }

  // kings should prefer castling
  if (!isEndgame) {
    if (hasCastled(Color::WHITE)) eval += 50;
    if (hasCastled(Color::BLACK)) eval -= 50;
  }

  return eval;
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
    return quiesce(alpha, beta, ply);
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
    // std::cout<<"Move: "<<move<<" "<<eval;
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

  printTTStats();

  return uci::moveToUci(bestMove);
}
