#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <sstream>

#include "engine.hpp"

// Logging functionality
void logMessage(const std::string &message);
void toggleLogging();
bool isLoggingEnabled();

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

  Engine *engine;
  std::thread searchThread;
  std::atomic<bool> stopRequested;
};