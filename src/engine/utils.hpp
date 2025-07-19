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

// Count the total pieces on the board
int count_pieces(const chess::Board &board) {
  return board.pieces(chess::PieceType::PAWN, chess::Color::WHITE).count() +
         board.pieces(chess::PieceType::PAWN, chess::Color::BLACK).count() +
         board.pieces(chess::PieceType::KNIGHT, chess::Color::WHITE).count() +
         board.pieces(chess::PieceType::KNIGHT, chess::Color::BLACK).count() +
         board.pieces(chess::PieceType::BISHOP, chess::Color::WHITE).count() +
         board.pieces(chess::PieceType::BISHOP, chess::Color::BLACK).count() +
         board.pieces(chess::PieceType::ROOK, chess::Color::WHITE).count() +
         board.pieces(chess::PieceType::ROOK, chess::Color::BLACK).count() +
         board.pieces(chess::PieceType::QUEEN, chess::Color::WHITE).count() +
         board.pieces(chess::PieceType::QUEEN, chess::Color::BLACK).count();
}