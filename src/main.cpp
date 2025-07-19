#include "uci.hpp"
#include <fstream>

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