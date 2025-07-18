#include "defaults.hpp"

#pragma once

struct Board {
    // Store the white pieces state.
	BitBoard w_pawn = INIT_PAWN_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_king = INIT_KING_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_rook = INIT_ROOK_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_bishop = INIT_BISHOP_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_knight = INIT_KNIGHT_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_queen = INIT_QUEEN_SQUARES & INIT_WHITE_PIECES;

    // Store the black pieces state.
	BitBoard b_pawn = INIT_PAWN_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_king = INIT_KING_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_rook = INIT_ROOK_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_bishop = INIT_BISHOP_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_knight = INIT_KNIGHT_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_queen = INIT_QUEEN_SQUARES & INIT_BLACK_PIECES;

    // Occupancy boards.
	BitBoard w_board = INIT_WHITE_PIECES;
	BitBoard b_board = INIT_BLACK_PIECES;
	BitBoard occ_board = INIT_TOTAL_SQUARES;
};