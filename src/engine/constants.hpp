#pragma once

#include <climits>

constexpr int MATE_SCORE = 1000000;
constexpr int MATE_THRESHHOLD = 100;
constexpr int DRAW_SCORE = 0;

// Around 1 million entries for 64 bites each
constexpr size_t MAX_TT_ENTRIES = 1 << 20;

// For stop search Flag
constexpr int INCOMPLETE_SEARCH = INT_MIN;

// MAX search depth
constexpr int MAX_SEARCH_DEPTH = 12;

// Material values
constexpr int PAWN_VALUE = 100;
constexpr int KNIGHT_VALUE = 300;
constexpr int BISHOP_VALUE = 320;
constexpr int ROOK_VALUE = 500;
constexpr int QUEEN_VALUE = 900;

// Time
long long INFINITE_TIME = 1000LL * 60 * 60 * 24;