#include "uci.hpp"

#include <atomic>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace {
  std::atomic<bool> storeLogs(false);
}

void logMessage(const std::string &message) {
  if (!storeLogs)
    return;
  try {
    std::ofstream logFile("uci_log.txt", std::ios_base::app);
    if (logFile.is_open()) {
      logFile << message << std::endl;
      logFile.close();
    }
  } catch (...) {
    // Silently ignore logging errors to prevent crashes
  }
}

void toggleLogging() {
  storeLogs = !storeLogs;
  logMessage(storeLogs ? "Started storing logs" : "Stopped storing logs");
}

bool isLoggingEnabled() { return storeLogs.load(); }

UCIAdapter::UCIAdapter(Engine *e) : engine(e), stopRequested(false) {}

void UCIAdapter::start() {
  logMessage("The adapter for the engine was started.");
  std::string line;
  while (std::getline(std::cin, line)) {
    try {
      processCommand(line);
    } catch (const std::exception &e) {
      // handle this
    }
  }
}

void UCIAdapter::processCommand(const std::string &cmd) {
  // Log the command
  logMessage(cmd);

  std::istringstream iss(cmd);
  std::string token;
  iss >> token;

  if (token == "uci") {
    handleUCI();
  } else if (token == "isready") {
    std::string readyOk = "readyok";
    std::cout << readyOk << std::endl;
    fflush(stdout);
    logMessage(readyOk);
  } else if (token == "position") {
    handlePosition(iss);
  } else if (token == "d") {
    if (engine) {
      engine->printBoard();
    }
  } else if (token == "go") {
    handleGo(iss);
  } else if (token == "stop") {
    handleStop();
  } else if (token == "quit") {
    handleStop(); // Ensure search thread is stopped before exit
    exit(0);
  } else if (token == "togglelogs") {
    toggleLogging();
  } else if (token == "ucinewgame") {
    handleStop(); // Stop any ongoing search before reinitializing
    if (engine) {
      engine->initilizeEngine();
    }
  } else if (token == "setoption") {
    handleSetOption(iss);
  }
}

void UCIAdapter::handleUCI() {
  std::string idName = "id name Indus Dragon";
  std::string idAuthor = "id author Razamindset";
  std::string uciOk = "uciok";

  std::cout << idName << std::endl;
  logMessage(idName);
  std::cout << idAuthor << std::endl;
  logMessage(idAuthor);

  // Output available options if any
  // std::cout << "option name Hash type spin default 64 min 1 max 1024" <<
  // std::endl;

  std::cout << uciOk << std::endl;
  fflush(stdout);
  logMessage(uciOk);
}

void UCIAdapter::handlePosition(std::istringstream &iss) {
  if (!engine) {
    logMessage("Engine is null in handlePosition");
    return;
  }

  std::string token;
  iss >> token;

  if (token == "startpos") {
    try {
      engine->setPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

      // Process any moves that come after "startpos moves"
      if (iss >> token && token == "moves") {
        std::string move;
        while (iss >> move) {
          // logMessage("Making move: " + move);
          try {
            engine->makeMove(move);
            // logMessage("Move " + move + " applied successfully");
          } catch (const std::exception &e) {
            logMessage("Exception making move: " + move + " - " + std::string(e.what()));
            break;
          } catch (...) {
            logMessage("Unknown exception making move: " + move);
            break;
          }
        }
      }
    } catch (const std::exception &e) {
      logMessage("Exception in startpos handling: " + std::string(e.what()));
    }
  } else if (token == "fen") {
    try {
      std::string fen;
      std::string fenPart;

      // FIXED: Properly collect FEN string parts
      // FEN has 6 parts: board, side, castling, en passant, halfmove,
      // fullmove
      for (int i = 0; i < 6 && iss >> fenPart; i++) {
        if (!fen.empty())
          fen += " ";
        fen += fenPart;
      }

      if (fen.empty()) {
        logMessage("Empty FEN string received");
        return;
      }

      logMessage("Setting FEN: " + fen);
      engine->setPosition(fen);

      // Process any moves after the FEN
      if (iss >> token && token == "moves") {
        std::string move;
        while (iss >> move) {
          logMessage("Making move after FEN: " + move);
          try {
            engine->makeMove(move);
            logMessage("Move " + move + " applied successfully after FEN");
          } catch (const std::exception &e) {
            logMessage("Exception making move after FEN: " + move + " - " + std::string(e.what()));
            break;
          } catch (...) {
            logMessage("Unknown exception making move after FEN: " + move);
            break;
          }
        }
      }
    } catch (const std::exception &e) {
      logMessage("Exception in FEN handling: " + std::string(e.what()));
    }
  } else {
    logMessage("Unknown position command: " + token);
  }
}

void UCIAdapter::handleGo(std::istringstream &iss) {
  if (!engine) {
    logMessage("Engine is null in handleGo");
    return;
  }

  // Stop any previous search properly
  handleStop();

  int wtime = 0, btime = 0, winc = 0, binc = 0, movestogo = 0, movetime = 0;
  std::string token;
  while (iss >> token) {
    if (token == "wtime") {
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

  engine->setSearchLimits(wtime, btime, winc, binc, movestogo, movetime);

  stopRequested = false; // Reset the stop flag

  // Start a new search in a separate thread
  searchThread = std::thread([this]() {
    try {
      engine->getBestMove();

      // FIXED: Use atomic check to prevent race condition
      if (!stopRequested.load()) {
        logMessage("Search was stopped before completion");
      }
    } catch (const std::exception &e) {
      logMessage("Exception in search thread: " + std::string(e.what()));
      if (!stopRequested.load()) {
        std::cout << "bestmove (none)" << std::endl;
      }
    }
  });
}

void UCIAdapter::handleStop() {
  if (searchThread.joinable()) {
    stopRequested = true; // Signal stop first
    if (engine) {
      engine->stopSearch();
    }
    searchThread.join();
    logMessage("Search stopped");
  }
}

void UCIAdapter::handleSetOption(std::istringstream &iss) {
  std::string token;
  iss >> token; // "name"

  if (token != "name") {
    logMessage("Invalid setoption command format");
    return;
  }

  // Get option name (might contain spaces)
  std::string nameStr;
  while (iss >> token && token != "value") {
    if (!nameStr.empty())
      nameStr += " ";
    nameStr += token;
  }

  // Get option value
  std::string valueStr;
  while (iss >> token) {
    if (!valueStr.empty())
      valueStr += " ";
    valueStr += token;
  }

  logMessage("Setting option: " + nameStr + " = " + valueStr);

  // Set the option in your engine
  // if (engine) {
  //     engine->setOption(nameStr, valueStr);
  // }
}