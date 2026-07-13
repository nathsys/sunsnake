#include "dynamic_tables.hpp"
#include "defaults.hpp"
#include "util.hpp"
#include <sys/stat.h>

#pragma once

namespace sunsnake
{

// Get king move board.
constexpr inline BitBoard get_king_move(const Square square)
{
	return Lookup::KING_MOVE_SQUARES[square];
}

// Get knight move board.
constexpr inline BitBoard get_knight_move(const Square square)
{
	return Lookup::KNIGHT_MOVE_SQUARES[square];
}

// Use pext if instruction is available on hardware.
#ifdef __BMI2__
// Bishop moving logic.
inline BitBoard get_bishop_move(const Square square, const BitBoard occ)
{
	return Lookup::bishop_pext_atk[Lookup::bishop_pext_offset[square] + static_cast<uint32_t>(pext(occ, Lookup::bishop_masks[square]))];
}

// Rook move logic.
inline BitBoard get_rook_move(const Square square, const BitBoard occ)
{
	return Lookup::rook_pext_atk[Lookup::rook_pext_offset[square] + static_cast<uint32_t>(pext(occ, Lookup::rook_masks[square]))];
}

#else
// Bishop moving logic.
static inline BitBoard get_bishop_move(const Square square, const BitBoard occ)
{
	uint64_t mask = Lookup::bishop_masks[square];
	uint64_t occupancy = occ & mask;
	occupancy *= Lookup::bishop_magic_numbers[square];
	occupancy >>= (64 - pcnt(mask));
	return Lookup::bishop_attacks[square][occupancy];
}

// Rook move logic.
static inline BitBoard get_rook_move(const Square square, const BitBoard occ)
{
	uint64_t mask = Lookup::rook_masks[square];
	uint64_t occupancy = occ & mask;
	occupancy *= Lookup::rook_magic_numbers[square];
	occupancy >>= (64 - pcnt(mask));
	return Lookup::rook_attacks[square][occupancy];
}
#endif

// Queen move logic.
inline BitBoard get_queen_move(const Square square, const BitBoard occ)
{
	return get_bishop_move(square, occ) | get_rook_move(square, occ);
}

template <PieceType piece, SliderType slider_override = SliderType::none>
inline BitBoard get_slider_move(const Square square, const BitBoard occ)
{
	if constexpr (slider_override == SliderType::none && piece == PieceType::queen)
	{
		return get_queen_move(square, occ);
	}
	else if constexpr (piece == PieceType::bishop || slider_override == SliderType::diagonal)
	{
		return get_bishop_move(square, occ);
	}
	else
	{
		return get_rook_move(square, occ);
	}
}

template <Player player>
inline BitBoard get_pawn_forward(const BitBoard piece_board)
{
	if constexpr (player == Player::white)
		return piece_board << 8;
	else
		return piece_board >> 8;
}

// Pawn move including double push.
template <Player player>
inline BitBoard get_pawn_double(const BitBoard piece_board, const BitBoard occ)
{
	const BitBoard single = get_pawn_forward<player>(piece_board) & ~occ;
	return get_pawn_forward<player>(single) & ~occ;
}

// Direction specific pawn attacks.
template <Player player, bool left>
inline BitBoard get_pawn_attack(const BitBoard piece_board)
{
	if constexpr (player == Player::white)
	{
		if constexpr (left)
			return (piece_board & c_no_left_edge) << 9;
		else
			return (piece_board & c_no_right_edge) << 7;
	}
	else
	{
		if constexpr (left)
			return (piece_board & c_no_left_edge) >> 7;
		else
			return (piece_board & c_no_right_edge) >> 9;
	}
}

// Get pawn diagonals, without trimming any form of illegal moves or edge cases.
template <Player player>
inline BitBoard get_pawn_attacks(const BitBoard piece_board)
{
	return get_pawn_attack<player, true>(piece_board) | get_pawn_attack<player, false>(piece_board);
}

struct PawnTargets
{
	BitBoard forward;
	BitBoard left_attack;
	BitBoard right_attack;
};

template <Player player>
inline PawnTargets get_pawn_targets(BitBoard from, BitBoard push_mask, BitBoard enemy_pieces)
{
	return {
		get_pawn_forward<player>(from) & push_mask,
		get_pawn_attack<player, true>(from) & enemy_pieces,
		get_pawn_attack<player, false>(from) & enemy_pieces,
	};
}

} // namespace sunsnake
