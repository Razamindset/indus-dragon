#pragma once

struct GoOptions {
  long long wtime = 0;
  long long btime = 0;
  long long winc = 0;
  long long binc = 0;
  long long movestogo = 0;
  long long movetime = 0;
  bool infinite = false;

  bool hasTimeLimit() const {
    return !infinite && (wtime > 0 || btime > 0 || movetime > 0);
  }
};

struct SearchTime {
  long long soft = 0;
  long long hard = 0;
};