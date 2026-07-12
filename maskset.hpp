#include "defaults.hpp"
#include "chess_board.hpp"
#include "util.hpp"
#include "dynamic_tables.hpp"

#pragma once

struct MaskSet
{
	// squares that are empty or enemy;
	BitBoard legal_squares;

	// Pinmask.
	BitBoard pinmask_dg = 0;
	BitBoard pinmask_orth = 0;
	BitBoard check_mask = 0;
	BitBoard nopin = 0;
	BitBoard en_passant = 0;

	int checkers = 0;

	inline int get_check_cnt() { return checkers; }

	// Create all the needed masks for the current position.
	template <Player player>
	void create_masks(const Board &b)
	{
		// Reset all the masks.
		pinmask_dg = 0;
		pinmask_orth = 0;
		check_mask = 0;
		checkers = 0;

		const Square king_square = lbr(b.get_piece_board<player, PieceType::king>());

		const BitBoard opp_board = b.get_player_occ<opponent<player>()>();
		const BitBoard own_board = b.get_player_occ<player>();

		// Player can move to all squares that are not occupied by own pieces.
		legal_squares = ~own_board;

		// Get opponent sliders.
		const BitBoard eq = b.get_piece_board<opponent<player>(), PieceType::queen>();
		const BitBoard eb = b.get_piece_board<opponent<player>(), PieceType::bishop>();
		const BitBoard er = b.get_piece_board<opponent<player>(), PieceType::rook>();

		// Get the current pinners.
		BitBoard diag_pinners = get_bishop_move(king_square, opp_board) & (eb | eq);
		BitBoard orth_pinners = get_rook_move(king_square, opp_board) & (er | eq);

		// Get knight and pawn checkers.
		const BitBoard k_checkers = get_knight_move(king_square) & b.get_piece_board<opponent<player>(), PieceType::knight>();
		const BitBoard p_checkers = get_pawn_attacks<player>(b.get_piece_board<player, PieceType::king>()) & b.get_piece_board<opponent<player>(), PieceType::pawn>();
		const BitBoard checker_boards = k_checkers | p_checkers;

		if (checker_boards)
		{
			check_mask |= checker_boards;
			checkers = pcnt(checker_boards);
		}

		// Checks how many pieces are between the king and the pinner.
		// If there are no pieces, then the king is not pinned.
		auto process_pinners = [&](BitBoard &pinboard, BitBoard &goal_mask)
		{
			while (pinboard)
			{
				const Square src = pop(pinboard);
				const BitBoard between = Lookup::between_squares[king_square][src];

				// Check if we have pieces between the king and the pinner.
				switch (pcnt(between & own_board))
				{
				case 0:
					check_mask |= between;
					checkers++;
					break;
				case 1:
					goal_mask |= between;
					break;
				}
			}
		};

		process_pinners(diag_pinners, pinmask_dg);
		process_pinners(orth_pinners, pinmask_orth);
		nopin = ~(pinmask_dg | pinmask_orth);
	}
};