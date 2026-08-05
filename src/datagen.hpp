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
  int mateScoreFilter = 90000;

  // Random seed. 0 means "seed from std::random_device" (recommended for
  // real runs). Set explicitly for reproducible smoke tests.
  uint64_t seed = 0;

  // Print a progress line every N completed games.
  long long progressEvery = 50;

  // Small transposition table per game keeps datagen fast without needing a
  // large hash allocation; it's cleared between games for a clean slate.
  size_t ttMegabytes = 8;
};

// Runs self-play games and appends one line per recorded position to
// options.outputPath, in the format:
//     <fen> | <score_cp_stm_relative> | <result_stm_relative>
// where result is 1.0 / 0.5 / 0.0 (win/draw/loss for the side to move in
// that position). This is a plain-text intermediate format — convert it to
// your training format of choice (e.g. bulletformat) as a separate step.
void run(const DatagenOptions &options);

}  // namespace Datagen
