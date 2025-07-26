#include "evaluation.hpp"
#include "constants.hpp"
#include "piece-maps.hpp"

// Evaluation related functions
bool Evaluation::hasCastled(const chess::Board &board, chess::Color color) {
  Square kingSq = board.pieces(PieceType::KING, color).lsb();

  if (color == Color::WHITE)
    return kingSq.index() == 6 || kingSq.index() == 2; // G1 or C1
  else
    return kingSq.index() == 62 || kingSq.index() == 58; // G8 or C8
}

int Evaluation::evaluate(const chess::Board &board) {
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
  int whiteMaterial = whitePawns.count() * PAWN_VALUE + whiteKnights.count() * KNIGHT_VALUE +
                      whiteBishops.count() * BISHOP_VALUE + whiteRooks.count() * ROOK_VALUE +
                      whiteQueens.count() * QUEEN_VALUE;

  int blackMaterial = blackPawns.count() * PAWN_VALUE + blackKnights.count() * KNIGHT_VALUE +
                      blackBishops.count() * BISHOP_VALUE + blackRooks.count() * ROOK_VALUE +
                      blackQueens.count() * QUEEN_VALUE;

  eval += whiteMaterial - blackMaterial;

  // Better endgame detection based on total material
  int totalMaterial = whiteMaterial + blackMaterial;
  bool isEndgame = totalMaterial < 2500; // Endgame when less than ~25 points of material total
  // bool isOpening = totalMaterial > 6000; // Opening when more than ~60 points
  // of material total

  // Castling bonus (opening/middlegame only)
  if (!isEndgame) {
    if (hasCastled(board, Color::WHITE))
      eval += 15;
    if (hasCastled(board, Color::BLACK))
      eval -= 15;
  }

  if (whiteBishops.count() >= 2) {
    eval += 50; // Bishop pair bonus for white
  }
  if (blackBishops.count() >= 2) {
    eval -= 50; // Bishop pair bonus for black
  }

  evaluatePST(board, eval, isEndgame);

  evaluatePawns(eval, whitePawns, blackPawns);

  Bitboard allPawns = whitePawns | blackPawns;

  evaluateRooks(eval, allPawns, whiteRooks, whitePawns, 1);
  evaluateRooks(eval, allPawns, blackRooks, blackPawns, -1);
  // Rooks on 7th rank
  Bitboard seventhRank = Bitboard(0x00FF000000000000ULL);
  Bitboard secondRank = Bitboard(0x000000000000FF00ULL);
  eval += (whiteRooks & seventhRank).count() * 40;
  eval -= (blackRooks & secondRank).count() * 40;

  evaluateKnightOutposts(eval, whiteKnights, blackPawns, 1);
  evaluateKnightOutposts(eval, blackKnights, whitePawns, -1);

  evaluateMobility(eval, board, whiteKnights, whiteBishops, whiteRooks, whiteQueens, 1);
  evaluateMobility(eval, board, blackKnights, blackBishops, blackRooks, blackQueens, -1);

  if (isEndgame) {
    evaluateKingEndgameScore(board, eval);
  }

  return eval;
}

// Add values from piece square tables
void Evaluation::evaluatePST(const chess::Board &board, int &eval, bool isEndgame) {
  for (int sq = 0; sq < 64; sq++) {
    Piece piece = board.at(Square(sq));
    if (piece == PieceType::NONE)
      continue;

    // std::cout<<sq<<" Actual Index ";
    int index = (piece.color() == Color::WHITE) ? sq : mirrorIndex(sq);
    // std::cout<<index<<" Mirrored Index ";

    int squareValue = 0;

    if (piece == PieceType::PAWN) {
      squareValue = PAWN_TABLE[index];
    } else if (piece == PieceType::KNIGHT) {
      squareValue = KNIGHT_TABLE[index];
    } else if (piece == PieceType::BISHOP) {
      squareValue = BISHOP_TABLE[index];
    } else if (piece == PieceType::ROOK) {
      squareValue = ROOK_TABLE[index];
    } else if (piece == PieceType::QUEEN) {
      squareValue = QUEEN_TABLE[index];
    } else if (piece == PieceType::KING) {
      squareValue = isEndgame ? KING_END_TABLE[index] : KING_MIDDLE_TABLE[index];
    }

    // std::cout<<piece.color()<<" "<<piece.type()<<" Index: "<<index<<" Value:
    // "<<squareValue<<"\n";

    eval += (piece.color() == Color::WHITE) ? squareValue : -squareValue;
  }
}

void Evaluation::evaluatePawns(int &eval, const chess::Bitboard &whitePawns,
                               const chess::Bitboard &blackPawns) {
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
    if (file > 0)
      adjacentFiles |= chess::Bitboard(0x0101010101010101ULL << (file - 1));
    if (file < 7)
      adjacentFiles |= chess::Bitboard(0x0101010101010101ULL << (file + 1));

    if (whitePawnsOnFileCount > 0 && (whitePawns & adjacentFiles).empty()) {
      eval -= 15 * whitePawnsOnFileCount;
    }
    if (blackPawnsOnFileCount > 0 && (blackPawns & adjacentFiles).empty()) {
      eval += 15 * blackPawnsOnFileCount;
    }

    // 3. Passed pawns bonus
    while (whitePawnsOnFile) {
      Square sq = whitePawnsOnFile.pop();
      if (isPassedPawn(sq, Color::WHITE, blackPawns)) {
        eval += 15 * (sq.rank() - 1);
      }
    }

    while (blackPawnsOnFile) {
      Square sq = blackPawnsOnFile.pop();
      if (isPassedPawn(sq, Color::BLACK, whitePawns)) {
        eval -= 15 * (6 - sq.rank());
      }
    }
  }
}

bool Evaluation::isPassedPawn(Square sq, Color color, const chess::Bitboard &opponentPawns) {
  int file = sq.file();
  int rank = sq.rank();

  // Create a bitmask for the files including the pawn's file and adjacent files
  chess::Bitboard file_mask = chess::Bitboard(0x0101010101010101ULL << file);
  if (file > 0)
    file_mask |= chess::Bitboard(0x0101010101010101ULL << (file - 1));
  if (file < 7)
    file_mask |= chess::Bitboard(0x0101010101010101ULL << (file + 1));

  // Create a bitmask for the ranks in front of the pawn
  chess::Bitboard rank_mask = 0ULL;
  if (color == Color::WHITE) {
    // Ranks in front of a white pawn (rank + 1 to 7)
    if (rank < 7)
      rank_mask = ~((1ULL << ((rank + 1) * 8)) - 1);
  } else { // BLACK
    // Ranks in front of a black pawn (rank - 1 to 0)
    if (rank > 0)
      rank_mask = (1ULL << (rank * 8)) - 1;
  }

  // A pawn is passed if there are no opponent pawns in the path in front of it
  return (opponentPawns & file_mask & rank_mask).empty();
}

void Evaluation::evaluateRooks(int &eval, const Bitboard &allPawns, const Bitboard &rooks,
                               const Bitboard &friendlyPawns, int multiplier) {
  Bitboard tempRooks = rooks;
  while (tempRooks) {
    Square rookSq = tempRooks.pop();
    File rookFile = rookSq.file();

    // Create a file mask for this file
    Bitboard fileMask = Bitboard(0);
    for (int rank = 0; rank < 8; rank++) {
      Square sq = Square(rookFile, Rank(rank));
      fileMask |= Bitboard::fromSquare(sq);
    }

    // Check file occupancy using bitboard operations
    bool hasAnyPawn = (allPawns & fileMask) != 0;
    bool hasFriendlyPawn = (friendlyPawns & fileMask) != 0;

    if (!hasAnyPawn) {
      eval += 30 * multiplier; // Open file bonus
    } else if (!hasFriendlyPawn) {
      eval += 15 * multiplier; // Semi-open file bonus
    }
  }
}

void Evaluation::evaluateKnightOutposts(int &eval, const Bitboard &knights,
                                        const Bitboard &enemyPawns, int multiplier) {

  // Create masks for advanced ranks where outposts matter
  Bitboard advancedRanks;
  if (multiplier == 1) {                             // White
    advancedRanks = Bitboard(0x00FFFFFF00000000ULL); // Ranks 5-8
  } else {                                           // Black
    advancedRanks = Bitboard(0x00000000FFFFFF00ULL); // Ranks 2-5
  }

  // Only consider knights on advanced ranks
  Bitboard advancedKnights = knights & advancedRanks;
  if (!advancedKnights)
    return; // Early exit if no advanced knights

  // Create pawn attack mask - all squares enemy pawns can attack
  Bitboard enemyPawnAttacks;
  if (multiplier == 1) { // White knights vs black pawns
    enemyPawnAttacks = ((enemyPawns & ~Bitboard(0x0101010101010101ULL)) << 7) | // Left attacks
                       ((enemyPawns & ~Bitboard(0x8080808080808080ULL)) << 9);  // Right attacks
  } else { // Black knights vs white pawns
    enemyPawnAttacks = ((enemyPawns & ~Bitboard(0x8080808080808080ULL)) >> 7) | // Right attacks
                       ((enemyPawns & ~Bitboard(0x0101010101010101ULL)) >> 9);  // Left attacks
  }

  // Knights that can't be attacked by pawns = outposts
  Bitboard outpostKnights = advancedKnights & ~enemyPawnAttacks;

  // Award bonus for each outpost knight
  eval += outpostKnights.count() * 25 * multiplier;
}

void Evaluation::evaluateMobility(int &eval, const Board &board, const Bitboard &knights,
                                  const Bitboard &bishops, const Bitboard &rooks,
                                  const Bitboard &queens, int multiplier) {

  Bitboard occupied = board.occ(); // All pieces on board
  Bitboard friendlyPieces = (multiplier == 1) ? board.us(Color::WHITE) : board.us(Color::BLACK);

  int mobility = 0;

  // Knight mobility
  Bitboard tempKnights = knights;
  while (tempKnights) {
    Square sq = tempKnights.pop();
    Bitboard moves = attacks::knight(sq) & ~friendlyPieces; // Can't capture own pieces
    mobility += moves.count() * 4;                          // 4 points per square
  }

  // Bishop mobility
  Bitboard tempBishops = bishops;
  while (tempBishops) {
    Square sq = tempBishops.pop();
    Bitboard moves = attacks::bishop(sq, occupied) & ~friendlyPieces;
    mobility += moves.count() * 3; // 3 points per square
  }

  // Rook mobility
  Bitboard tempRooks = rooks;
  while (tempRooks) {
    Square sq = tempRooks.pop();
    Bitboard moves = attacks::rook(sq, occupied) & ~friendlyPieces;
    mobility += moves.count() * 2; // 2 points per square
  }

  // Queen mobility (but limit it so queens don't dominate)
  Bitboard tempQueens = queens;
  while (tempQueens) {
    Square sq = tempQueens.pop();
    Bitboard moves = attacks::queen(sq, occupied) & ~friendlyPieces;
    int queenMobility = std::min(moves.count(), 14); // Cap at 14 squares
    mobility += queenMobility * 1;                   // 1 point per square
  }

  eval += mobility * multiplier;
}

// Calculate King endgame score
void Evaluation::evaluateKingEndgameScore(const chess::Board &board, int &eval) {
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