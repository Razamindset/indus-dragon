#pragma once

#include <cstdint>
#include <string>

namespace Datagen {

struct DatagenOptions {
  std::string outputPath = "datagen_out.txt";
  long long numGames = 1000;

  // Fixed-depth search used to pick moves and label positions. Kept shallow
  // on purpose — datagen needs raw throughput (positions/sec), not maximum
  // per-move strength. Depth 6-8 is the usual sweet spot for hobby engines.
  int searchDepth = 7;

  // Random legal moves played from the startpos before search-driven play
  // begins, to diversify the opening distribution instead of always
  // reaching the same handful of well-known lines.
  int randomPlies = 8;

  // Hard cap on game length (in plies) before we force-adjudicate a draw.
  int maxGameLength = 300;

  // Early adjudication: if the eval stays beyond +/- adjudicateMarginCp for
  // adjudicateStreak consecutive plies (from the same side's perspective),
  // stop the game early and label it decisive. Saves a lot of wasted time
  // on already-decided games.
  int adjudicateMarginCp = 1000;
  int adjudicateStreak = 6;

  // Positions whose |score| exceeds this are excluded from the output —
  // these are "mate-distance" scores (see MATE_SCORE in constants.hpp),
  // not real centipawn evaluations, and would corrupt training if fed to
  // the sigmoid-based loss as if they were normal cp values.
  //
  // NOTE: in practice this is subsumed by quietScoreFilterCp below (which
  // is much tighter), so this only matters if you raise quietScoreFilterCp
  // above it. Kept for clarity/documentation of intent.
  int mateScoreFilter = 90000;

  // Positions with |score| beyond this are excluded from the output. Not
  // because the score is invalid, but because heavily lopsided positions
  // add little training signal and tend to precede forcing sequences.
  // 1500 is a common default used by other datagen tools.
  int quietScoreFilterCp = 1500;

  // Probability per ply (0.0-1.0) of playing a random legal move instead of
  // the engine's best move, to diversify self-play beyond the lines the
  // engine would naturally choose on its own. The position is still
  // labeled with the engine's own search score/best-move judgement — only
  // which move is actually played diverges. 0.01-0.02 is a reasonable
  // starting point.
  double randomMoveProbability = 0.01;

  // Random seed. 0 means "seed from std::random_device" (recommended for
  // real runs). Set explicitly for reproducible smoke tests.
  uint64_t seed = 0;

  // Print a progress line every N completed games.
  long long progressEvery = 50;

  // Small transposition table per game keeps datagen fast without needing a
  // large hash allocation; it's cleared between games for a clean slate.
  size_t ttMegabytes = 8;

  unsigned int numThreads = 1;

  // How often (in total completed games, aggregated across all threads) to
  // merge each worker's newly-written positions into the final combined
  // output file. Lower = less data lost on an abrupt kill, at the cost of
  // slightly more I/O overhead. 20 is a reasonable default.
  long long mergeEveryGames = 20;

  bool appendOutput = false;
};

// Runs self-play games and appends one line per recorded position to
// options.outputPath, in bulletformat's chess text format:
//     <fen> | <score_cp_white_relative> | <result_white_relative>
// where result is 1.0 / 0.5 / 0.0 (win/draw/loss for White). This can be
// fed directly to bullet's text-format loader, or converted to bullet's
// binary format with bullet's own converter tool.
void run(const DatagenOptions &options);

}  // namespace Datagen