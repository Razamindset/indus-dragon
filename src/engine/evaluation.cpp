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
    if (hasCastled(Color::WHITE)) eval += 20;
    if (hasCastled(Color::BLACK)) eval -= 20;
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
  for (int sq = 0; sq < 64; sq++) {
    Piece piece = board.at(Square(sq));
    if (piece == PieceType::NONE) continue;

    // std::cout<<sq<<" Actual Index ";
    int index = (piece.color() == Color::WHITE) ? sq : mirrorIndex(sq);
    // std::cout<<index<<" Mirrored Index ";

    int squareValue = 0;

    if(piece == PieceType::PAWN){
      squareValue = PAWN_TABLE[index];
    }
    else if(piece == PieceType::KNIGHT){
      squareValue = KNIGHT_TABLE[index];
    }
    else if(piece == PieceType::BISHOP){
      squareValue = BISHOP_TABLE[index];
    }
    else if(piece == PieceType::ROOK){
      squareValue = ROOK_TABLE[index];
    }
    else if(piece == PieceType::QUEEN){
      squareValue = QUEEN_TABLE[index];
    }
    else if(piece == PieceType::KING){
      squareValue = isEndgame ? KING_END_TABLE[index] : KING_MIDDLE_TABLE[index];
    }

    // std::cout<<piece.color()<<" "<<piece.type()<<" Index: "<<index<<" Value: "<<squareValue<<"\n";

    eval += (piece.color() == Color::WHITE) ? squareValue : -squareValue;
  }
}

//! This function is Ai generated I don't want to get stuck in bitboards for now. I hope it works
void Engine::evaluatePawns(int& eval, const chess::Bitboard& whitePawns, const chess::Bitboard& blackPawns){
  chess::Bitboard fileBB;

  for (int file = 0; file < 8; ++file) {
      fileBB = chess::Bitboard(0x0101010101010101ULL << file);

      chess::Bitboard whitePawnsOnFile = whitePawns & fileBB;
      chess::Bitboard blackPawnsOnFile = blackPawns & fileBB;
      int whitePawnsOnFileCount = whitePawnsOnFile.count();
      int blackPawnsOnFileCount = blackPawnsOnFile.count();

      // 1. Doubled pawns penalty
      if (whitePawnsOnFileCount > 1) {
          eval -= 15 * (whitePawnsOnFileCount - 1);
      }
      if (blackPawnsOnFileCount > 1) {
          eval += 15 * (blackPawnsOnFileCount - 1);
      }

      // 2. Isolated pawns penalty
      chess::Bitboard adjacentFiles = 0ULL;
      if (file > 0) adjacentFiles |= chess::Bitboard(0x0101010101010101ULL << (file - 1));
      if (file < 7) adjacentFiles |= chess::Bitboard(0x0101010101010101ULL << (file + 1));

      if (whitePawnsOnFileCount > 0 && (whitePawns & adjacentFiles).empty()) {
          eval -= 15 * whitePawnsOnFileCount;
      }
      if (blackPawnsOnFileCount > 0 && (blackPawns & adjacentFiles).empty()) {
          eval += 15 * blackPawnsOnFileCount;
      }

      // 3. Passed pawns bonus
      while(whitePawnsOnFile) {
          Square sq = whitePawnsOnFile.pop();
          if (isPassedPawn(sq, Color::WHITE, blackPawns)) {
              eval += 15 * (sq.rank() - 1);
          }
      }

      while(blackPawnsOnFile) {
          Square sq = blackPawnsOnFile.pop();
          if (isPassedPawn(sq, Color::BLACK, whitePawns)) {
              eval -= 15 * (6 - sq.rank());
          }
      }
  }
}

//! This function is Ai generated
bool Engine::isPassedPawn(Square sq, Color color, const chess::Bitboard& opponentPawns) {
    int file = sq.file();
    int rank = sq.rank();

    // Create a bitmask for the files including the pawn's file and adjacent files
    chess::Bitboard file_mask = chess::Bitboard(0x0101010101010101ULL << file);
    if (file > 0) file_mask |= chess::Bitboard(0x0101010101010101ULL << (file - 1));
    if (file < 7) file_mask |= chess::Bitboard(0x0101010101010101ULL << (file + 1));

    // Create a bitmask for the ranks in front of the pawn
    chess::Bitboard rank_mask = 0ULL;
    if (color == Color::WHITE) {
        // Ranks in front of a white pawn (rank + 1 to 7)
        if (rank < 7) rank_mask = ~((1ULL << ((rank + 1) * 8)) - 1);
    } else { // BLACK
        // Ranks in front of a black pawn (rank - 1 to 0)
        if (rank > 0) rank_mask = (1ULL << (rank * 8)) - 1;
    }

    // A pawn is passed if there are no opponent pawns in the path in front of it
    return (opponentPawns & file_mask & rank_mask).empty();
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