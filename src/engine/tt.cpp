#include "engine.hpp"

void Engine::printTTStats() const {
  std::cout << "Transposition Table Stats:";
  std::cout << "  TT Hits       : " << ttHits << "";
  std::cout << "  TT Collisions : " << ttCollisions << "";
  std::cout << "  TT Stores     : " << ttStores << "";
  std::cout << "  TT Size       : " << transpositionTable.size() << " / " << MAX_TT_ENTRIES << "";
}

bool Engine::probeTT(uint64_t hash, int depth, int& score, int alpha, int beta, Move& bestMove, int ply){
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
    // The score in the TT is already adjusted for ply from the search that stored it.
    // No further adjustment is needed here.
    int tt_score = entry.score;

    //* Check if:
    // The score is with in the window or
    // The score can cause an alpha or beta cutoff.
    if(entry.type == TTEntryType::EXACT){
      score = tt_score;
      return true;
    }
    if(entry.type == TTEntryType::LOWER && tt_score >= beta){
      score = tt_score;
      return true;
    }
    if(entry.type == TTEntryType::UPPER && tt_score <= alpha){
      score = tt_score;
      return true;
    }
  }

  return false;
}

void Engine::storeTT(uint64_t hash, int depth, int score, TTEntryType type, Move bestMove) {
    auto it = transpositionTable.find(hash);

    if (it != transpositionTable.end()) {
        // Entry exists, update if new entry has greater or equal depth
        if (depth >= it->second.depth) {
            it->second = {hash, score, depth, type, bestMove};
            ttStores++;
        } else {
            ttCollisions++;
        }
    } else {
        // Entry does not exist, add if table is not full
        if (transpositionTable.size() < MAX_TT_ENTRIES) {
            transpositionTable[hash] = {hash, score, depth, type, bestMove};
            ttStores++;
        } else {
            // Table is full, implement a replacement strategy
            // For now, we will use a simple random replacement
            auto toErase = transpositionTable.begin();
            std::advance(toErase, rand() % transpositionTable.size());
            transpositionTable.erase(toErase);

            transpositionTable[hash] = {hash, score, depth, type, bestMove};
            ttStores++;
        }
    }
}