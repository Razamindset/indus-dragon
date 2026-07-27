#include "engine.hpp"

Engine::Engine() : board(), tt_helper(), search(board, tt_helper) {}

void Engine::printBoard() { std::cout << board << "\n" << board.getFen(); }

void Engine::setPosition(const std::string &fen) { board.setFen(fen); }

void Engine::initializeEngine() {
  board = chess::Board();
  tt_helper.clear_table();
}

void Engine::makeMove(std::string move) {
  chess::Move parsedMove = chess::uci::uciToMove(board, move);
  board.makeMove(parsedMove);
}

GoOptions parseGoOptions(std::istringstream& iss) {
  GoOptions options;
  std::string token;

  while (iss >> token) {
    if (token == "infinite") {
      options.infinite = true;
    } else if (token == "wtime") {
      iss >> options.wtime;
    } else if (token == "btime") {
      iss >> options.btime;
    } else if (token == "winc") {
      iss >> options.winc;
    } else if (token == "binc") {
      iss >> options.binc;
    } else if (token == "movestogo") {
      iss >> options.movestogo;
    } else if (token == "movetime") {
      iss >> options.movetime;
    }
  }

  return options;
}

void Engine::handleBench() {
  const int benchDepth = 8; // fixed depth, not time-limited
  long long totalNodes = 0;
  auto start = std::chrono::steady_clock::now();

  for (const auto& fen : BENCH_POSITIONS) {
    board.setFen(fen);
    search.setTimeValues(GoOptions{}); // infinite/no time limit — depth-limited only
    // (needs a depth-limited entry point into Search — see note below)
    totalNodes += search.benchSearch(benchDepth);
  }

  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start).count();
  long long nps = elapsed > 0 ? (totalNodes * 1000) / elapsed : 0;

  std::cout << totalNodes << " nodes " << nps << " nps\n";
}

void Engine::handleGo(std::istringstream& iss) {
  GoOptions options = parseGoOptions(iss);
  search.setTimeValues(options);
  search.searchBestMove();
}


void Engine::handleFen(std::istringstream &iss) {
  std::string token;
  iss >> token;

  if (token == "startpos") {
    initializeEngine();

    // Process any moves that come after "startpos moves"
    if (iss >> token && token == "moves") {
      std::string move;
      while (iss >> move) {
        makeMove(move);
      }
    }

  } else if (token == "fen") {
    std::string fen;
    std::string fenPart;

    // FIXED: Properly collect FEN string parts
    // FEN has 6 parts: board, side, castling, en passant, halfmove,
    // fullmove
    for (int i = 0; i < 6 && iss >> fenPart; i++) {
      if (!fen.empty()) fen += " ";
      fen += fenPart;
    }

    setPosition(fen);

    // Process any moves after the FEN
    if (iss >> token && token == "moves") {
      std::string move;
      while (iss >> move) {
        makeMove(move);
      }
    }
  }
}

void Engine::uciLoop() {
  /*
  This loop will be in action while there is no search ongoing.
  When we are given a go command we call the search on the same thread.
  Search itself is responsible for handling the rest of commands that might
  occur during search like stop, quit.
  Therefore in ideal conditions stopsearch will never need to be called from
  here.
  */
 std::cout << "Extended Commands for debugging\n";
 std::cout << "'d' - print the current board\n";
 std::cout << "'togglelogs' - Write the engine logs to a log file for debug\n";
 std::cout << "'ttstats' - Print TTHits and Stores\n";
 std::cout << "'bench' - run fixed-depth benchmark for regression testing\n";

  std::string cmd;

  while (std::getline(std::cin, cmd)) {
    search.logMessage(cmd);
    std::istringstream iss(cmd);
    std::string token;
    iss >> token;

    if (token == "uci") {
      std::string idName = "id name Indus Dragon";
      std::string idAuthor = "id author Razamindset";
      std::string uciOk = "uciok";

      std::cout << idName << std::endl;
      std::cout << idAuthor << std::endl;

      std::cout << uciOk << std::endl;
      fflush(stdout);

    } else if (token == "isready") {
      std::string readyOk = "readyok";
      std::cout << readyOk << std::endl;
      fflush(stdout);

    } else if (token == "position") {
      handleFen(iss);
    } else if (token == "d") {
      printBoard();
    } else if (token == "quit") {
      exit(0);
    } else if (token == "ucinewgame") {
      initializeEngine();
    } else if (token == "bench") {
      handleBench();
    } else if (token == "togglelogs") {
      search.toggleLogs();
    } else if (token == "ttstats") {
      tt_helper.printTTStats();
    } else if (token == "go") {
      handleGo(iss);
    }
  }
}
