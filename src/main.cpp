#include "engine.hpp"

int main() {
  Engine engine;

  engine.initilizeEngine();

  engine.uci_loop();

  return 0;
}