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
  TranspositionTable();  // Constructor to initialize the vector
  void storeTT(uint64_t hash, int depth, int score, TTEntryType type,
               chess::Move bestMove, int ply);
  bool probeTT(uint64_t hash, int depth, int &score, int alpha, int beta,
               chess::Move &bestMove, int ply);
  // Table Stats
  void printTTStats() const;

 private:
  std::vector<TTEntry> transpositionTable;
  int ttHits = 0;    // Number of search matches
  int ttStores = 0;  // Total stores
};
