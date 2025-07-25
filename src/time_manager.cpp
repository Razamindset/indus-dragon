#include "time_manager.hpp"

CalculatedTime TimeManager::calculateSearchTime(chess::Board &board) {
  CalculatedTime values;

  // Infinte Case
  if ((wtime <= 0 && btime <= 0 && movetime <= 0) || is_infinte) {
    values.soft_time = INFINITE_TIME;
    values.hard_time = INFINITE_TIME;
    return values;
  }

  // Given movetime
  if (movetime > 0) {
    values.soft_time = movetime;
    values.hard_time = movetime;
    return values;
  }

  // Now lets calculate for normal commands
  long long remaining_time = board.sideToMove() == chess::Color::WHITE ? wtime : btime;
  long long increment = board.sideToMove() == chess::Color::WHITE ? winc : binc;
  int moves_remaining = movestogo > 0 ? movestogo : estimateMovesToGo(board);

  // Safety check - don't search if we have very little time
  if (remaining_time < SAFETY_BUFFER) {
    values.soft_time = MIN_SEARCH_TIME;
    values.hard_time = MIN_SEARCH_TIME;
    return values;
  }

  long long base_time = (remaining_time - SAFETY_BUFFER) / moves_remaining;

  // We get after the move
  base_time += increment * 0.8;

  // Positional adjustments
  double position_factor = getPositionFactor(board);

  // Apply evaluation-based adjustments
  double eval_factor = getEvaluationFactor(board);

  // Apply the calculted factors
  long long soft_time =
      static_cast<long long>(base_time * SOFT_TIME_FACTOR * position_factor * eval_factor);

  long long hard_time = static_cast<long long>(soft_time * HARD_TIME_FACTOR);

  // Panic Mode - If very low on time reduce time significantly
  if (remaining_time < (wtime + btime) * PANIC_THRESHOLD) {
    soft_time = std::min(soft_time, remaining_time / 4);
    hard_time = std::min(hard_time, remaining_time / 2);
  }

  // Ensure minimum search time
  soft_time = std::max(soft_time, MIN_SEARCH_TIME);
  hard_time = std::max(hard_time, MIN_SEARCH_TIME);

  // Don't exceed remaining time minus safety buffer
  long long max_time = remaining_time - SAFETY_BUFFER;
  soft_time = std::min(soft_time, max_time);
  hard_time = std::min(hard_time, max_time);

  values.soft_time = soft_time;
  values.hard_time = hard_time;
  return values;
}

// Estimate moves remaining based on game phase
int TimeManager::estimateMovesToGo(const chess::Board &board) {
  int piece_count = count_pieces(board);

  int full_moves = board.fullMoveNumber();

  // Opening (many pieces): expect ~35 more moves
  if (piece_count >= 24) {
    return std::max(35 - full_moves / 2, 25);
  }
  // Middlegame (medium pieces): expect ~25 more moves
  else if (piece_count >= 12) {
    return std::max(25 - full_moves / 3, 15);
  }
  // Endgame (few pieces): expect ~15 more moves
  else {
    return std::max(15 - full_moves / 4, 8);
  }
}

// Adjust time based on position characteristics
double TimeManager::getPositionFactor(const chess::Board &board) {
  double factor = 1.0;

  // Spend more time in complex positions
  int mobility = 0;
  chess::Movelist moves;
  chess::movegen::legalmoves(moves, board);
  mobility = moves.size();

  // More legal moves = more complex position = more time
  if (mobility > 35) {
    factor *= 1.3; // Complex position
  } else if (mobility < 15) {
    factor *= 0.8; // Simple position
  }

  // Spend more time in endgame (fewer pieces, critical decisions)
  int total_pieces = count_pieces(board);
  if (total_pieces <= 12) {
    factor *= ENDGAME_FACTOR; // Endgame precision is crucial
  }

  // Spend more time when in check (tactical situation)
  if (board.inCheck()) {
    factor *= IN_CHECK_FACTOR;
  }

  return std::max(0.5, std::min(2.0, factor)); // Clamp between 0.5x and 2.0x
}

// Adjust time based on evaluation score
double TimeManager::getEvaluationFactor(const chess::Board &board) {
  Evaluation evaluator;
  int eval_score = evaluator.evaluate(board);

  // Convert to our side's perspective
  if (board.sideToMove() == chess::Color::BLACK) {
    eval_score = -eval_score;
  }

  double factor = 1.0;

  // Critical positions (close to equal) - spend more time
  if (abs(eval_score) < 50) {         // Within 0.5 pawns
    factor *= 1.4;                    // Spend 40% more time in critical positions
  } else if (abs(eval_score) < 100) { // Within 1 pawn
    factor *= 1.2;                    // Spend 20% more time
  }

  // Clearly winning positions - spend less time, play faster
  else if (eval_score > 300) {   // Winning by 3+ pawns
    factor *= 0.7;               // Spend 30% less time
  } else if (eval_score > 150) { // Winning by 1.5+ pawns
    factor *= 0.85;              // Spend 15% less time
  }

  // Clearly losing positions - spend more time to find tactics/tricks
  else if (eval_score < -300) {   // Losing by 3+ pawns
    factor *= 1.3;                // Spend more time looking for tactical shots
  } else if (eval_score < -150) { // Losing by 1.5+ pawns
    factor *= 1.1;                // Spend slightly more time
  }

  return std::max(0.6, std::min(1.8, factor)); // Clamp between 0.6x and 1.8x
}

void TimeManager::setTimevalues(int wtime, int btime, int winc, int binc, int movestogo,
                                int movetime, bool is_infinite) {
  this->wtime = static_cast<long long>(wtime);
  this->btime = static_cast<long long>(btime);
  this->winc = static_cast<long long>(winc);
  this->binc = static_cast<long long>(binc);
  this->movestogo = static_cast<long long>(movestogo);
  this->movetime = static_cast<long long>(movetime);
}
