#include "defaults.hpp"
#include "move_logic.hpp"
#include "util.hpp"
#include <cstddef>

#pragma once

struct Board {
    // Store the white pieces state.
	BitBoard w_pawn = c_init_pawn_squares & c_init_white_pieces;
	BitBoard w_king = c_init_king_squares & c_init_white_pieces;
	BitBoard w_rook = c_init_rook_squares & c_init_white_pieces;
	BitBoard w_bishop = c_init_bishop_squares & c_init_white_pieces;
	BitBoard w_knight = c_init_knight_squares & c_init_white_pieces;
	BitBoard w_queen = c_init_queen_squares & c_init_white_pieces;

    // Store the black pieces state.
	BitBoard b_pawn = c_init_pawn_squares & c_init_black_pieces;
	BitBoard b_king = c_init_king_squares & c_init_black_pieces;
	BitBoard b_rook = c_init_rook_squares & c_init_black_pieces;
	BitBoard b_bishop = c_init_bishop_squares & c_init_black_pieces;
	BitBoard b_knight = c_init_knight_squares & c_init_black_pieces;
	BitBoard b_queen = c_init_queen_squares & c_init_black_pieces;

    // Occupancy boards.
	BitBoard w_board = c_init_white_pieces;
	BitBoard b_board = c_init_black_pieces;
	BitBoard occ_board = c_init_total_squares;

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

	template <Player player, PieceType piece>
	inline BitBoard* get_board_pointer() {
		switch (piece) {
		case PieceType::pawn:
			return player == Player::white ? &w_pawn : &b_pawn;
		case PieceType::king:
			return player == Player::white ? &w_king : &b_king;
		case PieceType::rook:
			return player == Player::white ? &w_rook : &b_rook;
		case PieceType::bishop:
			return player == Player::white ? &w_bishop : &b_bishop;
		case PieceType::knight:
			return player == Player::white ? &w_knight : &b_knight;
		case PieceType::queen:
			return player == Player::white ? &w_queen : &b_queen;
		}
		return nullptr; // Should never reach here, but added to avoid compiler warnings.
	}

	template <Player player, PieceType piece>
	inline PieceType get_piece_type(BitBoard mask) const {
		if (get_piece_board<player, PieceType::pawn>() & mask) return PieceType::pawn;
		else if (get_piece_board<player, PieceType::knight>() & mask) return PieceType::knight;
		else if (get_piece_board<player, PieceType::bishop>() & mask) return PieceType::bishop;
		else if (get_piece_board<player, PieceType::rook>() & mask) return PieceType::rook;
		else if (get_piece_board<player, PieceType::queen>() & mask) return PieceType::queen;
		else if (get_piece_board<player, PieceType::king>() & mask) return PieceType::king;
		throw std::runtime_error("No piece found.");
	}
};