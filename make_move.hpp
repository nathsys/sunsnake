
#pragma once

// Add piece to board.
#include "defaults.hpp"
#include "chess_board.hpp"

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



#endif // count_moves