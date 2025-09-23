#include "engine.hpp"

int main() {
  Engine engine;

  engine.initilizeEngine();

  engine.uciLoop();

  return 0;
}