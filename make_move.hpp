
#pragma once

// Add piece to board.
#include "defaults.hpp"
#include "chess_board.hpp"
#include "util.hpp"

#ifdef count_moves
// If we are just counting moves, we can use templated functions to increase performance.

template <Player player, PieceType piece>
inline void add_to_board(Board& b, BitBoard mask) {
	*b.get_board_pointer<player, piece>() |= mask;
	b.occ_board |= mask;
	if constexpr (player == Player::white)
		b.w_board |= mask;
	else
		b.b_board |= mask;
}

template <Player player, PieceType piece>
inline void remove_from_board(Board& b, BitBoard mask) {
	*b.get_board_pointer<player, piece>() ^= mask;
	b.occ_board ^= mask;
	if constexpr (player == Player::white)
		b.w_board ^= mask;
	else
		b.b_board ^= mask;
}

// Move a piece.
template <Player player, PieceType piece>
inline void move_piece(Board& b, BitBoard move) {
	*b.get_board_pointer<player, piece>() ^= move;
	b.occ_board ^= move;
	if constexpr (player == Player::white)
		b.w_board ^= move;
	else
		b.b_board ^= move;
}

template <Player player, PieceType piece, PieceType captured>
inline void capture_move(Board& b, BitBoard from, BitBoard to) {
	remove_from_board<player, captured>(b, to);
	move_piece<player, piece>(b, from | to);
}

template <Player player, PieceType p, PieceType captured>
inline void unmake_capture_move(Board& b, BitBoard from, BitBoard to) {
	move_piece<player, p>(b, from | to);
	add_to_board<opponent<player>(), captured>(b, to);
}

// Do pawn double forward move. Retrurns true if en passant is possible.
template <Player player>
inline bool pawn_double(Board& b, BitBoard move, BitBoard to) {
	move_piece<player, PieceType::pawn>(b, move);
	// If we move pawn to a square next to a pawn, en pessant is possible for next turn.
	const BitBoard enemy_pawns = b.get_piece_board<opponent<player>(), PieceType::pawn>();
	const BitBoard en_passant_squares = player == Player::white ? c_en_passant_squares_w : c_en_passant_squares_b;
	return en_passant_squares & enemy_pawns;
}

template <Player player>
inline void unmake_pawn_double(Board& b, BitBoard move) {
	move_piece<player, PieceType::pawn>(b, move);
}

#endif // count_moves