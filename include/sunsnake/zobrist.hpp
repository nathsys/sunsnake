#include "defaults.hpp"
#include "chess_board.hpp"
#include "util.hpp"

#pragma once

namespace sunsnake
{

namespace Zobrist
{
	// Fixed, deterministic key table computed at compile time. Only used to
	// spread bits well, not meant to be cryptographically strong.
	constexpr uint64_t splitmix64(uint64_t &state)
	{
		uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
		return z ^ (z >> 31);
	}

	struct KeyTable
	{
		Hash piece_square[2][6][64] = {};
		Hash castling[16] = {};
		Hash en_passant[16] = {};
		Hash side_to_move = 0;
		Hash depth[16] = {};
	};

	constexpr KeyTable make_keys()
	{
		KeyTable table;
		uint64_t seed = 0x5EED5EED5EED5EEDULL;
		for (int player = 0; player < 2; player++)
			for (int piece = 0; piece < 6; piece++)
				for (int square = 0; square < 64; square++)
					table.piece_square[player][piece][square] = splitmix64(seed);
		for (int i = 0; i < 16; i++)
			table.castling[i] = splitmix64(seed);
		for (int i = 0; i < 16; i++)
			table.en_passant[i] = splitmix64(seed);
		table.side_to_move = splitmix64(seed);
		for (int i = 0; i < 16; i++)
			table.depth[i] = splitmix64(seed);
		return table;
	}

	constexpr KeyTable keys = make_keys();

	template <Player player, PieceType piece>
	constexpr inline Hash piece_key(Square square)
	{
		return keys.piece_square[static_cast<int>(player)][static_cast<int>(piece)][square];
	}

	inline Hash piece_key(Player player, PieceType piece, Square square)
	{
		return keys.piece_square[static_cast<int>(player)][static_cast<int>(piece)][square];
	}

	// data is GameState::data: bits 0-3 castling rights, bits 4-7 en passant file.
	constexpr inline Hash state_key(uint8_t data)
	{
		return keys.castling[data & 0x0F] ^ keys.en_passant[(data >> 4) & 0x0F];
	}

	// Mixed into a position hash to key a transposition table by (position, depth
	// remaining), since the same position can have different subtree counts at
	// different depths.
	constexpr inline Hash depth_key(int depth)
	{
		return keys.depth[depth & 0x0F];
	}

	// Combined key for a castling move: both the king's and rook's from/to squares.
	// Mirrors the exact squares make_move::castle<player, kingside>() moves.
	template <Player player, bool kingside>
	constexpr inline Hash castle_key()
	{
		if constexpr (player == Player::white)
		{
			if constexpr (kingside)
				return piece_key<player, PieceType::king>(notation_to_square("e1")) ^ piece_key<player, PieceType::king>(notation_to_square("g1")) ^ piece_key<player, PieceType::rook>(notation_to_square("h1")) ^ piece_key<player, PieceType::rook>(notation_to_square("f1"));
			else
				return piece_key<player, PieceType::king>(notation_to_square("e1")) ^ piece_key<player, PieceType::king>(notation_to_square("c1")) ^ piece_key<player, PieceType::rook>(notation_to_square("a1")) ^ piece_key<player, PieceType::rook>(notation_to_square("d1"));
		}
		else
		{
			if constexpr (kingside)
				return piece_key<player, PieceType::king>(notation_to_square("e8")) ^ piece_key<player, PieceType::king>(notation_to_square("g8")) ^ piece_key<player, PieceType::rook>(notation_to_square("h8")) ^ piece_key<player, PieceType::rook>(notation_to_square("f8"));
			else
				return piece_key<player, PieceType::king>(notation_to_square("e8")) ^ piece_key<player, PieceType::king>(notation_to_square("c8")) ^ piece_key<player, PieceType::rook>(notation_to_square("a8")) ^ piece_key<player, PieceType::rook>(notation_to_square("d8"));
		}
	}

	// Full recompute from scratch, independent of any incremental update.
	// Used to seed the initial position and to verify the incremental path.
	inline Hash compute(const Board &board, uint8_t state_data, Player side_to_move)
	{
		Hash hash = 0;
		for (int piece = 0; piece < 6; piece++)
		{
			BitBoard white_bb = board.bitboards[0][piece];
			while (white_bb)
				hash ^= piece_key(Player::white, static_cast<PieceType>(piece), pop(white_bb));
			BitBoard black_bb = board.bitboards[1][piece];
			while (black_bb)
				hash ^= piece_key(Player::black, static_cast<PieceType>(piece), pop(black_bb));
		}
		hash ^= state_key(state_data);
		if (side_to_move == Player::black)
			hash ^= keys.side_to_move;
		return hash;
	}
}

} // namespace sunsnake
