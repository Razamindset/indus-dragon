#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <string>
#include<unordered_map>

#include "../chess-library/include/chess.hpp"

constexpr int MATE_SCORE = 1000000;
constexpr int MATE_THRESHHOLD = 100;
constexpr int DRAW_SCORE = 0;
// Around 1 million entries for 64 bites each
constexpr size_t MAX_TT_ENTRIES = 1 << 20;

using namespace chess;


// Transposition table entry types
enum class TTEntryType {
  EXACT,  // Exact score for the position
  LOWER,  // Lower bound (alpha cutoff)
  UPPER   // Upper bound (beta cutoff)
};

// Structure for transposition table entries
struct TTEntry {
  uint64_t hash;     // Zobrist hash of the position
  int score;         // Evaluation score
  int depth;         // Depth at which the position was evaluated
  TTEntryType type;  // Type of entry
  Move bestMove;     // Best move found for this position
};

class Engine {
 private:
  Board board;
  int getPieceValue(Piece piece);
  int positionsSearched = 0;

  // Transposition table
  std::unordered_map<uint64_t, TTEntry> transpositionTable;
  int ttHits = 0;        // Number of search matches
  int ttCollisions = 0;  // Number of overwrites
  int ttStores = 0;      // Total stores
  void storeTT(uint64_t hash, int depth, int score, TTEntryType type, Move bestMove);
  bool probeTT(uint64_t hash, int depth, int& score, int alpha, int beta, Move& bestMove);

  // Evaluation
  int evaluate(int ply);
  bool hasCastled(Color color);

  // Search
  int minmax(int depth, int alpha, int beta, bool isMaximizing, std::vector<Move>& pv, int ply);

  // Move ordering and heuristics
  void orderMoves(Movelist &moves, Move ttMove);

 public:
  void setPosition(const std::string& fen);
  void printBoard();
  void initilizeEngine();

  // Client accessible get best move function
  std::string getBestMove(int depth);

  //* Game state related
  bool isGameOver() {
    auto result = board.isGameOver();
    return result.second != GameResult::NONE;
  }

  GameResultReason getGameOverReason() {
    auto result = board.isGameOver();
    return result.first;
  }

  // Move making used when the chess gui gives a list of move history. use this
  // to set the board state correctly
  void makeMove(std::string move);

  // table stats
  void printTTStats() const;
};

#endif
