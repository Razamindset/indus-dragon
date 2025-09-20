#include "evaluation.hpp"

#include "constants.hpp"
#include "piece-maps.hpp"

int Evaluation::evaluate(const chess::Board &board) {
  int eval = 0;

  // I actually donot know if this is the best way to do this. I will read some
  // code from other engines for the best approach

  // 1. Create and populate the evaluation context
  EvalContext ctx{
      .board = board,
  };

  ctx.whitePawns = board.pieces(PieceType::PAWN, Color::WHITE);
  ctx.blackPawns = board.pieces(PieceType::PAWN, Color::BLACK);
  ctx.whiteKnights = board.pieces(PieceType::KNIGHT, Color::WHITE);
  ctx.blackKnights = board.pieces(PieceType::KNIGHT, Color::BLACK);
  ctx.whiteBishops = board.pieces(PieceType::BISHOP, Color::WHITE);
  ctx.blackBishops = board.pieces(PieceType::BISHOP, Color::BLACK);
  ctx.whiteRooks = board.pieces(PieceType::ROOK, Color::WHITE);
  ctx.blackRooks = board.pieces(PieceType::ROOK, Color::BLACK);
  ctx.whiteQueens = board.pieces(PieceType::QUEEN, Color::WHITE);
  ctx.blackQueens = board.pieces(PieceType::QUEEN, Color::BLACK);

  ctx.whiteMaterial = ctx.whitePawns.count() * PAWN_VALUE +
                      ctx.whiteKnights.count() * KNIGHT_VALUE +
                      ctx.whiteBishops.count() * BISHOP_VALUE +
                      ctx.whiteRooks.count() * ROOK_VALUE +
                      ctx.whiteQueens.count() * QUEEN_VALUE;

  ctx.blackMaterial = ctx.blackPawns.count() * PAWN_VALUE +
                      ctx.blackKnights.count() * KNIGHT_VALUE +
                      ctx.blackBishops.count() * BISHOP_VALUE +
                      ctx.blackRooks.count() * ROOK_VALUE +
                      ctx.blackQueens.count() * QUEEN_VALUE;

  int totalMaterial = ctx.whiteMaterial + ctx.blackMaterial;
  ctx.isEndgame = totalMaterial < 2500;

  // 2. Run evaluation components
  eval += ctx.whiteMaterial - ctx.blackMaterial;

  evaluatePST(eval, ctx);
  evaluatePawns(eval, ctx);
  evaluateRooks(eval, ctx);

  if (ctx.isEndgame) {
    evaluateKingEndgameScore(eval, ctx);
  }

  // Slight advantage for having bishop pair
  if (ctx.whiteBishops.count() >= 2) {
    eval += 30;
  }
  if (ctx.blackBishops.count() >= 2) {
    eval -= 30;
  }

  // Todos
  // 1. King safety Pawn sheilds etc
  // 2. Mobility Score.
  // 3. Tappered evaluations
  // 4. Center control
  // 5. Rooks behind passed pawns.

  // Small Tempo bonus
  if (board.sideToMove() == Color::WHITE)
    eval += TEMPO_BONUS;
  else
    eval -= TEMPO_BONUS;

  // 3. Return score relative to side to move
  return (board.sideToMove() == Color::WHITE) ? eval : -eval;
}

// Add values from piece square tables
void Evaluation::evaluatePST(int &eval, const EvalContext &ctx) {
  for (int sq = 0; sq < 64; sq++) {
    Piece piece = ctx.board.at(Square(sq));
    if (piece == PieceType::NONE) continue;

    int index = (piece.color() == Color::WHITE) ? sq : mirrorIndex(sq);
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
          ctx.isEndgame ? KING_END_TABLE[index] : KING_MIDDLE_TABLE[index];
    }

    eval += (piece.color() == Color::WHITE) ? squareValue : -squareValue;
  }
}

void Evaluation::evaluatePawns(int &eval, const EvalContext &ctx) {
  chess::Bitboard fileBB;

  for (int file = 0; file < 8; ++file) {
    fileBB = chess::Bitboard(0x0101010101010101ULL << file);

    chess::Bitboard whitePawnsOnFile = ctx.whitePawns & fileBB;
    chess::Bitboard blackPawnsOnFile = ctx.blackPawns & fileBB;
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
    if (whitePawnsOnFileCount > 0 &&
        (ctx.whitePawns & adjacentFilesMask).empty()) {
      eval -= 15 * whitePawnsOnFileCount;
    }
    if (blackPawnsOnFileCount > 0 &&
        (ctx.blackPawns & adjacentFilesMask).empty()) {
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
      if ((ctx.blackPawns & passedPawnMask & rank_mask).empty()) {
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
      if ((ctx.whitePawns & passedPawnMask & rank_mask).empty()) {
        eval -= 15 * (6 - rank);
      }
    }
  }
}

// Calculate King endgame score
void Evaluation::evaluateKingEndgameScore(int &eval, const EvalContext &ctx) {
  Square whiteKingSq = ctx.board.kingSq(Color::WHITE);
  Square blackKingSq = ctx.board.kingSq(Color::BLACK);

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

void Evaluation::evaluateRooks(int &eval, const EvalContext &ctx) {
  chess::Bitboard fileBB;

  for (int file = 0; file < 8; ++file) {
    fileBB = chess::Bitboard(0x0101010101010101ULL << file);

    bool whitePawnsOnFile = !(ctx.whitePawns & fileBB).empty();
    bool blackPawnsOnFile = !(ctx.blackPawns & fileBB).empty();

    if (!whitePawnsOnFile) {
      if (!blackPawnsOnFile) {
        // Open File
        if (!(ctx.whiteRooks & fileBB).empty()) eval += 15;
        if (!(ctx.blackRooks & fileBB).empty()) eval -= 15;
      } else {
        // Semi-Open File for White
        if (!(ctx.whiteRooks & fileBB).empty()) eval += 10;
      }
    }

    if (!blackPawnsOnFile) {
      if (whitePawnsOnFile) {
        // Semi-Open File for Black. Open file for black is already handled
        // above
        if (!(ctx.blackRooks & fileBB).empty()) eval -= 10;
      }
    }
  }
}