#pragma once

constexpr int MATE_SCORE = 1000000;
constexpr int MATE_THRESHHOLD = 100;
constexpr int DRAW_SCORE = 0;

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

// Late Move Reductions
constexpr int LMR_FULL_DEPTH_MOVES = 3;   // moves searched at full depth before reducing
constexpr int LMR_MIN_DEPTH = 3;          // don't reduce below this remaining depth
constexpr double LMR_DIVISOR = 2.5;       // larger = less aggressive reduction