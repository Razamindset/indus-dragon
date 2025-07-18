#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <atomic>
#include <chrono>
#include <climits>
#include <string>
#include <unordered_map>

#include "chess.hpp"

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

using namespace chess;

// Transposition table entry types
enum class TTEntryType {
  EXACT, // Exact score for the position
  LOWER, // Lower bound (alpha cutoff)
  UPPER  // Upper bound (beta cutoff)
};

// Structure for transposition table entries
struct TTEntry {
  uint64_t hash;    // Zobrist hash of the position
  int score;        // Evaluation score
  int depth;        // Depth at which the position was evaluated
  TTEntryType type; // Type of entry
  Move bestMove;    // Best move found for this position
};

class Engine {
private:
  Board board;
  std::atomic<bool> stopSearchFlag{false};
  int getPieceValue(Piece piece);
  long long positionsSearched = 0;

  // Time management
  int wtime = 0;
  int btime = 0;
  int winc = 0;
  int binc = 0;
  int movestogo = 0;
  int movetime = 0;
  bool time_controls_enabled = false;

  long long soft_time_limit = 0;
  long long hard_time_limit = 0;
  int best_move_changes = 0;
  Move last_iteration_best_move = Move::NULL_MOVE;

  void calculateSearchTime();

  std::chrono::steady_clock::time_point search_start_time;
  int allocated_time = 0;

  // Transposition table
  std::unordered_map<uint64_t, TTEntry> transpositionTable;
  int ttHits = 0;       // Number of search matches
  int ttCollisions = 0; // Number of overwrites
  int ttStores = 0;     // Total stores
  void storeTT(uint64_t hash, int depth, int score, TTEntryType type,
               Move bestMove, int ply);
  bool probeTT(uint64_t hash, int depth, int &score, int alpha, int beta,
               Move &bestMove, int ply);

  // History Table
  // 12 pieces 6 of each type and 64 squares.
  int historyTable[12][64];
  void clearHistoryTable();
  void updateHistoryScore(chess::Piece piece, chess::Square to, int depth);

  // Evaluation
  int evaluate(int ply);
  void evaluatePST(int &eval, bool isEndgame);
  void evaluatePawns(int &eval, const chess::Bitboard &whitePawns,
                     const chess::Bitboard &blackPawns);
  void evaluateKingEndgameScore(int &eval);
  bool hasCastled(Color color);
  bool isPassedPawn(Square sq, Color color, const chess::Bitboard &pawns);
  inline int manhattanDistance(Square s1, Square s2) {
    return std::abs(s1.file() - s2.file()) + std::abs(s1.rank() - s2.rank());
  }

  // Search
  int minmax(int depth, int alpha, int beta, bool isMaximizing,
             std::vector<Move> &pv, int ply);
  int quiescenceSearch(int alpha, int beta, bool isMaximizing, int ply);

  // Move ordering and heuristics
  void orderMoves(Movelist &moves, Move ttMove, int ply);
  void orderQuiescMoves(Movelist &moves);
  Move killerMoves[MAX_SEARCH_DEPTH][2];
  void clearKiller();

public:
  void setPosition(const std::string &fen);
  void printBoard();
  void initilizeEngine();
  void setSearchLimits(int wtime, int btime, int winc, int binc, int movestogo,
                       int movetime);

  // Client accessible get best move function
  std::string getBestMove();

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

  void stopSearch() { stopSearchFlag = true; }

  // table stats
  void printTTStats() const;
};

#endif
