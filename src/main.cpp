#include <atomic>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "./engine/engine.hpp"

//! Claude generated slappy code

class UCIAdapter {
 private:
  // Pointer to your engine
  Engine* engine;

  std::thread searchThread;
  std::atomic<bool> stopRequested;

 public:
  UCIAdapter(Engine* e) : engine(e), stopRequested(false) {}

  void start() {
    std::string line;
    while (std::getline(std::cin, line)) {
      processCommand(line);
    }
  }

 private:
  void processCommand(const std::string& cmd) {
    std::istringstream iss(cmd);
    std::string token;
    iss >> token;

    if (token == "uci") {
      handleUCI();
    } else if (token == "isready") {
      std::cout << "readyok" << std::endl;
    } else if (token == "position") {
      handlePosition(iss);
    } else if (token == "d") {
      engine->printBoard();
    } else if (token == "go") {
      handleGo();
    } else if (token == "stop") {
      handleStop();
    } else if (token == "quit") {
      exit(0);
    } else if (token == "ucinewgame") {
      engine->initilizeEngine();
    } else if (token == "setoption") {
      handleSetOption(iss);
    }
  }

  void handleUCI() {
    std::cout << "id name Pawnstar" << std::endl;
    std::cout << "id author Razamindset" << std::endl;

    // Output available options if any
    // std::cout << "option name Hash type spin default 64 min 1 max 1024" <<
    // std::endl;

    std::cout << "uciok" << std::endl;
  }

  void handlePosition(std::istringstream& iss) {
    std::string token;
    iss >> token;

    if (token == "startpos") {
      engine->setPosition(
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");  // Standard
                                                                        // starting
                                                                        // FEN

      // Process any moves that come after "startpos moves"
      iss >> token;  // Skip "moves" token if present
      if (token == "moves") {
        std::string move;
        while (iss >> move) {
          engine->makeMove(move);
        }
      }
    } else if (token == "fen") {
      std::string fen;
      // Collect all parts of the FEN string
      for (int i = 0; i < 6; i++) {
        iss >> token;
        fen += token + " ";
      }
      engine->setPosition(fen);

      // Process any moves after the FEN
      iss >> token;  // Check for "moves" token
      if (token == "moves") {
        std::string move;
        while (iss >> move) {
          engine->makeMove(move);
        }
      }
    }
  }

  void handleGo() {
    // Simply get the best move without time controls
    std::string bestMove = engine->getBestMove(4);
    std::cout << "bestmove " << bestMove << std::endl;
  }

  void handleStop() {
    // Since we're not using time controls, this might not be needed
    // But keep it for compatibility with UCI protocol
    std::string bestMove = engine->getBestMove(4);
    std::cout << "bestmove " << bestMove << std::endl;
  }

  void handleSetOption(std::istringstream& iss) {
    std::string token, name, value;
    iss >> token;  // "name"

    if (token != "name") return;

    // Get option name (might contain spaces)
    std::string nameStr;
    while (iss >> token && token != "value") {
      if (!nameStr.empty()) nameStr += " ";
      nameStr += token;
    }

    // Get option value
    std::string valueStr;
    while (iss >> token) {
      if (!valueStr.empty()) valueStr += " ";
      valueStr += token;
    }

    // Set the option in your engine
    // engine->setOption(nameStr, valueStr);
  }
};

// Usage example
int main() {
  Engine engine;
  UCIAdapter adapter(&engine);
  adapter.start();
  return 0;
}