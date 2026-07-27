#pragma once

struct GoOptions {
  long long wtime = 0;
  long long btime = 0;
  long long winc = 0;
  long long binc = 0;
  long long movestogo = 0;
  long long movetime = 0;
  bool infinite = false;
  int depth = 0;

  bool hasTimeLimit() const {
    return !infinite && (wtime > 0 || btime > 0 || movetime > 0);
  }
};

struct SearchTime {
  long long soft = 0;
  long long hard = 0;
};


static const std::vector<std::string> BENCH_POSITIONS = {
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",          // startpos
  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", // Kiwipete (tactical)
  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",                          // endgame
  "r1bq1rk1/pp2bppp/2n2n2/3p4/3P4/2N1BN2/PP2BPPP/R2Q1RK1 w - - 0 1",     // quiet middlegame
  // add 4-6 more you care about, e.g. one from a game where the engine looked weak
};
