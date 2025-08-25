#include "tt.hpp"

#include "constants.hpp"

TranspositionTable::TranspositionTable() : transpositionTable(MAX_TT_ENTRIES) {}

void TranspositionTable::printTTStats() const {
  std::cout << "Transposition Table Stats:\n";
  std::cout << "  TT Hits       : " << ttHits << "\n";
  std::cout << "  TT Stores     : " << ttStores << "\n";
  std::cout << "  TT Size       : " << MAX_TT_ENTRIES << "\n";
}

bool TranspositionTable::probeTT(uint64_t hash, int depth, int &score,
                                 int alpha, int beta, chess::Move &bestMove,
                                 int ply, TTEntryType &entry_type) {
  const int index = hash & (MAX_TT_ENTRIES - 1);
  const TTEntry &entry = transpositionTable[index];

  if (entry.hash != hash) {
    return false;
  }

  ttHits++;
  bestMove = entry.bestMove;
  entry_type = entry.type;

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
  int adjustedScore = score;
  if (std::abs(score) >= MATE_SCORE - MATE_THRESHHOLD) {
    adjustedScore += (score > 0 ? ply : -ply);  // Adjust to ply 0
  }

  const int index = hash & (MAX_TT_ENTRIES - 1);
  transpositionTable[index] = {hash, adjustedScore, depth, type, bestMove};
  ttStores++;
}