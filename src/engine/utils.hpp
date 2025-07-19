#pragma once
#include "chess.hpp"

// Transposition table entry types
enum class TTEntryType {
  EXACT, // Exact score for the position
  LOWER, // Lower bound (alpha cutoff)
  UPPER  // Upper bound (beta cutoff)
};

// Structure for transposition table entries
struct TTEntry {
  uint64_t hash;        // Zobrist hash of the position
  int score;            // Evaluation score
  int depth;            // Depth at which the position was evaluated
  TTEntryType type;     // Type of entry
  chess::Move bestMove; // Best move found for this position
};

struct CalculatedTime {
  long long soft_time;
  long long hard_time;
};
