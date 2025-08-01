
#ifndef PIECE_VALUES_HPP
#define PIECE_VALUES_HPP

constexpr int mirrorIndex(int sq) { return (7 - sq / 8) * 8 + (sq % 8); }

//* These values are so that the white pieces are on top. For black they need to
// be flipped horizontally

constexpr int PAWN_TABLE[64] = {
    0,  0,  0,   0,  0,  0,   0,  0,  5,  10, 10, -20, -20, 10, 10, 5,
    5,  -5, -10, 0,  0,  -10, -5, 5,  0,  0,  0,  20,  20,  0,  0,  0,
    5,  5,  10,  25, 25, 10,  5,  5,  10, 10, 20, 30,  30,  20, 10, 10,
    50, 50, 50,  50, 50, 50,  50, 50, 0,  0,  0,  0,   0,   0,  0,  0};

// Knight piece-square table
constexpr int KNIGHT_TABLE[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0,   5,   5,
    0,   -20, -40, -30, 5,   10,  15,  15,  10,  5,   -30, -30, 0,
    15,  20,  20,  15,  0,   -30, -30, 5,   15,  20,  20,  15,  5,
    -30, -30, 0,   10,  15,  15,  10,  0,   -30, -40, -20, 0,   0,
    0,   0,   -20, -40, -50, -40, -30, -30, -30, -30, -40, -50};

// Bishop piece-square table
constexpr int BISHOP_TABLE[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20, -10, 5,   0,   0,   0,
    0,   5,   -10, -10, 10,  10,  10,  10,  10,  10,  -10, -10, 0,
    10,  10,  10,  10,  0,   -10, -10, 5,   5,   10,  10,  5,   5,
    -10, -10, 0,   5,   10,  10,  5,   0,   -10, -10, 0,   0,   0,
    0,   0,   0,   -10, -20, -10, -10, -10, -10, -10, -10, -20};

// Rook piece-square table
constexpr int ROOK_TABLE[64] = {0,  0,  0,  5,  5, 0,  0,  0, -5, 0, 0,  0,  0,
                                0,  0,  -5, -5, 0, 0,  0,  0, 0,  0, -5, -5, 0,
                                0,  0,  0,  0,  0, -5, -5, 0, 0,  0, 0,  0,  0,
                                -5, -5, 0,  0,  0, 0,  0,  0, -5, 5, 10, 10, 10,
                                10, 10, 10, 5,  0, 0,  0,  0, 0,  0, 0,  0};

// Queen piece-square table
constexpr int QUEEN_TABLE[64] = {
    -20, -10, -10, -5, -5, -10, -10, -20, -10, 0,   5,   0,  0,  0,   0,   -10,
    -10, 5,   5,   5,  5,  5,   0,   -10, 0,   0,   5,   5,  5,  5,   0,   -5,
    -5,  0,   5,   5,  5,  5,   0,   -5,  -10, 0,   5,   5,  5,  5,   0,   -10,
    -10, 0,   0,   0,  0,  0,   0,   -10, -20, -10, -10, -5, -5, -10, -10, -20};

// King middle game piece-square table
constexpr int KING_MIDDLE_TABLE[64] = {
    20,  30,  10,  0,   0,   10,  30,  20,  20,  20,  0,   0,   0,
    0,   20,  20,  -10, -20, -20, -20, -20, -20, -20, -10, -20, -30,
    -30, -40, -40, -30, -30, -20, -30, -40, -40, -50, -50, -40, -40,
    -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50,
    -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30};

// King end game piece-square table
constexpr int KING_END_TABLE[64] = {
    -50, -30, -30, -30, -30, -30, -30, -50, -30, -30, 0,   0,   0,
    0,   -30, -30, -30, -10, 20,  30,  30,  20,  -10, -30, -30, -10,
    30,  40,  40,  30,  -10, -30, -30, -10, 30,  40,  40,  30,  -10,
    -30, -30, -10, 20,  30,  30,  20,  -10, -30, -30, -20, -10, 0,
    0,   -10, -20, -30, -50, -40, -30, -20, -20, -30, -40, -50};

#endif
