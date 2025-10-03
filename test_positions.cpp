#include <iostream>
#include <cstdint>

constexpr uint64_t c_init_queen_squares = 0b0001000000000000000000000000000000000000000000000000000000010000ULL;
constexpr uint64_t c_init_king_squares = 0b0000100000000000000000000000000000000000000000000000000000001000ULL;

int main() {
    std::cout << "Queen squares bits set at positions:" << std::endl;
    for (int i = 0; i < 64; i++) {
        if (c_init_queen_squares & (1ULL << i)) {
            char file = 'a' + (i % 8);
            char rank = '1' + (i / 8);
            std::cout << "Position " << i << ": " << file << rank << std::endl;
        }
    }
    
    std::cout << "\nKing squares bits set at positions:" << std::endl;
    for (int i = 0; i < 64; i++) {
        if (c_init_king_squares & (1ULL << i)) {
            char file = 'a' + (i % 8);
            char rank = '1' + (i / 8);
            std::cout << "Position " << i << ": " << file << rank << std::endl;
        }
    }
    
    return 0;
}
