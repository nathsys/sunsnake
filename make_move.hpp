
#pragma once

// Add piece to board.
#include "defaults.hpp"
#include "chess_board.hpp"
#include "util.hpp"
#include "move.hpp"

#ifdef count_moves

namespace make_move
{

	// Move a piece.
	template <Player player>
	inline PieceType move_piece(Board &b, const Square from, const Square to)
	{
		// Find the piece type.
		const BitBoard from_mask = square_to_mask(from);
		const BitBoard to_mask = square_to_mask(to);
		const PieceType piece = b.get_piece_type(from_mask);
		*b.get_board_pointer<player>(piece) ^= from_mask;
		b.occ_board ^= from_mask;
		*b.get_board_pointer<player>(piece) |= to_mask;
		b.occ_board |= to_mask;

		if constexpr (player == Player::white)
		{
			b.w_board ^= from_mask;
			b.w_board |= to_mask;
		}
		else
		{
			b.b_board ^= from_mask;
			b.b_board |= to_mask;
		}
		b.pieces[to] = piece;
		b.pieces[from] = PieceType::none;
		return piece;
	}


	// Move a piece.
	template <Player player>
	inline void move_piece_and_check_castling(Board &b, const Square from, const Square to, GameState &state)
	{
		PieceType piece = move_piece<player>(b, from, to);

		// If the piece is a rook, we need to check if we need to remove castling rights.
		if (piece == PieceType::rook)
		{
			if (from == (player == Player::white ? notation_to_square("h1") : notation_to_square("h8")))
			{
				state.rm_cr<player, true>();
			} else if (from == (player == Player::white ? notation_to_square("a1") : notation_to_square("a8")))
			{
				state.rm_cr<player, false>();
			}
		}

		// If piece is a king, we need to remove castling rights for that player.
		if (piece == PieceType::king)
		{
			state.rm_cr<player>();
		}
	}

	template <Player player>
	inline PieceType capture(Board &b, const Square from, const Square to, GameState &state)
	{
		const PieceType moving = b.get_piece_type(from);
		const PieceType cap = b.get_piece_type(to);
		const BitBoard to_mask = square_to_mask(to);
		// Remove the captured piece from the board.
		*b.get_board_pointer<opponent<player>()>(cap) ^= to_mask;
		b.occ_board ^= to_mask;
		b.get_player_occ_ref<opponent<player>()>() ^= to_mask;
		// Move the piece.
		move_piece<player>(b, from, to);

		// If we capture a rook, we need to check if we need to remove castling rights.
		if (cap == PieceType::rook) {
			if (to == (player == Player::white ? notation_to_square("h8") : notation_to_square("h1"))) {
				state.rm_cr<opponent<player>(), true>();
			} else if (to == (player == Player::white ? notation_to_square("a8") : notation_to_square("a1"))) {
				state.rm_cr<opponent<player>(), false>();
			}
		}

		return cap;
	}

	template <Player player>
	inline void unmake_capture_move(Board &b, const Square from, const Square to, const PieceType captured)
	{
		const PieceType moving = b.get_piece_type(to);
		const BitBoard to_mask = square_to_mask(to);
		move_piece<player>(b, to, from);
		// Restore the captured piece to the board.
		*b.get_board_pointer<opponent<player>()>(captured) |= to_mask;
		b.occ_board |= to_mask;
		b.get_player_occ_ref<opponent<player>()>() |= to_mask;
		b.pieces[to] = captured;
	}

	// Do pawn double forward move. Retrurns true if en passant is possible.
	template <Player player>
	inline void pawn_double(Board &b, const Square from, const Square to, GameState &state)
	{
		move_piece<player>(b, from, to);
		// If we move pawn to a square next to a pawn, en pessant is possible for next turn.
		const BitBoard enemy_pawns = b.get_piece_board<opponent<player>(), PieceType::pawn>();
		const BitBoard en_passant_squares = player == Player::white ? c_en_passant_squares_w : c_en_passant_squares_b;
		const BitBoard to_board = square_to_mask(to);
		if (enemy_pawns & en_passant_squares & (to_board << 1 | to_board >> 1))
		{
			const uint8_t file = (to % 8) + 1;
			state.data = (state.data & 0b00001111) | (file << 4);
		}
	}

	template <Player player>
	inline void unmake_pawn_double(Board &b, Square from, Square to)
	{
		move_piece<player>(b, to, from);
	}

	template <Player player, bool kingside>
	inline void castle(Board &b)
	{
		if constexpr (player == Player::white)
		{
			if constexpr (kingside)
			{
				move_piece<Player::white>(b, notation_to_square("e1"), notation_to_square("g1"));
				move_piece<Player::white>(b, notation_to_square("h1"), notation_to_square("f1"));
			}
			else
			{
				move_piece<Player::white>(b, notation_to_square("e1"), notation_to_square("c1"));
				move_piece<Player::white>(b, notation_to_square("a1"), notation_to_square("d1"));
			}
		}
		else
		{
			if constexpr (kingside)
			{
				move_piece<Player::black>(b, notation_to_square("e8"), notation_to_square("g8"));
				move_piece<Player::black>(b, notation_to_square("h8"), notation_to_square("f8"));
			}
			else
			{
				move_piece<Player::black>(b, notation_to_square("e8"), notation_to_square("c8"));
				move_piece<Player::black>(b, notation_to_square("a8"), notation_to_square("d8"));
			}
		}
	}

	template <Player player, bool kingside>
	inline void unmake_castle(Board &b)
	{
		if constexpr (player == Player::white)
		{
			if constexpr (kingside)
			{
				move_piece<Player::white>(b, notation_to_square("g1"), notation_to_square("e1"));
				move_piece<Player::white>(b, notation_to_square("f1"), notation_to_square("h1"));
			}
			else
			{
				move_piece<Player::white>(b, notation_to_square("c1"), notation_to_square("e1"));
				move_piece<Player::white>(b, notation_to_square("d1"), notation_to_square("a1"));
			}
		}
		else
		{
			if constexpr (kingside)
			{
				move_piece<Player::black>(b, notation_to_square("g8"), notation_to_square("e8"));
				move_piece<Player::black>(b, notation_to_square("f8"), notation_to_square("h8"));
			}
			else
			{
				move_piece<Player::black>(b, notation_to_square("c8"), notation_to_square("e8"));
				move_piece<Player::black>(b, notation_to_square("d8"), notation_to_square("a8"));
			}
		}
	}

	template <Player player>
	inline void ep_capture(Board &b, const Square from, const Square to)
	{
		move_piece<player>(b, from, to);
		// Remove the captured pawn.
		const Square cap_sq = player == Player::white ? to - 8 : to + 8;
		const BitBoard cap_mask = square_to_mask(cap_sq);
		*b.get_board_pointer<opponent<player>()>(PieceType::pawn) ^= cap_mask;
		b.occ_board ^= cap_mask;
		b.get_player_occ_ref<opponent<player>()>() ^= cap_mask;
		b.pieces[cap_sq] = PieceType::none;
	}

	template <Player player>
	inline void unmake_ep_capture(Board &b, const Square from, const Square to)
	{
		move_piece<player>(b, to, from);
		// Restore the captured pawn.
		const Square cap_sq = player == Player::white ? to - 8 : to + 8;
		const BitBoard cap_mask = square_to_mask(cap_sq);
		*b.get_board_pointer<opponent<player>()>(PieceType::pawn) |= cap_mask;
		b.occ_board |= cap_mask;
		b.get_player_occ_ref<opponent<player>()>() |= cap_mask;
		b.pieces[cap_sq] = PieceType::pawn;
	}

	template <Player player, PieceType promotion_type>
	inline void promotion(Board &b, const Square from, const Square to)
	{
		move_piece<player>(b, from, to);
		// Change the piece type to the promoted piece.
		b.get_piece_board_ref<player, PieceType::pawn>() ^= square_to_mask(to);
		b.get_piece_board_ref<player, promotion_type>() |= square_to_mask(to);
		b.pieces[to] = promotion_type;
	}

	template <Player player, PieceType promotion_type>
	inline void unmake_promotion(Board &b, const Square from, const Square to)
	{
		move_piece<player>(b, to, from);
		// Change the piece type to the promoted piece.
		b.get_piece_board_ref<player, PieceType::pawn>() ^= square_to_mask(to);
		b.get_piece_board_ref<player, promotion_type>() |= square_to_mask(from);
		b.pieces[from] = PieceType::pawn;
	}

	template <Player player, PieceType promotion_type>
	inline PieceType promotion_capture(Board &b, const Square from, const Square to)
	{
		// Remove the captured piece from the board.
		const PieceType cap = b.get_piece_type(to);
		const BitBoard to_mask = square_to_mask(to);
		*b.get_board_pointer<opponent<player>()>(cap) ^= to_mask;
		b.occ_board ^= to_mask;
		b.get_player_occ_ref<opponent<player>()>() ^= to_mask;

		move_piece<player>(b, from, to);
		// Change the piece type to the promoted piece.
		b.get_piece_board_ref<player, PieceType::pawn>() ^= to_mask;
		b.get_piece_board_ref<player, promotion_type>() |= to_mask;
		b.pieces[to] = promotion_type;

		return cap;
	}

	template <Player player, PieceType promotion_type>
	inline void unmake_capture_promotion(Board &b, const Square from, const Square to, const PieceType captured)
	{
		move_piece<player>(b, to, from);
		const BitBoard from_mask = square_to_mask(from);
		const BitBoard to_mask = square_to_mask(to);
		// Change the piece type to the promoted piece.
		b.get_piece_board_ref<player, promotion_type>() ^= from_mask;
		b.get_piece_board_ref<player, PieceType::pawn>() |= from_mask;
		b.pieces[from] = PieceType::pawn;
		// Restore the captured piece to the board.
		*b.get_board_pointer<opponent<player>()>(captured) |= to_mask;
		b.occ_board |= to_mask;
		b.get_player_occ_ref<opponent<player>()>() |= to_mask;
		b.pieces[to] = captured;
	}

	template <Player player>
	void make_move(Board &board, GameState &state, Move move, FlagStack &flag_stack)
	{
		Square from = MoveEncoding::get_from_square(move);
		Square to = MoveEncoding::get_to_square(move);
		flag_stack.push(state.data); // Save the game state.
		// Clear en passant.
		state.data &= 0b00001111;

		switch (MoveEncoding::get_move_flag(move))
		{
		case MoveEncoding::quiet_flag:
			move_piece_and_check_castling<player>(board, from, to, state);
			break;
		case MoveEncoding::double_pawn_push_flag:
			pawn_double<player>(board, from, to, state);
			break;
		case MoveEncoding::kingside_castle_flag:
			castle<player, true>(board);
			state.rm_cr<player>();
			break;
		case MoveEncoding::queenside_castle_flag:
			castle<player, false>(board);
			state.rm_cr<player>();
			break;
		case MoveEncoding::capture_flag:
			flag_stack.push(static_cast<u_int8_t>(capture<player>(board, from, to, state)));
			break;
		case MoveEncoding::ep_capture_flag:
			ep_capture<player>(board, from, to);
			break;
		case MoveEncoding::knight_promotion_flag:
			promotion<player, PieceType::knight>(board, from, to);
			break;
		case MoveEncoding::bishop_promotion_flag:
			promotion<player, PieceType::bishop>(board, from, to);
			break;
		case MoveEncoding::rook_promotion_flag:
			promotion<player, PieceType::rook>(board, from, to);
			break;
		case MoveEncoding::queen_promotion_flag:
			promotion<player, PieceType::queen>(board, from, to);
			break;
		case MoveEncoding::knight_promotion_capture_flag:
			flag_stack.push(static_cast<uint8_t>(promotion_capture<player, PieceType::knight>(board, from, to)));
			break;
		case MoveEncoding::bishop_promotion_capture_flag:
			flag_stack.push(static_cast<uint8_t>(promotion_capture<player, PieceType::bishop>(board, from, to)));
			break;
		case MoveEncoding::rook_promotion_capture_flag:
			flag_stack.push(static_cast<uint8_t>(promotion_capture<player, PieceType::rook>(board, from, to)));
			break;
		case MoveEncoding::queen_promotion_capture_flag:
			flag_stack.push(static_cast<uint8_t>(promotion_capture<player, PieceType::queen>(board, from, to)));
			break;
		}

		state.switch_player();
	}

	template <Player player>
	void unmake_move(Board &board, GameState &state, Move move, FlagStack &flag_stack)
	{
		Square from = MoveEncoding::get_from_square(move);
		Square to = MoveEncoding::get_to_square(move);

		switch (MoveEncoding::get_move_flag(move))
		{
		case MoveEncoding::quiet_flag:
			move_piece<player>(board, to, from);
			break;
		case MoveEncoding::double_pawn_push_flag:
			unmake_pawn_double<player>(board, from, to);
			break;
		case MoveEncoding::kingside_castle_flag:
			unmake_castle<player, true>(board);
			break;
		case MoveEncoding::queenside_castle_flag:
			unmake_castle<player, false>(board);
			break;
		case MoveEncoding::capture_flag:
			unmake_capture_move<player>(board, from, to, static_cast<PieceType>(flag_stack.pop()));
			break;
		case MoveEncoding::ep_capture_flag:
			unmake_ep_capture<player>(board, from, to);
			break;
		case MoveEncoding::knight_promotion_flag:
			unmake_promotion<player, PieceType::knight>(board, from, to);
			break;
		case MoveEncoding::bishop_promotion_flag:
			unmake_promotion<player, PieceType::bishop>(board, from, to);
			break;
		case MoveEncoding::rook_promotion_flag:
			unmake_promotion<player, PieceType::rook>(board, from, to);
			break;
		case MoveEncoding::queen_promotion_flag:
			unmake_promotion<player, PieceType::queen>(board, from, to);
			break;
		case MoveEncoding::knight_promotion_capture_flag:
			unmake_capture_promotion<player, PieceType::knight>(board, from, to, static_cast<PieceType>(flag_stack.pop()));
			break;
		case MoveEncoding::bishop_promotion_capture_flag:
			unmake_capture_promotion<player, PieceType::bishop>(board, from, to, static_cast<PieceType>(flag_stack.pop()));
			break;
		case MoveEncoding::rook_promotion_capture_flag:
			unmake_capture_promotion<player, PieceType::rook>(board, from, to, static_cast<PieceType>(flag_stack.pop()));
			break;
		case MoveEncoding::queen_promotion_capture_flag:
			unmake_capture_promotion<player, PieceType::queen>(board, from, to, static_cast<PieceType>(flag_stack.pop()));
			break;
		}

		state.data = flag_stack.pop(); // Restore the game state data.
		state.switch_player();
	}
} // namespace make_move

#endif // count_moves