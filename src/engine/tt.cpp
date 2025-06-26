#include "engine.hpp"

void Engine::printTTStats() const {
  std::cout << "Transposition Table Stats:";
  std::cout << "  TT Hits       : " << ttHits << "";
  std::cout << "  TT Collisions : " << ttCollisions << "";
  std::cout << "  TT Stores     : " << ttStores << "";
  std::cout << "  TT Size       : " << transpositionTable.size() << " / " << MAX_TT_ENTRIES << "";
}

bool Engine::probeTT(uint64_t hash, int depth, int& score, int alpha, int beta, Move& bestMove){
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