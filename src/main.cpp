#include <atomic>
#include <fstream> // FIXED: Added missing header for file operations
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "./engine/engine.hpp"

class UCIAdapter {
private:
  // Pointer to your engine
  Engine *engine;

  std::thread searchThread;
  std::atomic<bool> stopRequested;
  bool storeLogs = false;

public:
  UCIAdapter(Engine *e) : engine(e), stopRequested(false) {}

  void start() {
    logCommand("The adapter for the engine was started.");
    std::string line;
    while (std::getline(std::cin, line)) {
      try {
        processCommand(line);
      } catch (const std::exception &e) {
        // Log errors to help debug crashes
        logError("Exception in processCommand: " + std::string(e.what()));
      }
    }
  }

private:
  void logCommand(const std::string &cmd) {
    if (!storeLogs)
      return;
    try {
      std::ofstream logFile("uci_log.txt", std::ios_base::app);
      if (logFile.is_open()) {
        logFile << "[CMD] " << cmd << std::endl;
        logFile.close();
      }
    } catch (...) {
      // Silently ignore logging errors to prevent crashes
    }
  }

  void logOutput(const std::string &output) {
    if (!storeLogs)
      return;
    try {
      std::ofstream logFile("uci_log.txt", std::ios_base::app);
      if (logFile.is_open()) {
        logFile << "[OUTPUT] " << output << std::endl;
        logFile.close();
      }
    } catch (...) {
      // Silently ignore logging errors to prevent crashes
    }
  }

  void logError(const std::string &error) {
    if (!storeLogs)
      return;
    try {
      std::ofstream logFile("uci_log.txt", std::ios_base::app);
      if (logFile.is_open()) {
        logFile << "[ERROR] " << error << std::endl;
        logFile.close();
      }
    } catch (...) {
      // Silently ignore logging errors
    }
  }

  void processCommand(const std::string &cmd) {
    // Log the command
    logCommand(cmd);

    std::istringstream iss(cmd);
    std::string token;
    iss >> token;

    if (token == "uci") {
      handleUCI();
    } else if (token == "isready") {
      std::string readyOk = "readyok";
      std::cout << readyOk << std::endl;
      logOutput(readyOk);
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
      storeLogs = !storeLogs;
      logCommand(storeLogs ? "Started storing logs" : "Stopped storing logs");
    } else if (token == "ucinewgame") {
      handleStop(); // Stop any ongoing search before reinitializing
      if (engine) {
        engine->initilizeEngine();
      }
    } else if (token == "setoption") {
      handleSetOption(iss);
    }
  }

  void handleUCI() {
    std::string idName = "id name Indus Dragon";
    std::string idAuthor = "id author Razamindset";
    std::string uciOk = "uciok";

    std::cout << idName << std::endl;
    logOutput(idName);
    std::cout << idAuthor << std::endl;
    logOutput(idAuthor);

    // Output available options if any
    // std::cout << "option name Hash type spin default 64 min 1 max 1024" <<
    // std::endl;

    std::cout << uciOk << std::endl;
    logOutput(uciOk);
  }

  void handlePosition(std::istringstream &iss) {
    if (!engine) {
      logError("Engine is null in handlePosition");
      return;
    }

    std::string token;
    iss >> token;

    if (token == "startpos") {
      try {
        engine->setPosition(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

        // Process any moves that come after "startpos moves"
        if (iss >> token && token == "moves") {
          std::string move;
          while (iss >> move) {
            // logCommand("Making move: " + move);
            try {
              engine->makeMove(move);
              // logCommand("Move " + move + " applied successfully");
            } catch (const std::exception &e) {
              logError("Exception making move: " + move + " - " +
                       std::string(e.what()));
              break;
            } catch (...) {
              logError("Unknown exception making move: " + move);
              break;
            }
          }
        }
      } catch (const std::exception &e) {
        logError("Exception in startpos handling: " + std::string(e.what()));
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
          logError("Empty FEN string received");
          return;
        }

        logCommand("Setting FEN: " + fen);
        engine->setPosition(fen);

        // Process any moves after the FEN
        if (iss >> token && token == "moves") {
          std::string move;
          while (iss >> move) {
            logCommand("Making move after FEN: " + move);
            try {
              engine->makeMove(move);
              logCommand("Move " + move + " applied successfully after FEN");
            } catch (const std::exception &e) {
              logError("Exception making move after FEN: " + move + " - " +
                       std::string(e.what()));
              break;
            } catch (...) {
              logError("Unknown exception making move after FEN: " + move);
              break;
            }
          }
        }
      } catch (const std::exception &e) {
        logError("Exception in FEN handling: " + std::string(e.what()));
      }
    } else {
      logError("Unknown position command: " + token);
    }
  }

  void handleGo(std::istringstream &iss) {
    if (!engine) {
      logError("Engine is null in handleGo");
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
    logCommand("Starting search");

    // Start a new search in a separate thread
    searchThread = std::thread([this]() {
      try {
        std::string bestMove = engine->getBestMove();

        // FIXED: Use atomic check to prevent race condition
        if (!stopRequested.load()) {
          std::string bestMoveMsg = "bestmove " + bestMove;
          std::cout << bestMoveMsg << std::endl;
          logOutput(bestMoveMsg);
          logCommand("Best move found: " + bestMove);
        } else {
          logCommand("Search was stopped before completion");
        }
      } catch (const std::exception &e) {
        logError("Exception in search thread: " + std::string(e.what()));
        if (!stopRequested.load()) {
          std::cout << "bestmove (none)" << std::endl;
        }
      }
    });
  }

  void handleStop() {
    if (searchThread.joinable()) {
      stopRequested = true; // Signal stop first
      if (engine) {
        engine->stopSearch();
      }
      searchThread.join();
      logCommand("Search stopped");
    }
  }

  void handleSetOption(std::istringstream &iss) {
    std::string token;
    iss >> token; // "name"

    if (token != "name") {
      logError("Invalid setoption command format");
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

    logCommand("Setting option: " + nameStr + " = " + valueStr);

    // Set the option in your engine
    // if (engine) {
    //     engine->setOption(nameStr, valueStr);
    // }
  }
};

// Usage example
int main() {
  try {
    Engine engine;
    UCIAdapter adapter(&engine);
    adapter.start();
  } catch (const std::exception &e) {
    std::ofstream logFile("uci_log.txt", std::ios_base::app);
    if (logFile.is_open()) {
      logFile << "[FATAL] Exception in main: " << e.what() << std::endl;
    }
    return 1;
  }
  return 0;
}