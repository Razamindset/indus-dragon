#pragma once

#include <vector>

#include "chess.hpp"

// Transposition table entry types
enum class TTEntryType {
  EXACT,  // Exact score for the position
  LOWER,  // Lower bound (alpha cutoff)
  UPPER   // Upper bound (beta cutoff)
};

// Structure for transposition table entries
struct TTEntry {
  uint64_t hash;         // Zobrist hash of the position
  int score;             // Evaluation score
  int depth;             // Depth at which the position was evaluated
  TTEntryType type;      // Type of entry
  chess::Move bestMove;  // Best move found for this position
};

class TranspositionTable {
 public:
  explicit TranspositionTable(size_t mb = 16);  // default 16 MB

  // Resize the table to hold roughly `mb` megabytes. Always rounds down to
  // a power-of-two entry count so the `hash & sizeMask` trick keeps working.
  // Wipes all existing entries (unavoidable — the index scheme changes).
  void resize(size_t mb);

  void storeTT(uint64_t hash, int depth, int score, TTEntryType type,
               chess::Move bestMove, int ply);

  bool probeTT(uint64_t hash, int depth, int &score, int alpha, int beta,
               chess::Move &bestMove, int ply);

  void clear_table() {
    for (auto &entry : transpositionTable) {
      entry = TTEntry{};
    }
    ttHits = 0;
    ttStores = 0;
  }

  // Table Stats
  void printTTStats() const;

 private:
  std::vector<TTEntry> transpositionTable;
  size_t sizeMask = 0;  // entries - 1, entries is always a power of two
  int ttHits = 0;       // Number of search matches
  int ttStores = 0;     // Total stores

  static size_t entriesForMB(size_t mb);
};