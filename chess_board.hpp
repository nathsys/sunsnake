#include "defaults.hpp"
#include "move_logic.hpp"
#include "util.hpp"
#include <cstddef>

#pragma once

struct Board
{
	// Occupancy boards.
	BitBoard w_board = c_init_white_pieces;
	BitBoard b_board = c_init_black_pieces;
	BitBoard occ_board = c_init_total_squares;

	PieceType pieces[64] = {
		PieceType::rook, PieceType::knight, PieceType::bishop, PieceType::king, PieceType::queen, PieceType::bishop, PieceType::knight, PieceType::rook,
		PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn,
		PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none,
		PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none,
		PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none,
		PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none, PieceType::none,
		PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn, PieceType::pawn,
		PieceType::rook, PieceType::knight, PieceType::bishop, PieceType::king, PieceType::queen, PieceType::bishop, PieceType::knight, PieceType::rook};

	// Individual piece boards.
	BitBoard bitboards[2][6] = {
		{c_init_pawn_squares & c_init_white_pieces,	 // white pawn
		 c_init_knight_squares &c_init_white_pieces, // white knight
		 c_init_bishop_squares &c_init_white_pieces, // white bishop
		 c_init_rook_squares &c_init_white_pieces,	 // white rook
		 c_init_queen_squares &c_init_white_pieces,	 // white queen
		 c_init_king_squares &c_init_white_pieces},	 // white king
		{c_init_pawn_squares & c_init_black_pieces,	 // black pawn
		 c_init_knight_squares &c_init_black_pieces, // black knight
		 c_init_bishop_squares &c_init_black_pieces, // black bishop
		 c_init_rook_squares &c_init_black_pieces,	 // black rook
		 c_init_queen_squares &c_init_black_pieces,	 // black queen
		 c_init_king_squares &c_init_black_pieces}}; // black king

	template <Player player>
	constexpr inline BitBoard get_player_occ() const
	{
		if constexpr (player == Player::white)
		{
			return w_board;
		}
		else
		{
			return b_board;
		}
	}

	template <Player player>
	constexpr inline BitBoard &get_player_occ_ref()
	{
		if constexpr (player == Player::white)
		{
			return w_board;
		}
		else
		{
			return b_board;
		}
	}

	template <Player player, PieceType piece>
	constexpr inline BitBoard get_piece_board() const
	{
		return bitboards[static_cast<int>(player)][static_cast<int>(piece)];
	}

	template <Player player, PieceType piece>
	constexpr inline BitBoard &get_piece_board_ref()
	{
		return bitboards[static_cast<int>(player)][static_cast<int>(piece)];
	}

	template <Player player>
	inline bool is_attacked(BitBoard pos) const
	{
		Square square = lbit_from_right(pos);
		return (get_pawn_attacks<player>(pos) & get_piece_board<opponent<player>(), PieceType::pawn>()) || (get_knight_move(square) & get_piece_board<opponent<player>(), PieceType::knight>()) || (get_rook_move(square, occ_board) & (get_piece_board<opponent<player>(), PieceType::rook>() | get_piece_board<opponent<player>(), PieceType::queen>())) || (get_bishop_move(square, occ_board) & (get_piece_board<opponent<player>(), PieceType::bishop>() | get_piece_board<opponent<player>(), PieceType::queen>())) || (get_king_move(square) & get_piece_board<opponent<player>(), PieceType::king>());
	}

	template <Player player, bool kingside>
	inline bool castle_sqs_attacked() const
	{
		if constexpr (kingside)
		{
			if constexpr (player == Player::white)
				return is_attacked<player>(c_castle_k_squares_w1) || is_attacked<player>(c_castle_k_squares_w2);
			else
				return is_attacked<player>(c_castle_k_squares_b1) || is_attacked<player>(c_castle_k_squares_b2);
		}
		else
		{
			if constexpr (player == Player::white)
				return is_attacked<player>(c_castle_q_squares_w1) || is_attacked<player>(c_castle_q_squares_w2);
			else
				return is_attacked<player>(c_castle_q_squares_b1) || is_attacked<player>(c_castle_q_squares_b2);
		}
	}

	template <Player player, bool debug = false>
	inline bool is_attacked_by_slider(BitBoard pos) const
	{
		const Square square = lbit_from_right(pos);
		const BitBoard rook_attackers = get_rook_move(square, occ_board) & get_piece_board<opponent<player>(), PieceType::rook>();
		const BitBoard queen_rook_attackers = get_rook_move(square, occ_board) & get_piece_board<opponent<player>(), PieceType::queen>();
		const BitBoard bishop_attackers = get_bishop_move(square, occ_board) & get_piece_board<opponent<player>(), PieceType::bishop>();
		const BitBoard queen_bishop_attackers = get_bishop_move(square, occ_board) & get_piece_board<opponent<player>(), PieceType::queen>();
		const bool attacked = rook_attackers || queen_rook_attackers || bishop_attackers || queen_bishop_attackers;
		if constexpr (debug)
		{
			std::cout << "Checking if square " << (int)square << " is attacked by slider of player " << (player == Player::white ? "white" : "black") << "\n";
			if (rook_attackers)
			{
				Square attacker_sq = lbit_from_right(rook_attackers);
				std::cout << "King attacked by rook at " << (int)attacker_sq << "\n";
				print_bitboard(rook_attackers);
				std::cout << "Rook move board for attacker:" << std::endl;
				print_bitboard(get_rook_move(attacker_sq, occ_board));
			}
			if (queen_rook_attackers)
			{
				Square attacker_sq = lbit_from_right(queen_rook_attackers);
				std::cout << "King attacked by queen (rook direction) at " << (int)attacker_sq << "\n";
				print_bitboard(queen_rook_attackers);
				std::cout << "Queen (rook direction) move board for attacker:" << std::endl;
				print_bitboard(get_rook_move(attacker_sq, occ_board));
			}
			if (bishop_attackers)
			{
				Square attacker_sq = lbit_from_right(bishop_attackers);
				std::cout << "King attacked by bishop at " << (int)attacker_sq << "\n";
				print_bitboard(bishop_attackers);
				std::cout << "Bishop move board for attacker:" << std::endl;
				print_bitboard(get_bishop_move(attacker_sq, occ_board));
			}
			if (queen_bishop_attackers)
			{
				Square attacker_sq = lbit_from_right(queen_bishop_attackers);
				std::cout << "King attacked by queen (bishop direction) at " << (int)attacker_sq << "\n";
				print_bitboard(queen_bishop_attackers);
				std::cout << "Queen (bishop direction) move board for attacker:" << std::endl;
				print_bitboard(get_bishop_move(attacker_sq, occ_board));
			}
		}
		return attacked;
	}

	template <Player player>
	inline bool is_attacked(Square pos) const
	{
		BitBoard mask = square_to_mask(pos);
		return (get_pawn_attacks<player>(mask) & get_piece_board<opponent<player>(), PieceType::pawn>()) || (get_knight_move(pos) & get_piece_board<opponent<player>(), PieceType::knight>()) || (get_rook_move(pos, occ_board) & (get_piece_board<opponent<player>(), PieceType::rook>() | get_piece_board<opponent<player>(), PieceType::queen>())) || (get_bishop_move(pos, occ_board) & (get_piece_board<opponent<player>(), PieceType::bishop>() | get_piece_board<opponent<player>(), PieceType::queen>())) || (get_king_move(pos) & get_piece_board<opponent<player>(), PieceType::king>());
	}

	template <Player player, PieceType piece>
	inline BitBoard *get_board_pointer()
	{
		return &bitboards[static_cast<int>(player)][static_cast<int>(piece)];
	}

	template <Player player>
	inline BitBoard *get_board_pointer(PieceType piece)
	{
		return &bitboards[static_cast<int>(player)][static_cast<int>(piece)];
	}

	template <Player player>
	inline const BitBoard *get_board_pointer(PieceType piece) const
	{
		return &bitboards[static_cast<int>(player)][static_cast<int>(piece)];
	}

	inline PieceType get_piece_type(BitBoard mask) const
	{
		return pieces[lbit_from_right(mask)];
	}

	inline PieceType get_piece_type(Square mask) const
	{
		return pieces[mask];
	}

	inline void print_board() const
	{
		std::cout << "   a b c d e f g h\n";
		std::cout << " +-----------------+\n";
		for (int row = 7; row >= 0; row--)
		{
			std::cout << row + 1 << "| ";
			for (int col = 7; col >= 0; col--)
			{
				Square square = row * 8 + col;
				BitBoard mask = square_to_mask(square);
				char piece = ' ';
				bool is_white = w_board & mask;
				bool is_black = b_board & mask;
				PieceType type = get_piece_type(mask);

				switch (type)
				{
				case PieceType::king:
					piece = 'K';
					break;
				case PieceType::queen:
					piece = 'Q';
					break;
				case PieceType::rook:
					piece = 'R';
					break;
				case PieceType::bishop:
					piece = 'B';
					break;
				case PieceType::knight:
					piece = 'N';
					break;
				case PieceType::pawn:
					piece = 'P';
					break;
				default:
					piece = ' ';
					break;
				}
				if (is_black)
					piece = tolower(piece);
				if (!is_white && !is_black)
					piece = ' ';
				std::cout << piece << ' ';
			}
			std::cout << "|" << row + 1 << "\n";
		}
		std::cout << " +-----------------+\n";
		std::cout << "   a b c d e f g h" << std::endl;
	}
};