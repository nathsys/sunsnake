#include "defaults.hpp"
#include "move_logic.hpp"
#include "util.hpp"

#pragma once

struct Board {
    // Store the white pieces state.
	BitBoard w_pawn = INIT_PAWN_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_king = INIT_KING_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_rook = INIT_ROOK_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_bishop = INIT_BISHOP_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_knight = INIT_KNIGHT_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_queen = INIT_QUEEN_SQUARES & INIT_WHITE_PIECES;

    // Store the black pieces state.
	BitBoard b_pawn = INIT_PAWN_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_king = INIT_KING_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_rook = INIT_ROOK_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_bishop = INIT_BISHOP_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_knight = INIT_KNIGHT_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_queen = INIT_QUEEN_SQUARES & INIT_BLACK_PIECES;

    // Occupancy boards.
	BitBoard w_board = INIT_WHITE_PIECES;
	BitBoard b_board = INIT_BLACK_PIECES;
	BitBoard occ_board = INIT_TOTAL_SQUARES;

	template<Player player>
	constexpr inline BitBoard get_player_occ() const {
		if constexpr (player == Player::white) {
			return w_board;
		} else {
			return b_board;
		}
	}

	template<Player player, PieceType piece>
	constexpr inline BitBoard get_piece_board() const {
		if constexpr (player == Player::white) {
			if constexpr (piece == PieceType::pawn) {
				return w_pawn;
			} else if constexpr (piece == PieceType::knight) {
				return w_knight;
			} else if constexpr (piece == PieceType::bishop) {
				return w_bishop;
			} else if constexpr (piece == PieceType::rook) {
				return w_rook;
			} else if constexpr (piece == PieceType::queen) {
				return w_queen;
			} else {
				return w_king;
			}
		} else {
			if constexpr (piece == PieceType::pawn) {
				return b_pawn;
			} else if constexpr (piece == PieceType::knight) {
				return b_knight;
			} else if constexpr (piece == PieceType::bishop) {
				return b_bishop;
			} else if constexpr (piece == PieceType::rook) {
				return b_rook;
			} else if constexpr (piece == PieceType::queen) {
				return b_queen;
			} else {
				return b_king;
			}
		}
	}

	template <Player player>
	inline bool is_attacked(BitBoard pos) const {
		Square square = lbit(pos);
		return (get_pawn_attacks<player>(pos)
			& get_piece_board<opponent<player>(), PieceType::pawn>())
			|| (get_knight_move(square) 
				& get_piece_board<opponent<player>(), PieceType::knight>())
			|| (get_rook_move(square, occ_board)
				& (get_piece_board<opponent<player>(), PieceType::rook>() | get_piece_board<opponent<player>(), PieceType::queen>()))
			|| (get_bishop_move(square, occ_board)
				& (get_piece_board<opponent<player>(), PieceType::bishop>() | get_piece_board<opponent<player>(), PieceType::queen>()))
			|| (get_king_move(square) & get_piece_board<opponent<player>(), PieceType::king>());
	}

	template <bool white, PieceType piece>
	inline BitBoard* get_board_pointer() {
		switch (piece) {
		case PieceType::pawn:
			return white ? &w_pawn : &b_pawn;
		case PieceType::king:
			return white ? &w_king : &b_king;
		case PieceType::rook:
			return white ? &w_rook : &b_rook;
		case PieceType::bishop:
			return white ? &w_bishop : &b_bishop;
		case PieceType::knight:
			return white ? &w_knight : &b_knight;
		case PieceType::queen:
			return white ? &w_queen : &b_queen;
		}
	}
	
	template <bool white>
	inline PieceType get_piece_type(BitBoard mask) const {
		if (get_piece_board<white, PieceType::pawn>() & mask) return PieceType::pawn;
		else if (get_piece_board<white, PieceType::knight>() & mask) return PieceType::knight;
		else if (get_piece_board<white, PieceType::bishop>() & mask) return PieceType::bishop;
		else if (get_piece_board<white, PieceType::rook>() & mask) return PieceType::rook;
		else if (get_piece_board<white, PieceType::queen>() & mask) return PieceType::queen;
		else if (get_piece_board<white, PieceType::king>() & mask) return PieceType::king;
		throw std::runtime_error("No piece found.");
	}
};