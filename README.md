# sunsnake

[![CI](https://github.com/nathsys/sunsnake/actions/workflows/ci.yml/badge.svg)](https://github.com/nathsys/sunsnake/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A bitboard chess move generator and perft engine written in C++20.

## Highlights

* Legal move generation for every piece type
* Castling with correct rights tracking: rights are permanently lost once a king or rook has moved, even if the rook later returns to its original square
* En passant capture with discovered check detection
* All four promotion pieces, including under pin and check constraints
* Per piece pin resolution: each pinned piece's legal squares are computed individually through ray intersection with the king's line of sight, rather than one shared mask per pin direction, so two simultaneous pins of the same direction on different pieces are each handled correctly
* Incremental Zobrist hashing, updated inside `make_move`/`unmake_move` alongside the board itself, so a search built on top gets a transposition table key for free instead of hashing the whole board every node
* Bitboard core using hardware `PEXT`/`PDEP` (BMI2) for sliding piece attacks, falling back to magic bitboard multiplication when unavailable
* Move generation and perft counting kept as separate components (`move_generator.hpp` versus `move_counter.hpp`), so the generator can be reused by a search algorithm without depending on anything perft specific
* Usable as a library: headers live under `include/sunsnake/` in `namespace sunsnake`, consumable via CMake `add_subdirectory` or `FetchContent`, with a worked example in `examples/`
* Profile guided optimization available as a single script
* Exact perft verification through depth 8 plus randomized fuzz testing against Stockfish

## Performance

On an AMD Ryzen 9 7900X, perft depth 7 from the starting position (3,195,901,860 nodes) completes in 6.4 seconds here (about 500 million nodes per second, 750 million nodes per second with hashing enabled).

Profile guided optimization (see below) adds a further 10 to 15 percent on top of the plain `-Ofast -march=native` build.

## Building

Requires CMake 3.21 or newer and a C++20 compiler. Developed and tested on Linux with GCC; other GCC or Clang based Unix like setups should work, Windows/MSVC has not been tried.

```sh
git clone https://github.com/nathsys/sunsnake.git
cd sunsnake
cmake --preset release
cmake --build --preset release
./build/release/sunsnake
```

This builds with `-Ofast -march=native -funroll-loops`. If you're building a binary that needs to run on a different machine than the one compiling it, turn off the native arch tuning:

```sh
cmake --preset release -DSUNSNAKE_NATIVE_ARCH=OFF
```

### Other build presets

* `release`, the optimized build for normal use
* `debug`, unoptimized with debug symbols, for use with gdb
* `tests`, runs the internal self-test suite on startup (checks the magic bitboard attack tables against on the fly attack generation)
* `magics`, builds a helper that regenerates the magic bitboard numbers, see below

```sh
cmake --preset debug && cmake --build --preset debug
cmake --preset tests && cmake --build --preset tests && ./build/tests/sunsnake
```

## Usage

`sunsnake` starts from the standard opening position. With no arguments it drops into an interactive prompt; with arguments it runs the given commands and exits, writing only command output to stdout (status messages go to stderr), which makes it easy to script.

```
> perft 6                    run perft to depth 6 from the current position
> perft 6 hash               same, but using a Zobrist-hash-cached perft internally
> perft 6 hash compare       run perft plain and hashed, report whether they match and the NPS difference
> move e2e4                  play a move, from square to square, plus a promotion letter if needed: e7e8q
> print                      print the current board
> quit
```

```sh
./build/release/sunsnake move e2e4 move e7e5 perft 5
```

## Using this as a library

Move generation lives entirely in headers under `include/sunsnake/`, wrapped in `namespace sunsnake`, and does not depend on anything in `main.cpp`. A search algorithm can link against it directly instead of copying files around.

Add it to a CMake project with `add_subdirectory` or `FetchContent`, then link against `sunsnake::sunsnake`:

```cmake
add_subdirectory(sunsnake)
target_link_libraries(my_engine PRIVATE sunsnake::sunsnake)
```

The core types are `Board` (piece placement and occupancy), `GameState` (side to move, castling rights, en passant file, and a Zobrist `hash` kept in sync automatically), `Move` (a packed 16 bit encoding), and `MoveStack`/`MoveList` (the stack based move buffer used during generation, so generating moves does not allocate).

`GameState::hash` is a full Zobrist key, updated incrementally on every `make_move`/`unmake_move` call rather than recomputed from scratch, so it is ready to use as a transposition table key. `get_initial_gamestate()` seeds it correctly for the standard starting position; a `GameState` built any other way should call `Zobrist::compute(board, state.data, state.player())` once to seed `hash` before making any moves.

Player color, piece type, and slider direction are compile time template parameters throughout, so a hot recursive search should template its own functions on `Player` the same way `move_counting::perft` does internally. For the call boundary, where a caller typically only has a runtime `GameState`, non-template overloads are available:

* `move_generation::generate_moves(board, state, move_stack)`
* `make_move::make_move(board, state, move, flag_stack)`
* `make_move::unmake_move(board, state, move, flag_stack)`

`examples/search_skeleton.cpp` is a minimal negamax that shows the full pattern: generate moves, make one, recurse, unmake it, plus a transposition table keyed directly by `GameState::hash`.

```sh
cmake --preset release -DSUNSNAKE_BUILD_EXAMPLES=ON
cmake --build --preset release
./build/release/examples/search_skeleton 5
```

## Profile guided optimization

```sh
./scripts/build_pgo.sh
```

This builds an instrumented binary, runs a representative `perft 7` so the compiler has real branch and call data to work with, then rebuilds using that profile. The result is `build/pgo/sunsnake`. Both build steps use the same build directory on purpose, since GCC matches profile data to object files by path, and that only lines up correctly if the directory and target stay the same between the two builds.

## Magic bitboard generation

On CPUs without BMI2 (`PEXT`/`PDEP`), sliding piece attacks are looked up through magic bitboard multiplication instead. Valid magic numbers are already checked into `static_tables.hpp`, so a normal build does not need this. It exists for regenerating them, or verifying the search:

```sh
cmake --preset magics
cmake --build --preset magics
./build/magics/sunsnake
```

This brute forces a valid magic number per square for both rook and bishop attacks and prints them. Paste the output into `static_tables.hpp` to use them. The search is randomized, so a fresh run finds different, but equally valid, numbers than the ones already checked in.

## Testing

Correctness is checked at two levels.

Exact perft counts from the starting position match the well known reference node counts through depth 8, with no divergence at any depth:

* depth 1: 20
* depth 2: 400
* depth 3: 8,902
* depth 4: 197,281
* depth 5: 4,865,609
* depth 6: 119,060,324
* depth 7: 3,195,901,860
* depth 8: 84,998,978,956

Beyond the starting position, `tools/perft_compare.py` cross checks this engine against Stockfish. It has two modes: `compare` diffs the per move node count breakdown at a specific position, optionally isolating the exact diverging move automatically; `fuzz` plays out random legal games (Stockfish supplies the legal move list at each step, so every position reached is guaranteed valid) and runs `compare` at each one. Because random fuzzing reaches positions a fixed perft run from the starting position does not, such as two simultaneous pins of the same direction, it exercises pin resolution, castling rights tracking, and en passant handling under a wider range of conditions than the depth table above covers on its own.

The script has no third party dependencies beyond a local Stockfish binary, resolved from `PATH` or passed explicitly:

```sh
python3 tools/perft_compare.py compare --depth 6
python3 tools/perft_compare.py compare --depth 6 e2e4 e7e5 g1f3 --auto
python3 tools/perft_compare.py fuzz --trials 100 --depth 5 --stockfish /path/to/stockfish
```

The self-test suite (`tests` preset above) checks the magic bitboard attack tables against on the fly attack generation, and walks the move tree to depth 4 checking that the incremental Zobrist hash matches an independent from-scratch recompute at every node, and that it is restored exactly after every `unmake_move`. The suite builds with assertions forced on regardless of optimization level, since CMake's default Release flags define `NDEBUG`, which would otherwise silently strip every `assert()` the suite relies on to fail. Both the exact perft check and the self-test suite run in CI on every push.

`perft <depth> hash` (see Usage above) is a second, opt-in way to exercise the hash: it runs the same perft through a path that caches subtree node counts by `(GameState::hash, depth remaining)` instead of recomputing them, and prints identical output to a plain `perft <depth>` run, just faster once transpositions start hitting the cache.

Adding `compare` (`perft <depth> hash compare`) runs both the plain and hashed paths, timed separately, and reports whether their totals match plus the NPS difference between the two:

```
Plain nodes:  119,060,324
Plain NPS:    417,442,075
Hashed nodes: 119,060,324
Hashed NPS:   542,475,368
NPS diff:     +30.0%
Zobrist hash validation: match
```

A collision or an incremental update bug would cause a transposition to reuse the wrong cached count, so a mismatch here points at the hash rather than at move generation itself. The NPS gain from caching transpositions grows with depth, since deeper trees revisit far more transposed positions.

## Architecture

All headers live under `include/sunsnake/` and their contents sit in `namespace sunsnake`.

* `chess_board.hpp`, the board representation: bitboards, occupancy, attack queries
* `move_generator.hpp`, legal move generation, the reusable core of the project
* `move_counter.hpp`, `perft`, built on top of the move generator
* `make_move.hpp`, applying and unapplying a move on the board
* `maskset.hpp`, the per position check and pin masks used during move generation
* `move_logic.hpp`, `dynamic_tables.hpp`, `static_tables.hpp`, attack generation (PEXT or magic bitboards) and their lookup tables
* `gamestate.hpp`, castling rights, en passant file, side to move
* `zobrist.hpp`, the incremental position hash, kept in sync by `make_move.hpp`
* `movelist.hpp`, the stack based move list used during generation and search

## License

MIT, see [LICENSE](LICENSE).

Made by Nathanael Mohanu
