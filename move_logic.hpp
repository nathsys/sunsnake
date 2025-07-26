#include "dynamic_tables.hpp"
#include "defaults.hpp"
#include <sys/stat.h>

#pragma once

// Get king move board.
constexpr inline BitBoard get_king_move(const Square square) { return Lookup::KING_MOVE_SQUARES[square]; }

// Get knight move board.
constexpr inline BitBoard get_knight_move(const Square square) { return Lookup::KNIGHT_MOVE_SQUARES[square]; }


// Use pext if instruction is available on hardware.
#ifdef __BMI2__
// Bishop moving logic.
inline BitBoard get_bishop_move(const Square square, const BitBoard occ) {
	return Lookup::bishop_pext_atk[Lookup::bishop_pext_offset[square] + static_cast<uint32_t>(pext(occ, Lookup::bishop_masks[square]))];
}

// Rook move logic.
inline BitBoard get_rook_move(const Square square, const BitBoard occ) {
	return Lookup::rook_pext_atk[Lookup::rook_pext_offset[square] + static_cast<uint32_t>(pext(occ, Lookup::rook_masks[square]))];
}

template <SliderType slider_type>
inline BitBoard get_slider_move(const Square square, const BitBoard occ) {
	if constexpr (IS_DIAGONAL(slider_type)) {
		return get_bishop_move(square, occ);
	} else {
		return get_rook_move(square, occ);
	}
}

// Get pawn diagonals, without trimming any form of illegal moves or edge cases.
template <Player player>
static inline BitBoard get_pawn_attacks(const BitBoard piece_board) {
	if constexpr (player == Player::white)
		return ((piece_board & NO_LEFT_EDGE) << 9) | ((piece_board & NO_RIGHT_EDGE) << 7);
	else
		return ((piece_board & NO_RIGHT_EDGE) >> 9) | ((piece_board & NO_LEFT_EDGE) >> 7);
}

#else
// Bishop moving logic.
static inline BitBoard get_bishop_move(const Square square, const BitBoard occ) {
    uint64_t mask = Lookup::bishop_masks[square];
	uint64_t occupancy = occ & mask;
	occupancy *= Lookup::bishop_magic_numbers[square];
	occupancy >>= (64 - pcnt(mask));
	return Lookup::bishop_attacks[square][occupancy];
}

// Rook move logic.
static inline BitBoard get_rook_move(const Square square, const BitBoard occ) {
    uint64_t mask = Lookup::rook_masks[square];
	uint64_t occupancy = occ & mask;
	occupancy *= Lookup::rook_magic_numbers[square];
	occupancy >>= (64 - pcnt(mask));
	return Lookup::rook_attacks[square][occupancy];
}
#endif

// Queen move logic.
inline BitBoard get_queen_move(const Square square, const BitBoard occ) {
	return get_bishop_move(square, occ) | get_rook_move(square, occ);
}
