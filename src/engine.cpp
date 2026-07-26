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
    } else if (token == "togglelogs") {
      search.toggleLogs();
    } else if (token == "ttstats") {
      tt_helper.printTTStats();
    } else if (token == "go") {
      handleGo(iss);
    }
  }
}
