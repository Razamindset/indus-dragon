#pragma once

struct GoOptions {
  int wtime = 0;
  int btime = 0;
  int winc = 0;
  int binc = 0;
  int movestogo = 0;
  int movetime = 0;
  bool infinite = false;

  bool hasTimeLimit() const {
    return !infinite && (wtime > 0 || btime > 0 || movetime > 0);
  }
};

struct SearchTime {
  long long soft = 0;
  long long hard = 0;
};