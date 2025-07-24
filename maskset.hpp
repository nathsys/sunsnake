#include "defaults.hpp"

#pragma once

struct MaskSet {
	// squares that are empty or enemy;
	BitBoard can_move_to;

	// Pinmask.
	BitBoard pinmask_dg = 0;
	BitBoard pinmask_orth = 0;
	BitBoard check_mask = 0;
	BitBoard unpinned = 0;
	BitBoard king_cmt;

	uint8_t checkers = 0;

	inline uint8_t get_check_cnt() { return checkers; }

	inline void reset() {
		pinmask_dg = 0;
		pinmask_orth = 0;
		check_mask = 0;
		checkers = 0;
	}
};