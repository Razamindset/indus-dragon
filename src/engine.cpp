#include "engine.hpp"

Engine::Engine() : board(), tt_helper(), search(board, tt_helper) {}

void Engine::printBoard() { std::cout << board; }

void Engine::setPosition(const std::string &fen) { board.setFen(fen); }

void Engine::initilizeEngine() {
  board = chess::Board();
  tt_helper.clear_table();
}

void Engine::makeMove(std::string move) {
  Move parsedMove = uci::uciToMove(board, move);
  board.makeMove(parsedMove);
}

void Engine::handle_stop() { search.stopSearch(); }

void Engine::handle_go(std::istringstream &iss) {
  handle_stop();

  int wtime = 0, btime = 0, winc = 0, binc = 0, movestogo = 0, movetime = 0;

  std::string token;

  while (iss >> token) {
    if (token == "infinite") {
      break;

    } else if (token == "wtime") {
      iss >> wtime;

    } else if (token == "btime") {
      iss >> btime;

    } else if (token == "winc") {
      iss >> winc;

    } else if (token == "binc") {
      iss >> binc;

    } else if (token == "movestogo") {
      iss >> movestogo;

    } else if (token == "movetime") {
      iss >> movetime;
    }
  }

  search.setTimevalues(wtime, btime, winc, binc, movestogo, movetime);

  search.searchBestMove();
}

void Engine::handle_positon(std::istringstream &iss) {
  std::string token;
  iss >> token;

  if (token == "startpos") {
    initilizeEngine();

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

void Engine::uci_loop() {
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
      handle_positon(iss);
    } else if (token == "d") {
      printBoard();
    } else if (token == "quit") {
      handle_stop();  // Ensure search thread is stopped before exit
      exit(0);
    } else if (token == "stop") {
      handle_stop();
    } else if (token == "ucinewgame") {
      handle_stop();  // Stop any ongoing search before reinitializing
      initilizeEngine();
    } else if (token == "togglelogs") {
      search.toggleLogs();
    } else if (token == "ttstats") {
      tt_helper.printTTStats();
    } else if (token == "go") {
      handle_go(iss);
    }
  }
}
