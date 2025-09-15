#include "evaluation.hpp"

#include "constants.hpp"
#include "piece-maps.hpp"

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
  int whiteMaterial =
      whitePawns.count() * PAWN_VALUE + whiteKnights.count() * KNIGHT_VALUE +
      whiteBishops.count() * BISHOP_VALUE + whiteRooks.count() * ROOK_VALUE +
      whiteQueens.count() * QUEEN_VALUE;

  int blackMaterial =
      blackPawns.count() * PAWN_VALUE + blackKnights.count() * KNIGHT_VALUE +
      blackBishops.count() * BISHOP_VALUE + blackRooks.count() * ROOK_VALUE +
      blackQueens.count() * QUEEN_VALUE;

  eval += whiteMaterial - blackMaterial;

  // Better endgame detection based on total material
  int totalMaterial = whiteMaterial + blackMaterial;
  bool isEndgame = totalMaterial <
                   2500;  // Endgame when less than ~25 points of material total

  evaluatePST(board, eval, isEndgame);

  evaluatePawns(eval, whitePawns, blackPawns);

  if (isEndgame) {
    evaluateKingEndgameScore(board, eval);
  }

  // Slight advantage for having bishop pair
  if (whiteBishops.count() >= 2) {
    eval += 30;
  }
  if (blackBishops.count() >= 2) {
    eval -= 30;
  }

  return (board.sideToMove() == Color::WHITE) ? eval : -eval;
}

// Add values from piece square tables
void Evaluation::evaluatePST(const chess::Board &board, int &eval,
                             bool isEndgame) {
  for (int sq = 0; sq < 64; sq++) {
    Piece piece = board.at(Square(sq));
    if (piece == PieceType::NONE) continue;

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
      squareValue =
          isEndgame ? KING_END_TABLE[index] : KING_MIDDLE_TABLE[index];
    }

    // std::cout<<piece.color()<<" "<<piece.type()<<" Index: "<<index<<"
    // Value:
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

    // Create masks for adjacent files and passed pawn checks
    chess::Bitboard adjacentFilesMask = 0ULL;
    if (file > 0)
      adjacentFilesMask |= chess::Bitboard(0x0101010101010101ULL << (file - 1));
    if (file < 7)
      adjacentFilesMask |= chess::Bitboard(0x0101010101010101ULL << (file + 1));

    chess::Bitboard passedPawnMask = fileBB | adjacentFilesMask;

    // 2. Isolated pawns penalty
    if (whitePawnsOnFileCount > 0 && (whitePawns & adjacentFilesMask).empty()) {
      eval -= 15 * whitePawnsOnFileCount;
    }
    if (blackPawnsOnFileCount > 0 && (blackPawns & adjacentFilesMask).empty()) {
      eval += 15 * blackPawnsOnFileCount;
    }

    // 3. Passed pawns bonus
    // White passed pawns
    chess::Bitboard tempWhitePawns = whitePawnsOnFile;
    while (tempWhitePawns) {
      Square sq = tempWhitePawns.pop();
      int rank = sq.rank();
      chess::Bitboard rank_mask = 0ULL;
      if (rank < 7) {
        rank_mask = ~((1ULL << ((rank + 1) * 8)) - 1);
      }
      if ((blackPawns & passedPawnMask & rank_mask).empty()) {
        eval += 15 * (rank - 1);
      }
    }

    // Black passed pawns
    chess::Bitboard tempBlackPawns = blackPawnsOnFile;
    while (tempBlackPawns) {
      Square sq = tempBlackPawns.pop();
      int rank = sq.rank();
      chess::Bitboard rank_mask = 0ULL;
      if (rank > 0) {
        rank_mask = (1ULL << (rank * 8)) - 1;
      }
      if ((whitePawns & passedPawnMask & rank_mask).empty()) {
        eval -= 15 * (6 - rank);
      }
    }
  }
}

// Calculate King endgame score
void Evaluation::evaluateKingEndgameScore(const chess::Board &board,
                                          int &eval) {
  Square whiteKingSq = board.kingSq(Color::WHITE);
  Square blackKingSq = board.kingSq(Color::BLACK);

  // Reward active king: encourage the king to move towards the center
  int whiteKingCentrality = KING_END_TABLE[whiteKingSq.index()];
  int blackKingCentrality = KING_END_TABLE[mirrorIndex(blackKingSq.index())];
  eval += whiteKingCentrality - blackKingCentrality;

  // Encourage the stronger side's king to attack the weaker king
  int distance = manhattanDistance(whiteKingSq, blackKingSq);
  if (eval > 0) {  // White is stronger
    eval += (14 - distance) * 4;
  } else {  // Black is stronger
    eval -= (14 - distance) * 4;
  }
}