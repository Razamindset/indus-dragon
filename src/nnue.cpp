#include "nnue.hpp"
#include "nnue_data.hpp"
#include <iostream>
#include <cassert>

namespace NNUE {
    alignas(32) int16_t FEATURE_WEIGHTS[INPUT_FEATURES][HIDDEN_SIZE];
    alignas(32) int16_t FEATURE_BIASES[HIDDEN_SIZE];
    alignas(32) int16_t OUTPUT_WEIGHTS[HIDDEN_SIZE];
    int16_t OUTPUT_BIAS = 0;

    void Network::load_network(const std::string &path) {
        size_t offset = 0;

        // In out trainign code we go from 768 to 256
        // The layout in the .bin file is as follows
        // Each row has 768 int_16 values one and there are total 256 vlaues
        // Each row maps all inputs to some hidden layer
        // So we get 256 hidden size

        // Then we have 256 cols and one row for the hidden bias
        // then 256 cols and 1 row for the hidden weights
        // then 1 bias vlaue for the hidden or the output bias
        // This is the current network for now it is not much but
        // i am happpy that i ahve something working
        // In future for sure I will improve a lot of code and the netwrok

        // FEATURE_WEIGHTS [INPUT_FEATURES][HIDDEN_SIZE]
        for (int r = 0; r < HIDDEN_SIZE; ++r) {
            for (int c = 0; c < INPUT_FEATURES; ++c) {
                FEATURE_WEIGHTS[c][r] = NNUE_DATA[offset++];
            }
        }

        // FEATURE_BIAS [HIDDEN_SIZE]
        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            FEATURE_BIASES[i] = NNUE_DATA[offset++];
        }

        // OUTPUT_WEIGHTS [HIDDEN_SIZE]
        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            OUTPUT_WEIGHTS[i] = NNUE_DATA[offset++];
        }

        // OUTPUT_BIAS
        OUTPUT_BIAS = NNUE_DATA[offset++];
    }

    void Network::refreshAccumulator(const chess::Board &board, Accumulator &acc) {
        // Start from the bias
        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            acc.values[i] = FEATURE_BIASES[i];
        }

        // Now let's add the active peice features
        chess::Bitboard pieces = board.us(chess::Color::WHITE) | board.us(chess::Color::BLACK);

        while (pieces) {
            chess::Square sq = pieces.pop();
            auto piece = board.at(sq);

            if (piece.type() == chess::PieceType::NONE)
                continue;

            // Convert into 768 plane idx
            const int idx = getPieceIndex(
                piece.color(),
                piece.type(),
                sq);

            for (int h = 0; h < HIDDEN_SIZE; ++h) {
                acc.values[h] += FEATURE_WEIGHTS[idx][h];
            }
        }
    }

    void Network::updateAccumulator(const chess::Board &board, chess::Move move, Accumulator &acc) {
        const chess::Color stm = board.sideToMove();
        const auto from = move.from();
        const auto to = move.to();
        const auto piece = board.at(from);

        if (piece.type() == chess::PieceType::NONE) {
            return;
        }

        auto updatePiece = [&](Accumulator &a, int idx, bool add) {
            for (int h = 0; h < HIDDEN_SIZE; ++h) {
                if (add) a.values[h] += FEATURE_WEIGHTS[idx][h];
                else a.values[h] -= FEATURE_WEIGHTS[idx][h];
            }
        };

        // Remove moving piece from original square
        updatePiece(acc, getPieceIndex(piece.color(), piece.type(), from), false);

        if (move.typeOf() == chess::Move::CASTLING) {
            const bool king_side = (to.index() > from.index());
            const auto rook_from = to;
            const auto rook = board.at(rook_from);
            const auto king_to = chess::Square::castling_king_square(king_side, stm);
            const auto rook_to = chess::Square::castling_rook_square(king_side, stm);

            updatePiece(acc, getPieceIndex(rook.color(), rook.type(), rook_from), false);
            updatePiece(acc, getPieceIndex(rook.color(), rook.type(), rook_to), true);
            updatePiece(acc, getPieceIndex(piece.color(), piece.type(), king_to), true);
        } else if (move.typeOf() == chess::Move::PROMOTION) {
            const auto captured = board.at(to);
            if (captured != chess::Piece::NONE) {
                updatePiece(acc, getPieceIndex(captured.color(), captured.type(), to), false);
            }
            updatePiece(acc, getPieceIndex(stm, move.promotionType(), to), true);
        } else if (move.typeOf() == chess::Move::ENPASSANT) {
            updatePiece(acc, getPieceIndex(piece.color(), piece.type(), to), true);
            const auto captured_pawn_sq = chess::Square(to.file(), from.rank());
            const auto captured_pawn = board.at(captured_pawn_sq);
            updatePiece(acc, getPieceIndex(captured_pawn.color(), captured_pawn.type(), captured_pawn_sq), false);
        } else {
            const auto captured = board.at(to);
            if (captured != chess::Piece::NONE) {
                updatePiece(acc, getPieceIndex(captured.color(), captured.type(), to), false);
            }
            updatePiece(acc, getPieceIndex(piece.color(), piece.type(), to), true);
        }
    }

    // Convert sigmoid output back to cp
    int Network::sigmoidToCp(float prob) {
        prob = std::clamp(prob, 0.0001f, 0.9999f);
        return static_cast<int>(
            -400.0f * std::log10((1.0f / prob) - 1.0f));
    }

    // Final board evaluation
    int Network::evaluate(chess::Color stm, const Accumulator& acc) const{
        int32_t sum = OUTPUT_BIAS;

        // Hidden -> output
        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            int32_t activated = std::max<int32_t>(0, acc.values[i]);
            sum += activated * OUTPUT_WEIGHTS[i];
        }

        // Undo the scaling from the export 
        // Undo scaling from export (*255 twice)
        float x = static_cast<float>(sum) /
        static_cast<float>(SCALE * SCALE);

        // Same sigmoid as PyTorch
        // prob is the probability of winning for for given side out of 1
        float prob = 1.0f / (1.0f + std::exp(-x));
        
        // We can convert this probability to cp value
        int cp = sigmoidToCp(prob);

        // Return from side-to-move perspective
        return (stm == chess::Color::WHITE) ? cp : -cp;
    }

}