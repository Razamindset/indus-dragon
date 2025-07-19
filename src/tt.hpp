#pragma once

#include "chess.hpp"
#include "utils.hpp"
#include <vector>



class TranspositionTable {
public:
  TranspositionTable(); // Constructor to initialize the vector
  void storeTT(uint64_t hash, int depth, int score, TTEntryType type, chess::Move bestMove,
               int ply);
  bool probeTT(uint64_t hash, int depth, int &score, int alpha, int beta, chess::Move &bestMove,
               int ply, TTEntryType &entry_type);
  // Table Stats
  void printTTStats() const;

private:
  std::vector<TTEntry> transpositionTable;
  int ttHits = 0;   // Number of search matches
  int ttStores = 0; // Total stores
};
