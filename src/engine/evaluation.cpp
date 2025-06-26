#include "engine.hpp"
#include "piece-maps.hpp"

// Evaluation related functions
bool Engine::hasCastled(Color color) {
  Square kingSq = board.pieces(PieceType::KING, color).lsb();

  if (color == Color::WHITE)
    return kingSq.index() == 6 || kingSq.index() == 2;  // G1 or C1
  else
    return kingSq.index() == 62 || kingSq.index() == 58;  // G8 or C8
}

int Engine::evaluate(int ply) {
  if (isGameOver()) {
    if (getGameOverReason() == GameResultReason::CHECKMATE) {
      if (board.sideToMove() == Color::WHITE) {
        return -MATE_SCORE + ply;
      } else {
        return MATE_SCORE - ply;
      }
    }
    return DRAW_SCORE;
  }

  int eval = 0;

  // Initialize all bitboards once
  auto whitePawns = board.pieces(PieceType::PAWN, Color::WHITE);
  auto blackPawns = board.pieces(PieceType::PAWN, Color::BLACK);
  auto whiteKnights = board.pieces(PieceType::KNIGHT, Color::WHITE);
  auto blackKnights = board.pieces(PieceType::KNIGHT, Color::BLACK);
  auto whiteBishops = board.pieces(PieceType::BISHOP, Color::WHITE);
  auto blackBishops = board.pieces(PieceType::BISHOP, Color::BLACK);
  auto whiteRooks = board.pieces(PieceType::ROOK, Color::WHITE);
  auto blackRooks = board.pieces(PieceType::ROOK, Color::BLACK);
  auto whiteQueens = board.pieces(PieceType::QUEEN, Color::WHITE);
  auto blackQueens = board.pieces(PieceType::QUEEN, Color::BLACK);

  // Count material using the initialized bitboards
  int whiteMaterial = whitePawns.count() * PAWN_VALUE +
                      whiteKnights.count() * KNIGHT_VALUE +
                      whiteBishops.count() * BISHOP_VALUE +
                      whiteRooks.count() * ROOK_VALUE +
                      whiteQueens.count() * QUEEN_VALUE;

  int blackMaterial = blackPawns.count() * PAWN_VALUE +
                      blackKnights.count() * KNIGHT_VALUE +
                      blackBishops.count() * BISHOP_VALUE +
                      blackRooks.count() * ROOK_VALUE +
                      blackQueens.count() * QUEEN_VALUE;

  eval += whiteMaterial - blackMaterial;

  // Better endgame detection based on total material
  int totalMaterial = whiteMaterial + blackMaterial;
  bool isEndgame = totalMaterial < 2500; // Endgame when less than ~25 points of material total
  // bool isOpening = totalMaterial > 6000; // Opening when more than ~60 points of material total


  // Castling bonus (opening/middlegame only)
  if (!isEndgame) {
    if (hasCastled(Color::WHITE)) eval += 50;
    if (hasCastled(Color::BLACK)) eval -= 50;
  }

  evaluatePST(eval, isEndgame);

  evaluatePawns(eval, whitePawns, blackPawns);

  if(isEndgame){
    evaluateKingEndgameScore(eval);
  }

  return eval;
}

// Add values from piece square tables
void Engine::evaluatePST(int &eval, bool isEndgame){
  for (Square sq = 0; sq < 64; sq++) {
    Piece piece = board.at(sq);
    if (piece.type() == PieceType::NONE) continue;

    int index = (piece.color() == Color::WHITE) ? mirrorIndex(sq.index()) : sq.index();
    int squareValue = 0;

    switch (piece.type()) {
      case PAWN:
        squareValue = PAWN_TABLE[index];
        break;
      case KNIGHT:
        squareValue = KNIGHT_TABLE[index];
        break;
      case BISHOP:
        squareValue = BISHOP_TABLE[index];
        break;
      case ROOK:
        squareValue = ROOK_TABLE[index];
        break;
      case QUEEN:
        squareValue = QUEEN_TABLE[index];
        break;
      case KING:
        squareValue = isEndgame ? KING_END_TABLE[index] : KING_MIDDLE_TABLE[index];
        break;
    }

    eval += (piece.color() == Color::WHITE) ? squareValue : -squareValue;
  }
}

// Calculate the pawns evaluation
void Engine::evaluatePawns(int& eval, const chess::Bitboard& whitePawns, const chess::Bitboard& blackPawns){
  // 1. Doubled pawns penalty
  for (int file = 0; file < 8; file++) {
    int whitePawnsOnFile = 0;
    int blackPawnsOnFile = 0;
    
    // Count pawns on each file
    chess::Bitboard fileMask = chess::Bitboard(0x0101010101010101ULL << file);
    whitePawnsOnFile = (whitePawns & fileMask).count();
    blackPawnsOnFile = (blackPawns & fileMask).count();

    
    
    // Penalty for doubled pawns
    if (whitePawnsOnFile > 1) {
      eval -= 20 * (whitePawnsOnFile - 1);
    }
    if (blackPawnsOnFile > 1) {
      eval += 20 * (blackPawnsOnFile - 1);
    }
  }

  // 2. Isolated pawns penalty
  for (int file = 0; file < 8; file++) {
    // Check if there are pawns on this file
    chess::Bitboard fileMask = chess::Bitboard(0x0101010101010101ULL << file);
    
    if ((whitePawns & fileMask).count() > 0) {
      // Check adjacent files for white pawns
      bool hasSupport = false;
      if (file > 0) {
        chess::Bitboard leftFile = chess::Bitboard(0x0101010101010101ULL << (file - 1));
        if ((whitePawns & leftFile).count() > 0) hasSupport = true;
      }
      if (file < 7) {
        chess::Bitboard rightFile = chess::Bitboard(0x0101010101010101ULL << (file + 1));
        if ((whitePawns & rightFile).count() > 0) hasSupport = true;
      }
      
      if (!hasSupport) {
        eval -= 15 * (whitePawns & fileMask).count(); // Penalty for each isolated pawn
      }
    }
    
    if ((blackPawns & fileMask).count() > 0) {
      // Check adjacent files for black pawns
      bool hasSupport = false;
      if (file > 0) {
        chess::Bitboard leftFile = chess::Bitboard(0x0101010101010101ULL << (file - 1));
        if ((blackPawns & leftFile).count() > 0) hasSupport = true;
      }
      if (file < 7) {
        chess::Bitboard rightFile = chess::Bitboard(0x0101010101010101ULL << (file + 1));
        if ((blackPawns & rightFile).count() > 0) hasSupport = true;
      }
      
      if (!hasSupport) {
        eval += 15 * (blackPawns & fileMask).count(); // Penalty for each isolated pawn
      }
    }
  }

  // Check if any passed pawns and award accordingly
}

// Calculate King endgame score
void Engine::evaluateKingEndgameScore(int& eval){
  Square whiteKingSq = board.kingSq(Color::WHITE);
  Square blackKingSq = board.kingSq(Color::BLACK);

  int distance = manhattanDistance(whiteKingSq, blackKingSq);
  eval += (14 - distance) * 6;

  int whiteKingFile = whiteKingSq.file();
  int whiteKingRank = whiteKingSq.rank();

  int blackKingFile = blackKingSq.file();
  int blackKingRank = blackKingSq.rank();

  // Higher bonus for corner squares
  if ((blackKingFile == 0 || blackKingFile == 7) && (blackKingRank == 0 || blackKingRank == 7)) {
    eval += 100;
  }
  if ((whiteKingFile == 0 || whiteKingFile == 7) && (whiteKingRank == 0 || whiteKingRank == 7)) {
    eval -= 100;
  }

  // Bonus for being on the edges
  if (blackKingFile == 0 || blackKingFile == 7 || blackKingRank == 0 || blackKingRank == 7) {
    eval += 100;
  }
  if (whiteKingFile == 0 || whiteKingFile == 7 || whiteKingRank == 0 || whiteKingRank == 7) {
    eval -= 100;
  }
}