#include "defaults.hpp"
#include "chess_board.hpp"
#include "util.hpp"
#include "dynamic_tables.hpp"

#pragma once

struct MaskSet {
	// squares that are empty or enemy;
	BitBoard legal_squares;

	// Pinmask.
	BitBoard pinmask_dg = 0;
	BitBoard pinmask_orth = 0;
	BitBoard check_mask = 0;
	BitBoard nopin = 0;

	int checkers = 0;

	inline int get_check_cnt() { return checkers; }

	// Create all the needed masks for the current position.
	template <Player player>
	void create_masks(Board& b, Square king_square) {

		// Reset all the masks.
		pinmask_dg = 0;
		pinmask_orth = 0;
		check_mask = 0;
		checkers = 0;

		BitBoard opp_board = b.get_player_occ<player>();
		BitBoard own_board = b.get_player_occ<player>();

		// Player can move to all squares that are not occupied by own pieces.
		legal_squares = !own_board;

		// Get opponent sliders.
		BitBoard eq = b.get_piece_board<opponent<player>(), PieceType::queen>();
		BitBoard eb = b.get_piece_board<opponent<player>(), PieceType::bishop>();
		BitBoard er = b.get_piece_board<opponent<player>(), PieceType::rook>();
		
		// Get the current pinners.
		BitBoard diag_pinners = get_bishop_move(king_square, opp_board) & (eb | eq);
		BitBoard orth_pinners = get_rook_move(king_square, opp_board) & (er | eq);

		// Get knight and pawn checkers.
		BitBoard k_checkers = get_knight_move(king_square) & b.get_piece_board<opponent<player>(), PieceType::knight>();
		BitBoard p_checkers = get_pawn_diags<player>(square_to_mask(king_square)) & b.get_piece_board<opponent<player>(), PieceType::pawn>();

		if (k_checkers) {
			check_mask |= k_checkers;
			checkers++;
		} else if (p_checkers) {
			check_mask |= p_checkers;
			checkers++;
		}

		// Checks how many pieces are between the king and the pinner.
		// If there are no pieces, then the king is not pinned.
		auto process_pinners = [&](BitBoard& pinboard, BitBoard& goal_mask) {
			while (pinboard) {
				Square src = pop(pinboard);
				BitBoard between = Lookup::between_squares[king_square][src];

				if(popcnt(between & own_board)) {
					goal_mask |= between;
				} else {
					check_mask |= between;
					checkers++;
				}
			}
		};

		process_pinners(diag_pinners, pinmask_dg);
		process_pinners(orth_pinners, pinmask_orth);
		nopin = ~(pinmask_dg | pinmask_orth);
	}
};