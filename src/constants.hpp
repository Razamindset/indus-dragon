#pragma once

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

// MAX search depth
constexpr int MAX_SEARCH_DEPTH = 100;

// Piece values in centipawns
constexpr int PAWN_VALUE = 100;
constexpr int KNIGHT_VALUE = 300;
constexpr int BISHOP_VALUE = 320;
constexpr int ROOK_VALUE = 500;
constexpr int QUEEN_VALUE = 900;

constexpr int TEMPO_BONUS = 15;  // ~0.15 pawns

// Time related
static constexpr long long INFINITE_TIME = 1000000000LL;  // 1 billion ms
static constexpr double SOFT_TIME_FACTOR = 0.4;
static constexpr double HARD_TIME_FACTOR = 2.5;
static constexpr long long MIN_SEARCH_TIME = 10;
static constexpr long long SAFETY_BUFFER = 50;