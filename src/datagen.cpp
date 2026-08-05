#include "datagen.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include "chess.hpp"
#include "constants.hpp"
#include "search.hpp"
#include "tt.hpp"

namespace Datagen {

namespace {

struct RecordedPosition {
  std::string fen;
  int score;             // STM-relative centipawns at the moment of recording
  chess::Color stm;      // side to move in that fen
};

// Plays `plies` uniformly-random legal moves from the current position.
// Returns false if the game ended (checkmate/stalemate) during the random
// phase, in which case the caller should discard the game and start a fresh
// one rather than trying to train on a near-empty/degenerate game.
bool playRandomOpening(chess::Board &board, int plies, std::mt19937_64 &rng) {
  for (int i = 0; i < plies; ++i) {
    if (board.isGameOver().second != chess::GameResult::NONE) return false;

    chess::Movelist moves;
    chess::movegen::legalmoves(moves, board);
    if (moves.empty()) return false;

    std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
    board.makeMove(moves[dist(rng)]);
  }
  return true;
}

// Final game result, from White's point of view: 1.0 win, 0.5 draw, 0.0 loss.
double resultFromGameOver(const chess::Board &board) {
  auto [reason, result] = board.isGameOver();
  const chess::Color stmAtEnd = board.sideToMove();

  if (result == chess::GameResult::DRAW) return 0.5;

  if (result == chess::GameResult::LOSE) {
    // The side to move at the terminal position is the one who lost
    // (checkmated / no-legal-move-equivalent in this codepath).
    return (stmAtEnd == chess::Color::WHITE) ? 0.0 : 1.0;
  }

  // GameResult::WIN for the side to move doesn't currently arise from
  // isGameOver() as implemented, but handle it defensively.
  if (result == chess::GameResult::WIN) {
    return (stmAtEnd == chess::Color::WHITE) ? 1.0 : 0.0;
  }

  return 0.5;  // Should be unreachable (caller only invokes this once
               // isGameOver() has already confirmed the game ended).
}

void playOneGame(chess::Board &board, TranspositionTable &tt, Search &search,
                  const DatagenOptions &opts, std::mt19937_64 &rng,
                  std::vector<RecordedPosition> &outPositions,
                  double &outWhiteResult) {
  outPositions.clear();
  board = chess::Board();  // reset to startpos
  tt.clear_table();

  if (!playRandomOpening(board, opts.randomPlies, rng)) {
    outWhiteResult = -1.0;  // sentinel: discard this game
    return;
  }

  int whiteStreak = 0;
  int blackStreak = 0;

  for (int ply = 0; ply < opts.maxGameLength; ++ply) {
    if (board.isGameOver().second != chess::GameResult::NONE) {
      outWhiteResult = resultFromGameOver(board);
      return;
    }

    search.setSilent(true);
    search.searchBestMove(opts.searchDepth);
    const int score = search.getLastScore();
    const chess::Move move = search.getLastBestMove();

    if (move == chess::Move::NULL_MOVE) {
      // No legal move but isGameOver() said NONE — shouldn't happen, bail
      // out safely rather than looping forever.
      outWhiteResult = 0.5;
      return;
    }

    // Record BEFORE making the move: this fen's side to move is the one the
    // score/move apply to.
    const bool tacticalPosition = board.inCheck();
    const bool scoreIsMateDistance = std::abs(score) > opts.mateScoreFilter;
    if (!tacticalPosition && !scoreIsMateDistance) {
      outPositions.push_back({board.getFen(), score, board.sideToMove()});
    }

    // Early adjudication: track sustained advantage from White's POV.
    const int whiteRelScore =
        (board.sideToMove() == chess::Color::WHITE) ? score : -score;
    if (whiteRelScore > opts.adjudicateMarginCp) {
      ++whiteStreak;
      blackStreak = 0;
    } else if (whiteRelScore < -opts.adjudicateMarginCp) {
      ++blackStreak;
      whiteStreak = 0;
    } else {
      whiteStreak = 0;
      blackStreak = 0;
    }

    if (whiteStreak >= opts.adjudicateStreak) {
      outWhiteResult = 1.0;
      return;
    }
    if (blackStreak >= opts.adjudicateStreak) {
      outWhiteResult = 0.0;
      return;
    }

    board.makeMove(move);
  }

  // Hit the ply cap without a decisive result or adjudication — call it a
  // draw rather than biasing the dataset toward whichever side happened to
  // be on move.
  outWhiteResult = 0.5;
}

}  // namespace

void run(const DatagenOptions &options) {
  std::mt19937_64 rng(options.seed != 0
                           ? options.seed
                           : std::random_device{}());

  chess::Board board;
  TranspositionTable tt(options.ttMegabytes);
  Search search(board, tt);

  std::ofstream out(options.outputPath, std::ios::app);
  if (!out.is_open()) {
    std::cerr << "Failed to open output file: " << options.outputPath << "\n";
    return;
  }

  const auto startTime = std::chrono::steady_clock::now();
  long long gamesPlayed = 0;
  long long positionsWritten = 0;

  std::vector<RecordedPosition> gamePositions;
  gamePositions.reserve(static_cast<size_t>(options.maxGameLength));

  while (gamesPlayed < options.numGames) {
    double whiteResult = 0.5;
    playOneGame(board, tt, search, options, rng, gamePositions, whiteResult);

    if (whiteResult < 0.0) {
      // Discarded (degenerate random opening) — don't count it, just retry.
      continue;
    }

    for (const auto &pos : gamePositions) {
      const double stmResult =
          (pos.stm == chess::Color::WHITE) ? whiteResult : (1.0 - whiteResult);
      out << pos.fen << " | " << pos.score << " | " << stmResult << "\n";
    }
    positionsWritten += static_cast<long long>(gamePositions.size());
    ++gamesPlayed;

    if (gamesPlayed % options.progressEvery == 0) {
      out.flush();
      const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                                std::chrono::steady_clock::now() - startTime)
                                .count();
      const double gamesPerSec =
          elapsed > 0 ? static_cast<double>(gamesPlayed) / elapsed : 0.0;
      std::cout << "[datagen] games=" << gamesPlayed << "/" << options.numGames
                << " positions=" << positionsWritten
                << " elapsed=" << elapsed << "s"
                << " games/s=" << gamesPerSec << std::endl;
    }
  }

  out.flush();
  std::cout << "[datagen] done. games=" << gamesPlayed
            << " positions=" << positionsWritten << " -> "
            << options.outputPath << std::endl;
}

}  // namespace Datagen
