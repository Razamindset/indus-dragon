#include "engine.hpp"

int main() {
  Engine engine;

  engine.initializeEngine();

  engine.uciLoop();

  return 0;
}
