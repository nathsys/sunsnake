#include <cstdio>
#include <iostream>
#include <random>

#include "defaults.hpp"

#pragma once

inline bool get_bit_64(uint64_t num, uint8_t pos) { return (num & (1ULL << (63 - pos))) != 0; }

inline constexpr uint64_t square_to_mask(Square s) { return 1ULL << s; }

inline uint8_t lbit(uint64_t n) { return __builtin_clzll(n); }

inline BitBoard pop_extract(BitBoard& b) {
    BitBoard bit = b & -b;
    b ^= bit;
    return bit;
}

static std::string print_binary(uint64_t n) {
    std::string result = "";
    do {
        if(n % 2 == 0) {
            result += '0';
        } else {
            result += '1';
        }
        n /= 2;
    } while (n > 0);
    return result;
}

static void print_bitboard(uint64_t bitboard) {
	printf("\n");
	// loop over board ranks.
	for (int rank = 0; rank < 8; rank++) {
		// loop over board files.
		for (int file = 0; file < 8; file++) {
			// convert file & rank into square index.
			int square = rank * 8 + file;

			// print ranks.
			if (!file) printf("  %d ", 8 - rank);

			// print bit state (either 1 or 0).
			printf(" %d", get_bit_64(bitboard, square) ? 1 : 0);
		}
		// print new line every rank.
		printf("\n");
	}
    std::cout << print_binary(bitboard) << '\n';
}

static uint64_t random_u64() {
    static std::mt19937_64 rng{ std::random_device{}() };
    return rng() & rng() & rng();
}

template <Player player>
constexpr Player opponent() {
	return (player == Player::white) ? Player::black : Player::white;
}