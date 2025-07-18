#include "tt.hpp"
#include "constants.hpp"

void TranspositionTable::printTTStats() const {
  std::cout << "Transposition Table Stats:\n";
  std::cout << "  TT Hits       : " << ttHits << "\n";
  std::cout << "  TT Collisions : " << ttCollisions << "\n";
  std::cout << "  TT Stores     : " << ttStores << "\n";
  std::cout << "  TT Size       : " << transpositionTable.size() << " / "
            << MAX_TT_ENTRIES << "\n";
}

bool TranspositionTable::probeTT(uint64_t hash, int depth, int &score,
                                 int alpha, int beta, chess::Move &bestMove,
                                 int ply, TTEntryType &entry_type) {
  auto it = transpositionTable.find(hash);
  if (it == transpositionTable.end() || it->second.hash != hash) {
    return false;
  }

  const TTEntry entry = it->second;
  ttHits++;
  bestMove = entry.bestMove;
  entry_type = entry.type;

  if (entry.depth >= depth) {
    int tt_score = entry.score;
    if (std::abs(tt_score) >= MATE_SCORE - MATE_THRESHHOLD) {
      tt_score += (tt_score > 0 ? -ply : ply); // Adjust for current ply
    }

    if (entry.type == TTEntryType::EXACT) {
      score = tt_score;
      return true;
    }
    if (entry.type == TTEntryType::LOWER && tt_score >= beta) {
      score = tt_score;
      return true;
    }
    if (entry.type == TTEntryType::UPPER && tt_score <= alpha) {
      score = tt_score;
      return true;
    }
  }

  return false;
}

void TranspositionTable::storeTT(uint64_t hash, int depth, int score,
                                 TTEntryType type, chess::Move bestMove,
                                 int ply) {
  int adjustedScore = score;
  if (std::abs(score) >= MATE_SCORE - MATE_THRESHHOLD) {
    adjustedScore += (score > 0 ? ply : -ply); // Adjust to ply 0
  }

  auto it = transpositionTable.find(hash);
  if (it != transpositionTable.end()) {
    if (depth >= it->second.depth) {
      it->second = {hash, adjustedScore, depth, type, bestMove};
      ttStores++;
    } else {
      ttCollisions++;
    }
  } else {
    if (transpositionTable.size() < MAX_TT_ENTRIES) {
      transpositionTable[hash] = {hash, adjustedScore, depth, type, bestMove};
      ttStores++;
    } else {
      // Depth-based replacement
      auto toErase = transpositionTable.begin();
      for (auto it = transpositionTable.begin(); it != transpositionTable.end();
           ++it) {
        if (it->second.depth < toErase->second.depth) {
          toErase = it;
        }
      }
      transpositionTable.erase(toErase);
      transpositionTable[hash] = {hash, adjustedScore, depth, type, bestMove};
      ttStores++;
    }
  }
}