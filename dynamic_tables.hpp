#include "defaults.hpp"
#include "util.hpp"
#include "static_tables.hpp"

#include <array>
#include <cstdint>

#pragma once

namespace Lookup {

// *= Rook and bishop attack boards on fly. *=
// Takes a square index and an occupancy board.
// These functions returns a bitboard with all the squares that the piece can attack
// or move to.

// Bishop moving logic. In order of up-right, up-left, down-right, down-left.
constexpr BitBoard bishop_attack_on_fly(Square s, BitBoard occ) {
    BitBoard atk = 0ULL;
    for (int sq = s + 9; sq <= 63 && sq % 8 != 0; sq += 9) {
        atk |= (1ULL << sq);
        if (occ & (1ULL << sq)) break;
    }
    for (int sq = s + 7; sq <= 63 && sq % 8 != 7; sq += 7) {
        atk |= (1ULL << sq);
        if (occ & (1ULL << sq)) break;
    }
    for (int sq = s - 7; sq >= 0 && sq % 8 != 0; sq -= 7) {
        atk |= (1ULL << sq);
        if (occ & (1ULL << sq)) break;
    }
    for (int sq = s - 9; sq >= 0 && sq % 8 != 7; sq -= 9) {
        atk |= (1ULL << sq);
        if (occ & (1ULL << sq)) break;
    }
    return atk;
}

// Rook move logic. In order of up down left right.
constexpr BitBoard rook_attack_on_fly(Square s, BitBoard occ) {
    BitBoard atk = 0ULL;
    for (int sq = s + 8; sq <= 63; sq += 8) {
        atk |= (1ULL << sq);
        if (occ & (1ULL << sq)) break;
    }
    for (int sq = s - 8; sq >= 0; sq -= 8) {
        atk |= (1ULL << sq);
        if (occ & (1ULL << sq)) break;
    }
    for (int sq = s + 1; sq % 8 != 0; ++sq) {
        atk |= (1ULL << sq);
        if (occ & (1ULL << sq)) break;
    }
    for (int sq = s - 1; sq % 8 != 7 && sq >= 0; --sq) {
        atk |= (1ULL << sq);
        if (occ & (1ULL << sq)) break;
    }
    return atk;
}


// *= PEXT *=
// If the processor supports the PEXT instruction, we can use it to get
// the attack boards for rook and bishop pieces more efficiently.
#ifdef _pdep_u64

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

// *= Bishop and rook masks *=

// Return the maximum amount of permutations possible for a given slider type.
// This is uded to determine the size of the attack board table.
template<SliderType slider_type>
inline constexpr int max_perms() {
	if constexpr IS_DIAGONAL(slider_type) {
		return MAX_PERM_BISH;
	} else {
		return MAX_PERM_ROOK;
	}
}

// Makes an attack mask from given s, the mask excludes the far edges in each
// direction as these are redundant, this is because if an edge is reached, the
// piece will always be able to attack the edge s.
// Primary template declaration
template <SliderType slider_type>
constexpr BitBoard make_slider_mask(const Square s);

// Rook specialization
template <>
constexpr BitBoard make_slider_mask<SliderType::orthogonal>(const Square s) {
    BitBoard mask = 0ULL;
    const int rank = s / 8, file = s % 8;
	// Up, down, left, right.
    for (int r = rank + 1; r <= 6; ++r)
        mask |= (1ULL << (file + 8 * r));
    for (int r = rank - 1; r >= 1; --r)
        mask |= (1ULL << (file + 8 * r));
    for (int f = file + 1; f <= 6; ++f)
        mask |= (1ULL << (f + 8 * rank));
    for (int f = file - 1; f >= 1; --f)
        mask |= (1ULL << (f + 8 * rank));
    return mask;
}

// Bishop specialization
template <>
constexpr BitBoard make_slider_mask<SliderType::diagonal>(const Square s) {
    BitBoard mask = 0ULL;
    const int rank = s / 8, file = s % 8;
    // Up-right, up-left, down-right, down-left.
    for (int r = rank + 1, f = file + 1; r <= 6 && f <= 6; ++r, ++f)
        mask |= (1ULL << (f + 8 * r));
    for (int r = rank + 1, f = file - 1; r <= 6 && f >= 1; ++r, --f)
        mask |= (1ULL << (f + 8 * r));
    for (int r = rank - 1, f = file + 1; r >= 1 && f <= 6; --r, ++f)
        mask |= (1ULL << (f + 8 * r));
    for (int r = rank - 1, f = file - 1; r >= 1 && f >= 1; --r, --f)
        mask |= (1ULL << (f + 8 * r));
    return mask;
}

// Make lookup tables for the attack masks. This assures that the masks are calculated at compile time,
// so they are available at runtime without the need to calculate them again.
template<SliderType slider_type>
constexpr auto make_mask_table() {
	std::array<BitBoard, BOARD_SIZE> values{};
	for (int s = 0; s < BOARD_SIZE; s++) 
		values[s] = make_slider_mask<slider_type>(s);
	return values;
}

constexpr auto bishop_masks = make_mask_table<SliderType::diagonal>();
constexpr auto rook_masks = make_mask_table<SliderType::orthogonal>();

// Given a movement mask, return an array containing all possible permutations of the mask.
template <SliderType slider_type>
constexpr auto create_all_perms(const BitBoard movement_mask) {
    std::array<BitBoard, max_perms<slider_type>()> blocker_boards{};
	// Use the bitcount to calculate the amount of permutations.
    const int move_count = pcnt(movement_mask);
    const int perm_amount = 1 << move_count;
    // Use the actual perm amount ( < max_perms) and generate all of them.
    for (int perm = 0; perm < perm_amount; ++perm) {
        BitBoard blocker_board = 0;
        BitBoard mask = movement_mask;
        for (int i = 0; i < move_count; ++i) {
            BitBoard lsb = mask & -mask;
            mask -= lsb; 
            if (perm & (1 << i)) blocker_board |= lsb;
        }
        blocker_boards[perm] = blocker_board;
    }
    return blocker_boards;
}

// Make lookup table for slider pieces.
template<SliderType slider_type>
constexpr auto create_attack_table() {
	// Fetch the right data and function depending on whether the function call is for
	// bishop or rook movement.
	constexpr auto& masks = IS_DIAGONAL(slider_type) ? bishop_masks : rook_masks;
	constexpr auto& magics = IS_DIAGONAL(slider_type) ? bishop_magic_numbers : rook_magic_numbers;
	constexpr auto on_fly = IS_DIAGONAL(slider_type) ? bishop_attack_on_fly : rook_attack_on_fly;

	// Generate all possible attack boards. 
	std::array<std::array<BitBoard, max_perms<slider_type>()>, BOARD_SIZE> values{};
	for (int s = 0; s < BOARD_SIZE; s++) {
		// Get the relevant mask for the current s. This mask will be used
		// to determine the blockers, only blockers on the mask are relevant.
		const BitBoard mask = masks[s];
		// Amount of bits in the mask, needed for index calculation.
		const int relevant_bits = IS_DIAGONAL(slider_type) ? REL_BISH_BITS[s] : REL_ROOK_BITS[s];
		// Generate an array of all possible permutations for the mask.
		const auto blockers = create_all_perms<slider_type>(mask);
		for (const auto &blocker_board : blockers) {
			// Calculate the index using magic number index formula.
			const int index = (blocker_board * magics[s]) >> (64 - relevant_bits);
			// Generate the movement mask and store it at the calculated index.
			// Now, using the same hashing function, we can later retrieve the attack mask.
			values[s][index] = on_fly(s, blocker_board);
		}
	}
    
    return values;
}

// Attack table instances. Using magic numbers, the index can be calculated.
// With the index and the s, this array contains the attack board.
const auto bishop_attacks = create_attack_table<SliderType::diagonal>();
const auto rook_attacks = create_attack_table<SliderType::orthogonal>();

#endif // _pdep_u64

#ifdef generate_magics

// Find magic number for s.
template<SliderType slider_type>
uint64_t find_magic(const Square s) {
	uint64_t attacks[max_perms<slider_type>()], used[max_perms<slider_type>()];
	// Generate all possible permutations for the mask of this s.
	constexpr BitBoard mask = IS_DIAGONAL(slider_type) ? bishop_masks[s] : rook_masks[s];
	constexpr auto occ = create_all_perms<max_perms<slider_type>()>(mask);
	// Calculate the actual amount of permutations possible with current mask.
	constexpr int relevant_bits = IS_DIAGONAL(slider_type) ? REL_BISH_BITS[s] : REL_ROOK_BITS[s];
	constexpr int occ_indices = 1 << relevant_bits;
	// Generate attack boards for all the permutations.
	for(int i = 0; i < occ_indices; i++) 
		attacks[i] = IS_DIAGONAL(slider_type) ? bishop_attack_on_fly(s, occ[i]) : rook_attack_on_fly(s, occ[i]);
	// Brute force a magic number.
	for(uint64_t r = 0; r < MAX_MAGIC_ATTEMPTS; r++) {
		const uint64_t candidate = random_u64();
		// Skip garbage candidates.
		if(pcnt((mask * candidate) & MIN_MAGIC_VALUE) < 6) continue;
		// Reset used array to zero.
		for(int i = 0; i < occ_indices; i++) used[i] = 0;
		// Check if magic number generates a unique index for each blocker board.
		int index, fail;
		for(index = 0, fail = 0; !fail && index < occ_indices; index++) {
			// Magic number hashing function.
			const int key = (occ[index] * candidate) >> (64 - relevant_bits);
			// If we get this key for the first time, store the attack board.
			// Else, we can still go on if the attack board is the same for the 
			// collision elements, otherwise, fail.
			if(used[key] == 0) used[key] = attacks[index];
			else if (used[key] != attacks[index]) fail = 1;
		}
		if(!fail) return candidate;
	}
	std::cout << "Failed to find magic number for " << std::hex << s << ". \n";
	return 0;
}

template<SliderType slider_type>
void init_magics() {
	for(Square s = 0; s < BOARD_SIZE; s++) {
		if(s) std::cout << ", \n";
		if constexpr (IS_DIAGONAL(slider_type))
			std::cout << "0x" << std::hex << find_magic<SliderType::diagonal>(s) << "ULL";
		else
			std::cout << "0x" << std::hex << find_magic<SliderType::orthogonal>(s) << "ULL";

	}
}
#endif // generate_magics

}