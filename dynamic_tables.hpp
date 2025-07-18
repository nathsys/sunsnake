#include "defaults.hpp"
#include "util.hpp"
#include "static_tables.hpp"

#include <array>
#include <cstdint>

#pragma once

namespace Lookup {
// *= Rook and bishop attack boards on fly. *=
// Bishop moving logic.
constexpr uint64_t bishop_attack_on_fly(Square square, BitBoard occupation) {
	uint64_t move_board = 0b0;

	int square_copy = square;
	uint64_t bit_mask = 1ULL << (square);
	// Continue until another piece is found. Repeat for each direction.
	// Up left.
	while (square_copy < 64 && square_copy % 8 != 7) {
		bit_mask <<= 9;
		square_copy += 9;
		move_board |= bit_mask;
		if (occupation & bit_mask) break;
	}

	square_copy = square;
	bit_mask = 1ULL << (square);
	// Move up-right direction.
	while (square_copy < 64 && square_copy % 8 != 0) {
		bit_mask <<= 7;
		square_copy += 7;
		move_board |= bit_mask;
		if (occupation & bit_mask) break; 
	}

	square_copy = square;
	bit_mask = 1ULL << (square);

	// Move down-right direction.
	while (square_copy >= 0 && square_copy % 8 != 0) {
		bit_mask >>= 9;
		square_copy -= 9;
		move_board |= bit_mask;
		if (occupation & bit_mask) break;  
	}

	square_copy = square;
	bit_mask = 1ULL << (square);

	// Move down-left direction.
	while (square_copy >= 0 && square_copy % 8 != 7) {
		bit_mask >>= 7;
		square_copy -= 7;
		move_board |= bit_mask;
		if (occupation & bit_mask) break;
	}

	return move_board;
}

// Rook move logic.
constexpr BitBoard rook_attack_on_fly(Square square, BitBoard occ) {
	BitBoard atk = 0ULL;
	int rank = square/8, file = square%8, r, f;
	for(r = rank+1; r <= 7 && !(occ & (1ULL << (file + 8*r)));) 
	 atk |= (1ULL << (file + 8*r++));
	for(r = rank-1; r >= 0 && !(occ & (1ULL << (file + 8*r)));)
	 atk |= (1ULL << (file + 8*r--));
	for(f = file+1; f <= 7 && !(occ & (1ULL << (f + 8*rank)));)
	 atk |= (1ULL << (f++ + 8*rank));
	for(f = file-1; f >= 0 && !(occ & (1ULL << (f + 8*rank)));)
	 atk |= (1ULL << (f-- + 8*rank));
	return atk;
}


// *= PEXT *=
#ifdef _pdep_u64
// Array sizes for pext tables.
const uint32_t rook_pext_size = 102400;
const uint32_t bishop_pext_size = 5248;

// Rook offsets.
static std::array<BitBoard, 64> init_rook_pext_offset() {
	std::array<BitBoard, 64> ret;
	uint32_t offset = 0;
	for (Square s = 0; s < 64; s++) {
		uint32_t perm_amount = 1 << pcnt(rook_mask_table[s]);
		ret[s] = offset;
		offset += perm_amount;
	}

	return ret;
}

static const std::array<BitBoard, 64> rook_pext_offset = init_rook_pext_offset();

// Bishop offsets.
static std::array<BitBoard, 64> init_bishop_pext_offset() {
	std::array<BitBoard, 64> ret;
	uint32_t offset = 0;
	for (Square s = 0; s < 64; s++) {
		uint32_t perm_amount = 1 << pcnt(bishop_mask_table[s]);
		ret[s] = offset;
		offset += perm_amount;
	}

	return ret;
}

static const std::array<BitBoard, 64> bishop_pext_offset = init_bishop_pext_offset();

// Bishop attacks.
static std::array<BitBoard, bishop_pext_size> init_bishop_pext_atk() {
	std::array<BitBoard, bishop_pext_size> ret{};
	uint32_t offset = 0;
	for (Square s = 0; s < 64; ++s) {
		uint32_t mask_bits = pcnt(bishop_mask_table[s]);
		uint32_t perm_amount = 1 << mask_bits;

		for (uint32_t perm = 0; perm < perm_amount; ++perm) {
			BitBoard blocker = _pdep_u64(static_cast<uint64_t>(perm), bishop_mask_table[s]);
			ret[offset + perm] = bishop_attack_on_fly(s, blocker);
		}
		offset += perm_amount;
	}

	return ret;
}
static const std::array<BitBoard, bishop_pext_size> bishop_pext_atk = init_bishop_pext_atk();

// Rook attacks.
static std::array<BitBoard, rook_pext_size> init_rook_pext_atk() {
	std::array<BitBoard, rook_pext_size> ret{};
	uint32_t offset = 0;
	for (Square s = 0; s < 64; ++s) {
		uint32_t mask_bits = pcnt(rook_mask_table[s]);
		uint32_t perm_amount = 1 << mask_bits;

		for (uint32_t perm = 0; perm < perm_amount; ++perm) {
			BitBoard blocker = _pdep_u64(static_cast<uint64_t>(perm), rook_mask_table[s]);
			ret[offset + perm] = rook_attack_on_fly(s, blocker);
		}
		offset += perm_amount;
	}

	return ret;
}
static const std::array<BitBoard, rook_pext_size> rook_pext_atk = init_rook_pext_atk();
#else 
// If pext instruction is not available, use magic bitboard method.

// *= Bishop and rook masks *=

// Makes an attack mask from given square by making an attack board with an empty occupation board.
// A valid slider mask contains the reachable squares without the board edges.
template <bool diagonal>
constexpr BitBoard make_slider_mask(Square s) {
    BitBoard mask = diagonal ? bishop_attack_on_fly(s, EMPTY_BOARD) : rook_attack_on_fly(s, EMPTY_BOARD);
	return MASK_SQUARES & mask & ~square_to_mask(s);
}

// Make lookup tables for the attack masks.
template<bool diagonal>
constexpr std::array<uint64_t, 64> make_mask_table() {
	std::array<uint64_t, 64> values{};
	for (int square = 0; square < 64; square++) 
		values[square] = make_slider_mask<diagonal>(square);
	return values;
}

// Slider mask table instances.
const auto bishop_masks = make_mask_table<true>();
const auto rook_masks = make_mask_table<false>();

template <int max_perms>
constexpr std::array<BitBoard, max_perms> create_all_perms(BitBoard movement_mask) {
    std::array<BitBoard, max_perms> blocker_boards{};
    int move_count = pcnt(movement_mask);
    int perm_amount = 1 << move_count;
    
    for (int perm = 0; perm < perm_amount; ++perm) {
        uint64_t blocker_board = 0;
        uint64_t mask = movement_mask;
        for (int i = 0; i < move_count; ++i) {
            uint64_t lsb = mask & -mask;
            mask -= lsb; 
            if (perm & (1 << i)) blocker_board |= lsb;
        }
        blocker_boards[perm] = blocker_board;

		for(int i = 0; i < perm; i++) {
			if(blocker_boards[i] == blocker_board) {
				print_bitboard(blocker_boards[i]);
				std::cout << "ERROR" << '\n';
			} 
		}
    }
    return blocker_boards;
}

// Make lookup table for slider pieces.
template<int max_perms, bool diagonal>
constexpr std::array<std::array<BitBoard, max_perms>, BOARD_SIZE> create_attack_table() {
	// Fetch the right data and function depending on whether the function call is for
	// bishop or rook movement.
	auto &masks = diagonal ? bishop_masks : rook_masks;
	auto magics = diagonal ? bishop_magic_numbers : rook_magic_numbers;
	auto on_fly = diagonal ? bishop_attack_on_fly : rook_attack_on_fly;

	// Generate all possible attack boards. 
	std::array<std::array<uint64_t, max_perms>, BOARD_SIZE> values{};
	for (int square = 0; square < BOARD_SIZE; square++) {
		// Get the relevant mask for the current square. This mask will be used
		// to determine the blockers, only blockers on the mask are relevant.
		BitBoard mask = masks[square];
		// Amount of bits in the mask, needed for index calculation.
		int relevant_bits = pcnt(mask);
		// Generate an array of all possible permutations for the mask.
		auto blockers = create_all_perms<max_perms>(mask);
		for (int board = 0; board < max_perms; ++board) {
			BitBoard blocker_board = blockers[board];
			// Calculate the index using magic number index formula.
			int index = (blocker_board * magics[square]) >> (64 - relevant_bits);
			// Generate the movement mask and store it at the calculated index.
			// Now, using the same hashing function, we can later retrieve the attack mask.
			values[square][index] = on_fly(square, blocker_board);
		}
	}
    
    return values;
}

// Attack table instances. Using magic numbers, the index can be calculated.
// With the index and the square, this array contains the attack board.
const auto bishop_attacks = create_attack_table<MAX_PERM_BISH, true>();
const auto rook_attacks = create_attack_table<MAX_PERM_BISH, false>();

// Find magic number for square.
template<bool diagonal, int max_perms>
uint64_t find_magic(Square s) {
	uint64_t attacks[max_perms], used[max_perms];
	// Get the amount of squares in the mask.
	const int relevant_bits = diagonal ? REL_BISH_BITS[s] : REL_ROOK_BITS[s];
	// Generate all possible permutations for the mask of this square.
	BitBoard mask = diagonal ? bishop_masks[s] : rook_masks[s];
	const auto occ = create_all_perms<max_perms>(mask);
	// Calculate the actual amount of permutations possible with current mask.
	int occ_indices = 1 << relevant_bits;
	// Generate attack boards for all the permutations.
	for(int i = 0; i < occ_indices; i++) 
		attacks[i] = diagonal ? bishop_attack_on_fly(s, occ[i]) : rook_attack_on_fly(s, occ[i]);
	// Brute force a magic number.
	for(uint64_t r = 0; r < 1000000000; r++) {
		uint64_t candidate = random_u64();
		// Skip garbage candidates.
		if(pcnt((mask * candidate) & 0xFF00000000000000ULL) < 6) continue;
		// Reset used array to zero.
		for(int i = 0; i < occ_indices; i++) used[i] = 0;
		// Check if magic number atks in a unique index for each blocker board.
		int index, fail;
		for(index = 0, fail = 0; !fail && index < occ_indices; index++) {
			int key = (occ[index] * candidate) >> (64 - relevant_bits);
			// If we get this key for the first time, store the attack board.
			// Else, we can still go on if the attack board is the same for the 
			// collision elements, otherwise, fail.
			if(used[key] == 0) used[key] = attacks[index];
			else if (used[key] != attacks[index]) fail = 1;
		}
		if(!fail) return candidate;
	}
	std::cout << "Failed to find magic number. \n";
	return 0;
}

void init_magics() {
	for(Square s = 0; s < 64; s++) {
		std::cout << "0x" << std::hex << find_magic<false, MAX_PERM_ROOK>(s) << '\n';
	}
}

#endif

}