#pragma once

#include <atomic>
#include <string>
#include <thread>

#include "engine.hpp"

class UCIAdapter {
public:
  UCIAdapter(Engine *e);
  void start();

private:
  void processCommand(const std::string &cmd);
  void handleUCI();
  void handlePosition(std::istringstream &iss);
  void handleGo(std::istringstream &iss);
  void handleStop();
  void handleSetOption(std::istringstream &iss);
  void logCommand(const std::string &cmd);
  void logOutput(const std::string &output);
  void logError(const std::string &error);

  Engine *engine;
  std::thread searchThread;
  std::atomic<bool> stopRequested;
  bool storeLogs = false;
};
