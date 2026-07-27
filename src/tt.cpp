#include "tt.hpp"

#include "constants.hpp"

size_t TranspositionTable::entriesForMB(size_t mb) {
  if (mb < 1) mb = 1;
  size_t bytes = mb * 1024ULL * 1024ULL;
  size_t entries = bytes / sizeof(TTEntry);

  // round DOWN to nearest power of two, minimum 1
  size_t pow2 = 1;
  while (pow2 * 2 <= entries) pow2 *= 2;
  return pow2;
}

TranspositionTable::TranspositionTable(size_t mb) { resize(mb); }

void TranspositionTable::resize(size_t mb) {
  size_t entries = entriesForMB(mb);
  transpositionTable.assign(entries, TTEntry{});
  sizeMask = entries - 1;
  ttHits = 0;
  ttStores = 0;
}

void TranspositionTable::printTTStats() const {
  std::cout << "Transposition Table Stats:\n";
  std::cout << "  TT Hits       : " << ttHits << "\n";
  std::cout << "  TT Stores     : " << ttStores << "\n";
  std::cout << "  TT Size       : " << transpositionTable.size() << " entries\n";
}

bool TranspositionTable::probeTT(uint64_t hash, int depth, int &score,
                                 int alpha, int beta, chess::Move &bestMove,
                                 int ply) {
  const size_t index = hash & sizeMask;
  const TTEntry &entry = transpositionTable[index];

  if (entry.hash != hash) {
    return false;
  }

  ttHits++;
  bestMove = entry.bestMove;

  if (entry.depth >= depth) {
    int tt_score = entry.score;
    if (std::abs(tt_score) >= MATE_SCORE - MATE_THRESHHOLD) {
      tt_score += (tt_score > 0 ? -ply : ply);  // Adjust for current ply
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
  if (std::abs(score) >= MATE_SCORE - MATE_THRESHHOLD) {
    score += (score > 0 ? ply : -ply);  // Adjust to ply 0
  }

  const size_t index = hash & sizeMask;
  transpositionTable[index] = {hash, score, depth, type, bestMove};
  ttStores++;
}