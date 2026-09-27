#include "datagen.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>
#include <vector>
#include <memory>     // std::unique_ptr, std::make_unique
#include <algorithm>  // std::min, std::max

#include "chess.hpp"
#include "constants.hpp"
#include "search.hpp"
#include "tt.hpp"

namespace Datagen {

struct RecordedPosition {
  std::string fen;
  int score;
  chess::Color stm;
};

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

double resultFromGameOver(const chess::Board &board) {
  auto [reason, result] = board.isGameOver();
  const chess::Color stmAtEnd = board.sideToMove();

  if (result == chess::GameResult::DRAW) return 0.5;

  if (result == chess::GameResult::LOSE) {
    return (stmAtEnd == chess::Color::WHITE) ? 0.0 : 1.0;
  }

  if (result == chess::GameResult::WIN) {
    return (stmAtEnd == chess::Color::WHITE) ? 1.0 : 0.0;
  }

  return 0.5;
}

void playOneGame(chess::Board &board, TranspositionTable &tt, Search &search,
                  const DatagenOptions &opts, std::mt19937_64 &rng,
                  std::vector<RecordedPosition> &outPositions,
                  double &outWhiteResult) {
  outPositions.clear();
  board = chess::Board();
  tt.clear_table();

  if (!playRandomOpening(board, opts.randomPlies, rng)) {
    outWhiteResult = -1.0;
    return;
  }

  int whiteStreak = 0;
  int blackStreak = 0;
  std::uniform_real_distribution<double> noiseChance(0.0, 1.0);

  for (int ply = 0; ply < opts.maxGameLength; ++ply) {
    if (board.isGameOver().second != chess::GameResult::NONE) {
      outWhiteResult = resultFromGameOver(board);
      return;
    }

    search.setSilent(true);
    search.searchBestMove(opts.searchDepth);
    const chess::Move bestMove = search.getLastBestMove();
    const int score = search.getLastScore();

    if (bestMove == chess::Move::NULL_MOVE) {
      outWhiteResult = 0.5;
      return;
    }

    chess::Move move = bestMove;
    if (noiseChance(rng) < opts.randomMoveProbability) {
      chess::Movelist legalMoves;
      chess::movegen::legalmoves(legalMoves, board);
      std::uniform_int_distribution<size_t> dist(0, legalMoves.size() - 1);
      move = legalMoves[dist(rng)];
    }

    const bool tacticalPosition = board.inCheck();
    const bool scoreIsMateDistance = std::abs(score) > opts.mateScoreFilter;
    const bool scoreTooLopsided = std::abs(score) > opts.quietScoreFilterCp;
    const bool isCapture = board.isCapture(bestMove);

    if (!tacticalPosition && !scoreIsMateDistance && !scoreTooLopsided &&
        !isCapture) {
      outPositions.push_back({board.getFen(), score, board.sideToMove()});
    }

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

  outWhiteResult = 0.5;
}

// Mixes a base seed with a thread index into a well-distributed per-thread
// seed (splitmix64), so adjacent thread indices don't produce correlated
// mt19937_64 streams the way naively adding small offsets can.
uint64_t deriveThreadSeed(uint64_t baseSeed, unsigned int threadIndex) {
  uint64_t z = baseSeed + 0x9E3779B97F4A7C15ULL * (threadIndex + 1);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

// Per-thread worker state, all constructed sequentially on the main thread
// before any thread starts running — Search's constructor loads the NNUE
// network into global arrays, so no two Search objects may ever be
// constructed concurrently.
struct WorkerContext {
  chess::Board board;
  TranspositionTable tt;
  Search search;
  std::mt19937_64 rng;
  std::string tempPath;
  long long gamesToPlay = 0;

  WorkerContext(size_t ttMegabytes, uint64_t seed, std::string path)
      : tt(ttMegabytes), search(board, tt), rng(seed), tempPath(std::move(path)) {}
};

void workerRun(WorkerContext &ctx, const DatagenOptions &opts,
               std::atomic<long long> &totalGamesDone,
               std::atomic<long long> &totalPositionsDone) {
  std::ofstream out(ctx.tempPath, std::ios::trunc);
  if (!out.is_open()) {
    std::cerr << "[datagen] worker failed to open " << ctx.tempPath << "\n";
    return;
  }

  std::vector<RecordedPosition> gamePositions;
  gamePositions.reserve(static_cast<size_t>(opts.maxGameLength));

  long long gamesPlayed = 0;
  while (gamesPlayed < ctx.gamesToPlay) {
    double whiteResult = 0.5;
    playOneGame(ctx.board, ctx.tt, ctx.search, opts, ctx.rng, gamePositions,
                whiteResult);

    if (whiteResult < 0.0) continue;

    for (const auto &pos : gamePositions) {
      const int whiteScore =
          (pos.stm == chess::Color::WHITE) ? pos.score : -pos.score;
      out << pos.fen << " | " << whiteScore << " | " << std::fixed
          << std::setprecision(1) << whiteResult << "\n";
    }

    // Flush after every game so the main thread's periodic merge never
    // reads a partially-written game's positions.
    out.flush();

    totalPositionsDone.fetch_add(static_cast<long long>(gamePositions.size()),
                                  std::memory_order_relaxed);
    ++gamesPlayed;
    totalGamesDone.fetch_add(1, std::memory_order_relaxed);
  }
}

void run(const DatagenOptions &options) {
  unsigned int numThreads = options.numThreads;
  if (numThreads == 0) {
    numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 1;
  }
  numThreads = std::min<unsigned int>(
      numThreads, static_cast<unsigned int>(std::max<long long>(1, options.numGames)));

  const uint64_t baseSeed =
      options.seed != 0 ? options.seed : std::random_device{}();

  const long long gamesPerThread = options.numGames / numThreads;
  long long remainder = options.numGames % numThreads;

  std::vector<std::unique_ptr<WorkerContext>> workers;
  workers.reserve(numThreads);

  for (unsigned int i = 0; i < numThreads; ++i) {
    const uint64_t threadSeed = deriveThreadSeed(baseSeed, i);
    std::string tempPath = options.outputPath + ".part" + std::to_string(i);
    auto ctx = std::make_unique<WorkerContext>(options.ttMegabytes, threadSeed,
                                                tempPath);
    ctx->gamesToPlay = gamesPerThread + (remainder > 0 ? 1 : 0);
    if (remainder > 0) --remainder;
    workers.push_back(std::move(ctx));
  }

  std::atomic<long long> totalGamesDone{0};
  std::atomic<long long> totalPositionsDone{0};

  const auto startTime = std::chrono::steady_clock::now();

  std::vector<std::thread> threads;
  threads.reserve(numThreads);
  for (unsigned int i = 0; i < numThreads; ++i) {
    threads.emplace_back(workerRun, std::ref(*workers[i]), std::cref(options),
                          std::ref(totalGamesDone), std::ref(totalPositionsDone));
  }

  // Combined output stays open for the whole run and is appended to
  // incrementally — it's always in a valid, complete-lines state.
  std::ofstream merged(options.outputPath,
                        options.appendOutput ? std::ios::app : std::ios::trunc);
  if (!merged.is_open()) {
    std::cerr << "[datagen] failed to open merged output: " << options.outputPath
               << "\n";
    for (auto &t : threads) t.join();
    return;
  }

  std::vector<std::streampos> mergedOffset(numThreads, 0);

  auto mergeNewData = [&]() {
    for (unsigned int i = 0; i < numThreads; ++i) {
      std::ifstream part(workers[i]->tempPath, std::ios::binary);
      if (!part.is_open()) continue;

      part.seekg(0, std::ios::end);
      const std::streampos endPos = part.tellg();
      if (endPos <= mergedOffset[i]) continue;  // nothing new since last merge

      part.seekg(mergedOffset[i]);
      const size_t newBytes = static_cast<size_t>(endPos - mergedOffset[i]);
      std::vector<char> buffer(newBytes);
      part.read(buffer.data(), static_cast<std::streamsize>(newBytes));
      merged.write(buffer.data(), static_cast<std::streamsize>(newBytes));
      mergedOffset[i] = endPos;
    }
    merged.flush();
  };

  long long lastReported = 0;
  long long lastMerged = 0;
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    const long long done = totalGamesDone.load(std::memory_order_relaxed);

    if (done - lastMerged >= options.mergeEveryGames || done >= options.numGames) {
      mergeNewData();
      lastMerged = done;
    }

    if (done - lastReported >= options.progressEvery || done >= options.numGames) {
      lastReported = done;
      const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                                std::chrono::steady_clock::now() - startTime)
                                .count();
      const double gamesPerSec =
          elapsed > 0 ? static_cast<double>(done) / elapsed : 0.0;
      std::cout << "[datagen] games=" << done << "/" << options.numGames
                << " positions=" << totalPositionsDone.load(std::memory_order_relaxed)
                << " elapsed=" << elapsed << "s"
                << " games/s=" << gamesPerSec
                << " threads=" << numThreads << std::endl;
    }

    if (done >= options.numGames) break;
  }

  for (auto &t : threads) t.join();
  mergeNewData();  // catch anything written between the last tick and thread exit

  for (auto &ctx : workers) {
    std::remove(ctx->tempPath.c_str());
  }

  std::cout << "[datagen] done. games=" << totalGamesDone.load()
            << " positions=" << totalPositionsDone.load() << " -> "
            << options.outputPath << std::endl;
}

}
// Namespace