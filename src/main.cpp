#include <cstdlib>
#include <iostream>
#include <string>

#include "datagen.hpp"
#include "engine.hpp"

// Usage: indus-dragon datagen <output_file> [num_games=1000] [depth=7] [seed=0] [threads=1]
static int runDatagen(int argc, char **argv) {
  Datagen::DatagenOptions opts;

  if (argc > 2) opts.outputPath = argv[2];
  if (argc > 3) opts.numGames = std::atoll(argv[3]);
  if (argc > 4) opts.searchDepth = std::atoi(argv[4]);
  if (argc > 5) opts.seed = static_cast<uint64_t>(std::atoll(argv[5]));
  if (argc > 6) opts.numThreads = static_cast<unsigned int>(std::atoi(argv[6]));
  if (argc > 7) opts.appendOutput = (std::atoi(argv[7]) != 0);

  std::cout << "[datagen] output=" << opts.outputPath
            << " games=" << opts.numGames
            << " depth=" << opts.searchDepth
            << " seed=" << opts.seed
            << " threads=" << opts.numThreads << std::endl;

  Datagen::run(opts);
  return 0;
}

int main(int argc, char **argv) {
  if (argc > 1 && std::string(argv[1]) == "datagen") {
    return runDatagen(argc, argv);
  }

  Engine engine;

  engine.initializeEngine();

  engine.uciLoop();

  return 0;
}
