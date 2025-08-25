#include "engine.hpp"
#include "nnue/nnue.h"
#include <fstream>

int main() {

  nnue_init("nn-04cf2b4ed1da.nnue");
  Engine engine;
  engine.initilizeEngine();
  engine.uci_loop();

  return 0;
}