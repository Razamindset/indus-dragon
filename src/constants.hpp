#pragma once

#include <climits>

constexpr int MATE_SCORE = 1000000;
constexpr int MATE_THRESHHOLD = 100;
constexpr int DRAW_SCORE = 0;

// Transpostion table size or count
//  2^22 to 4,194,304 entries. Assuming each TTEntry is around 24 bytes (8 bytes
//  for hash, 4 for score, 4 for depth, 4 for type, and 4 for move), this would
//  consume approximately 100 MB of RAM (4,194,304 * 24 bytes = 100,663,296
//  bytes).
constexpr size_t MAX_TT_ENTRIES = 1 << 21;

// For stop search Flag
constexpr int INCOMPLETE_SEARCH = INT_MIN;

// MAX search depth
constexpr int MAX_SEARCH_DEPTH = 24;