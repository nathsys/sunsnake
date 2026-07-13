
#pragma once

// Add piece to board.
#include "defaults.hpp"
#include "chess_board.hpp"
#include "util.hpp"
#include "move.hpp"
#include "zobrist.hpp"

#ifdef move_gen

namespace sunsnake
{

namespace make_move
{

	// Move a piece.
	template <Player player>
	inline PieceType move_piece(Board &b, const Square from, const Square to)
	{
		// Find the piece type.
		const BitBoard from_mask = square_to_mask(from);
		const BitBoard to_mask = square_to_mask(to);
		const BitBoard move_mask = from_mask | to_mask;
		const PieceType piece = b.get_piece_type(from_mask);
		*b.get_board_pointer<player>(piece) ^= move_mask;
		b.occ_board ^= move_mask;

		if constexpr (player == Player::white)
		{
			b.w_board ^= move_mask;
		}
		else
		{
			b.b_board ^= move_mask;
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
		const PieceType cap = b.get_piece_type(to);
		const BitBoard to_mask = square_to_mask(to);
		// Remove the captured piece from the board.
		*b.get_board_pointer<opponent<player>()>(cap) ^= to_mask;
		b.occ_board ^= to_mask;
		b.get_player_occ_ref<opponent<player>()>() ^= to_mask;
		// Move the piece. Also revokes the capturing player's own castling
		// rights if the capturing piece is a king or a rook leaving its home square.
		move_piece_and_check_castling<player>(b, from, to, state);

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
		b.get_piece_board_ref<player, promotion_type>() ^= square_to_mask(from);
		b.get_piece_board_ref<player, PieceType::pawn>() |= square_to_mask(from);
		b.pieces[from] = PieceType::pawn;
	}

	template <Player player, PieceType promotion_type>
	inline PieceType promotion_capture(Board &b, const Square from, const Square to, GameState &state)
	{
		// Remove the captured piece from the board.
		const PieceType cap = b.get_piece_type(to);
		const BitBoard to_mask = square_to_mask(to);
		*b.get_board_pointer<opponent<player>()>(cap) ^= to_mask;
		b.occ_board ^= to_mask;
		b.get_player_occ_ref<opponent<player>()>() ^= to_mask;

		// If we capture a rook, we need to check if we need to remove castling rights.
		if (cap == PieceType::rook) {
			if (to == (player == Player::white ? notation_to_square("h8") : notation_to_square("h1"))) {
				state.rm_cr<opponent<player>(), true>();
			} else if (to == (player == Player::white ? notation_to_square("a8") : notation_to_square("a1"))) {
				state.rm_cr<opponent<player>(), false>();
			}
		}

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
		const uint8_t old_data = state.data;
		// Clear en passant.
		state.data &= 0b00001111;

		switch (MoveEncoding::get_move_flag(move))
		{
		case MoveEncoding::quiet_flag:
		{
			const PieceType moved = board.get_piece_type(from);
			state.hash ^= Zobrist::piece_key(player, moved, from) ^ Zobrist::piece_key(player, moved, to);
			move_piece_and_check_castling<player>(board, from, to, state);
			break;
		}
		case MoveEncoding::double_pawn_push_flag:
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::pawn>(to);
			pawn_double<player>(board, from, to, state);
			break;
		case MoveEncoding::kingside_castle_flag:
			state.hash ^= Zobrist::castle_key<player, true>();
			castle<player, true>(board);
			state.rm_cr<player>();
			break;
		case MoveEncoding::queenside_castle_flag:
			state.hash ^= Zobrist::castle_key<player, false>();
			castle<player, false>(board);
			state.rm_cr<player>();
			break;
		case MoveEncoding::capture_flag:
		{
			const PieceType moved = board.get_piece_type(from);
			state.hash ^= Zobrist::piece_key(player, moved, from) ^ Zobrist::piece_key(player, moved, to);
			const PieceType captured = capture<player>(board, from, to, state);
			state.hash ^= Zobrist::piece_key(opponent<player>(), captured, to);
			flag_stack.push(static_cast<u_int8_t>(captured));
			break;
		}
		case MoveEncoding::ep_capture_flag:
		{
			const Square cap_sq = player == Player::white ? to - 8 : to + 8;
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::pawn>(to) ^ Zobrist::piece_key<opponent<player>(), PieceType::pawn>(cap_sq);
			ep_capture<player>(board, from, to);
			break;
		}
		case MoveEncoding::knight_promotion_flag:
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::knight>(to);
			promotion<player, PieceType::knight>(board, from, to);
			break;
		case MoveEncoding::bishop_promotion_flag:
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::bishop>(to);
			promotion<player, PieceType::bishop>(board, from, to);
			break;
		case MoveEncoding::rook_promotion_flag:
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::rook>(to);
			promotion<player, PieceType::rook>(board, from, to);
			break;
		case MoveEncoding::queen_promotion_flag:
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::queen>(to);
			promotion<player, PieceType::queen>(board, from, to);
			break;
		case MoveEncoding::knight_promotion_capture_flag:
		{
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::knight>(to);
			const PieceType captured = promotion_capture<player, PieceType::knight>(board, from, to, state);
			state.hash ^= Zobrist::piece_key(opponent<player>(), captured, to);
			flag_stack.push(static_cast<uint8_t>(captured));
			break;
		}
		case MoveEncoding::bishop_promotion_capture_flag:
		{
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::bishop>(to);
			const PieceType captured = promotion_capture<player, PieceType::bishop>(board, from, to, state);
			state.hash ^= Zobrist::piece_key(opponent<player>(), captured, to);
			flag_stack.push(static_cast<uint8_t>(captured));
			break;
		}
		case MoveEncoding::rook_promotion_capture_flag:
		{
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::rook>(to);
			const PieceType captured = promotion_capture<player, PieceType::rook>(board, from, to, state);
			state.hash ^= Zobrist::piece_key(opponent<player>(), captured, to);
			flag_stack.push(static_cast<uint8_t>(captured));
			break;
		}
		case MoveEncoding::queen_promotion_capture_flag:
		{
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::queen>(to);
			const PieceType captured = promotion_capture<player, PieceType::queen>(board, from, to, state);
			state.hash ^= Zobrist::piece_key(opponent<player>(), captured, to);
			flag_stack.push(static_cast<uint8_t>(captured));
			break;
		}
		}

		state.hash ^= Zobrist::state_key(old_data) ^ Zobrist::state_key(state.data) ^ Zobrist::keys.side_to_move;
		state.switch_player();
	}

	template <Player player>
	void unmake_move(Board &board, GameState &state, Move move, FlagStack &flag_stack)
	{
		Square from = MoveEncoding::get_from_square(move);
		Square to = MoveEncoding::get_to_square(move);
		const uint8_t data_before_restore = state.data;

		switch (MoveEncoding::get_move_flag(move))
		{
		case MoveEncoding::quiet_flag:
		{
			const PieceType moved = board.get_piece_type(to);
			state.hash ^= Zobrist::piece_key(player, moved, from) ^ Zobrist::piece_key(player, moved, to);
			move_piece<player>(board, to, from);
			break;
		}
		case MoveEncoding::double_pawn_push_flag:
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::pawn>(to);
			unmake_pawn_double<player>(board, from, to);
			break;
		case MoveEncoding::kingside_castle_flag:
			state.hash ^= Zobrist::castle_key<player, true>();
			unmake_castle<player, true>(board);
			break;
		case MoveEncoding::queenside_castle_flag:
			state.hash ^= Zobrist::castle_key<player, false>();
			unmake_castle<player, false>(board);
			break;
		case MoveEncoding::capture_flag:
		{
			const PieceType moved = board.get_piece_type(to);
			const PieceType captured = static_cast<PieceType>(flag_stack.pop());
			state.hash ^= Zobrist::piece_key(player, moved, from) ^ Zobrist::piece_key(player, moved, to) ^ Zobrist::piece_key(opponent<player>(), captured, to);
			unmake_capture_move<player>(board, from, to, captured);
			break;
		}
		case MoveEncoding::ep_capture_flag:
		{
			const Square cap_sq = player == Player::white ? to - 8 : to + 8;
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::pawn>(to) ^ Zobrist::piece_key<opponent<player>(), PieceType::pawn>(cap_sq);
			unmake_ep_capture<player>(board, from, to);
			break;
		}
		case MoveEncoding::knight_promotion_flag:
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::knight>(to);
			unmake_promotion<player, PieceType::knight>(board, from, to);
			break;
		case MoveEncoding::bishop_promotion_flag:
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::bishop>(to);
			unmake_promotion<player, PieceType::bishop>(board, from, to);
			break;
		case MoveEncoding::rook_promotion_flag:
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::rook>(to);
			unmake_promotion<player, PieceType::rook>(board, from, to);
			break;
		case MoveEncoding::queen_promotion_flag:
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::queen>(to);
			unmake_promotion<player, PieceType::queen>(board, from, to);
			break;
		case MoveEncoding::knight_promotion_capture_flag:
		{
			const PieceType captured = static_cast<PieceType>(flag_stack.pop());
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::knight>(to) ^ Zobrist::piece_key(opponent<player>(), captured, to);
			unmake_capture_promotion<player, PieceType::knight>(board, from, to, captured);
			break;
		}
		case MoveEncoding::bishop_promotion_capture_flag:
		{
			const PieceType captured = static_cast<PieceType>(flag_stack.pop());
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::bishop>(to) ^ Zobrist::piece_key(opponent<player>(), captured, to);
			unmake_capture_promotion<player, PieceType::bishop>(board, from, to, captured);
			break;
		}
		case MoveEncoding::rook_promotion_capture_flag:
		{
			const PieceType captured = static_cast<PieceType>(flag_stack.pop());
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::rook>(to) ^ Zobrist::piece_key(opponent<player>(), captured, to);
			unmake_capture_promotion<player, PieceType::rook>(board, from, to, captured);
			break;
		}
		case MoveEncoding::queen_promotion_capture_flag:
		{
			const PieceType captured = static_cast<PieceType>(flag_stack.pop());
			state.hash ^= Zobrist::piece_key<player, PieceType::pawn>(from) ^ Zobrist::piece_key<player, PieceType::queen>(to) ^ Zobrist::piece_key(opponent<player>(), captured, to);
			unmake_capture_promotion<player, PieceType::queen>(board, from, to, captured);
			break;
		}
		}

		state.data = flag_stack.pop(); // Restore the game state data.
		state.hash ^= Zobrist::state_key(data_before_restore) ^ Zobrist::state_key(state.data) ^ Zobrist::keys.side_to_move;
		state.switch_player();
	}

	// Runtime dispatch for callers holding a GameState rather than a
	// compile-time Player.
	inline void make_move(Board &board, GameState &state, Move move, FlagStack &flag_stack)
	{
		if (state.player() == Player::white)
			make_move<Player::white>(board, state, move, flag_stack);
		else
			make_move<Player::black>(board, state, move, flag_stack);
	}

	// state.player() has already been flipped by make_move() to the side now
	// to move, so the side that made the move is its opponent.
	inline void unmake_move(Board &board, GameState &state, Move move, FlagStack &flag_stack)
	{
		if (state.player() == Player::white)
			unmake_move<Player::black>(board, state, move, flag_stack);
		else
			unmake_move<Player::white>(board, state, move, flag_stack);
	}
} // namespace make_move

} // namespace sunsnake

#endif // move_gen