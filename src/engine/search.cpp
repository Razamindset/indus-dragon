#include "engine.hpp"

/* Order moves based on their priority */
void Engine::orderMoves(Movelist &moves, Move ttMove, int ply){
  std::vector<std::pair<Move, int>> scoredMoves;
  scoredMoves.reserve(moves.size());

  // Get the killer moves
  Move killer1 = killerMoves[ply][0];
  Move killer2 = killerMoves[ply][1];

  // Loop through each move and assign it a score
  for(Move move:moves){
    int score = 0;
    
    // Priorotize ttMove and break after putthing on top
    if(ttMove != Move::NULL_MOVE && move == ttMove){
      scoredMoves.emplace_back(move, 10000);
      continue;
    }

    // KIller moves score
    if (move == killer1) {
      score = 9000; // High score for primary killer
    } else if (move == killer2) {
      score = 8500; // Slightly lower score for secondary killer
    }

    // Todo Find a good way to check for checks can't use board.makemove() and then see if in check

    // Prioritize captures using MVV-LVA
    if (board.isCapture(move)) {
      Piece attacker = board.at(move.from());
      Piece victim = board.at(move.to());
      score += 100 * getPieceValue(victim) - getPieceValue(attacker);
    }
    else {
      Piece movingPiece = board.at(move.from());
      int pieceIndex = static_cast<int>(movingPiece);
      int squareIndex = move.to().index();
      score += historyTable[pieceIndex][squareIndex]; 
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
    return 0;
  }

  if (time_controls_enabled) {
      auto current_time = std::chrono::steady_clock::now();
      auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - search_start_time).count();
      if (elapsed_time >= allocated_time) {
          stopSearchFlag = true;
      }
  }

  if (stopSearchFlag) {
      return 0;
  }

  positionsSearched++;
  pv.clear();

  if(depth == 0 || isGameOver()){
    return quiescenceSearch(alpha, beta, isMaximizing, ply);
  }

  uint64_t boardhash = board.hash();
  int ttScore = 0;
  Move ttMove = Move::NULL_MOVE;
  int originalAlpha = alpha;

  // We will also do a a serach for the TT move so we can efficiently construct the pv line
  if (probeTT(boardhash, depth, ttScore, alpha, beta, ttMove, ply)) {
    if (transpositionTable.at(boardhash).type == TTEntryType::EXACT && ttMove != Move::NULL_MOVE) {
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
    board.makeMove(move); // Make the move

    std::vector<Move> childPv;
    int eval;

    // Todo: Add Late move reduction.
    // Moves that occur late in the list are first searched with a shalow depth similar to PVS

    // Todo: Add Null move pruning
    // Give the opponent 2 moves in a row and if still u are better than donot search

    // Todo: Add Fulfility pruning
    // StandPat + fulfility margin < alpha then prune this.

    // Todo: Add Delta pruning

    // Todo: Improve the quiescience search by adding pruning techniques or tables.
    // Modern engines use tts in quiescence search but we are not at that point i believe but i will try.
    // We can try adding some pruning techniques as defined above but until we have a full fleged evaluation function it would be dangerous.
    // We can add a depth limit to our quiescience search. I tried it but just made the engine worse. It was missing stuff.
    
    // Todo: Create a full fleged evlauation function.
    // Evaluation function should consider each type of piece separetely
    // Some other stuff to improve the eval...
    
    // ! After some testing I found out that PVS was making the engine faster but not stronger so it is not in use for now
    //* PVS - Principal Variation Search
    // We do a full search for the first move. Considering it is the best move.
    // Then we take a sneak peak at the other moves with a reduced window to see if they are promising.
    // If yes then we redo a full search for them.
    
    eval = minmax(depth - 1, alpha, beta, !isMaximizing, childPv, ply + 1);
    board.unmakeMove(move);

    // --- Update best score and PV ---
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

    // --- Alpha-beta cutoff ---
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

  // Store the entries as exact, upper or lower
  TTEntryType entryType;
  if (bestScore <= originalAlpha) {
    entryType = TTEntryType::UPPER;
  } else if (bestScore >= beta) {
    entryType = TTEntryType::LOWER;
  } else {
    entryType = TTEntryType::EXACT;
  }

  storeTT(boardhash, depth, bestScore, entryType, bestMove);

  return bestScore;
}

/* Reach a stable quiet pos before evaluating */
int Engine::quiescenceSearch(int alpha, int beta, bool isMaximizing, int ply) {
    if (stopSearchFlag) {
        return 0;
    }
    positionsSearched++;

    if (isGameOver()) {
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

void Engine::orderQuiescMoves(Movelist& moves) {
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
              [](const auto& a, const auto& b) { return a.second > b.second; });

    moves.clear();
    for (const auto& [move, score] : scoredMoves) {
        moves.add(move);
    }
}

void Engine::calculateSearchTime() {
  // Handle "go infinite" - when no time controls are given.
  if (movetime <= 0 && wtime <= 0 && btime <= 0) {
    // Set time limits to a very large value to simulate infinity.
    // The search will continue until a "stop" command or max depth is reached.
    const long long infinite_time = 1000LL * 60 * 60 * 24; // 24 hours in ms
    soft_time_limit = infinite_time;
    hard_time_limit = infinite_time;
    allocated_time = infinite_time;
    // The check inside minmax should be disabled for infinite search
    // so it doesn't stop prematurely based on a previous game's time.
    time_controls_enabled = false;
    return;
  }

  // If we are here, time controls are active for this search.
  time_controls_enabled = true;

  // If movetime is fixed, all limits are the same.
  if (movetime > 0) {
    soft_time_limit = movetime;
    hard_time_limit = movetime;
    // Use allocated_time for the hard stop with a small buffer
    allocated_time = movetime > 50 ? movetime - 50 : 0;
    return;
  }

  int remaining_time;
  int increment;
  // Estimate 40 moves left in the game if not specified by UCI.
  int moves_to_go = movestogo > 0 ? movestogo : 40;

  if (board.sideToMove() == Color::WHITE) {
    remaining_time = wtime;
    increment = winc;
  } else {
    remaining_time = btime;
    increment = binc;
  }

  // A safe portion of time to use for the move.
  // We don't want to use more than 50% of our remaining time on a single move.
  int safe_time = remaining_time * 0.5;

  // Calculate the ideal time for this move.
  int ideal_time = (remaining_time / moves_to_go) + (increment / 2);

  // Our soft target is the ideal time.
  soft_time_limit = ideal_time;

  // The hard limit is much larger, giving us flexibility.
  hard_time_limit = ideal_time * 2;

  // But never let the hard limit exceed our safe time.
  if (hard_time_limit > safe_time) {
    hard_time_limit = safe_time;
  }

  // The allocated_time is the hard limit that the minmax function will use to force a stop.
  // We leave a small buffer to ensure we don't lose on time.
  allocated_time = hard_time_limit > 50 ? hard_time_limit - 50 : 0;
}

void Engine::clearKiller(){
  for(int i = 0; i<MAX_SEARCH_DEPTH; ++i){
    killerMoves[i][0] = Move::NULL_MOVE;
    killerMoves[i][1] = Move::NULL_MOVE;
  }
}

void Engine::clearHistoryTable(){
  for (int i = 0; i < 12; ++i) {
    for (int j = 0; j < 64; ++j) {
      historyTable[i][j] = 0;
    }
  }
}

void Engine::updateHistoryScore(chess::Piece piece, chess::Square to, int depth){
  int pieceIndex = static_cast<int>(piece);
  historyTable[pieceIndex][to.index()] += depth*depth;
}

std::string Engine::getBestMove() {
  stopSearchFlag = false;
  if (isGameOver()) {
    return "";
  }
  int maxDepth = MAX_SEARCH_DEPTH;

  calculateSearchTime();
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
        if (last_iteration_best_move != Move::NULL_MOVE && currentBestLine.front() != last_iteration_best_move) {
            best_move_changes++;
        }
        bestMove = currentBestLine.front();
        bestLine = currentBestLine;
        last_iteration_best_move = bestMove;
    }

    // Elapsed Time and NPS
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - search_start_time).count();
    
    long long nps = 0;

    if (elapsed_time > 0) { 
      // Calculate nodes per second as (nodes / milliseconds) * 1000
      nps = (positionsSearched * 1000) / elapsed_time;
    }

    // UCI output
    std::cout << "info depth " << currentDepth << " nodes " << positionsSearched << " time " << elapsed_time << " nps " << nps << " score ";

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
    std::cout << std::endl;


    // Check if we should stop.
    if (elapsed_time >= soft_time_limit) {
        // If the best move has been unstable, give it a bit more time,
        // but only if we are not close to our hard limit.
        if (best_move_changes >= 2 && elapsed_time < hard_time_limit / 3) {
            // Extend time by a small, fixed amount (e.g., 30% of the original soft limit)
            soft_time_limit += soft_time_limit * 0.3;
            best_move_changes = 0; // Reset counter for the next potential extension
        } else {
            // Otherwise, the search is stable enough or we are out of time, so stop.
            break;
        }
    }
  }

  if (bestMove == Move::NULL_MOVE) {
    Movelist moves;
    movegen::legalmoves(moves, board);
    if (!moves.empty()) {
        bestMove = moves[0];
    }
    else {
        return "";
    }
  }

  return uci::moveToUci(bestMove);
}
