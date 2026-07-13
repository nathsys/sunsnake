#!/usr/bin/env python3
"""Cross-check sunsnake's perft output against Stockfish.

Two modes are provided:

  compare  Run perft at a given depth from a specific position (a sequence
           of moves from the starting position) and diff the per-move node
           counts against Stockfish. With --auto, automatically follows the
           first diverging move one ply at a time until the exact illegal or
           missing move is isolated.

  fuzz     Play out random legal games (Stockfish supplies the legal moves at
           each step, so every position reached is guaranteed valid) and run
           `compare` at each resulting position. Used to find bugs that don't
           show up in a straight perft run from the starting position.

Examples:

  python3 tools/perft_compare.py compare --depth 6
  python3 tools/perft_compare.py compare --depth 6 e2e4 e7e5 g1f3 --auto
  python3 tools/perft_compare.py fuzz --trials 100 --depth 5
"""

import argparse
import random
import re
import shutil
import subprocess
import sys
from pathlib import Path

MOVE_RE = re.compile(r"^([a-h][1-8][a-h][1-8][nbrq]?):\s*(\d+)$")
REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_ENGINE = REPO_ROOT / "build" / "release" / "sunsnake"


class EngineNotFoundError(RuntimeError):
    pass


class Stockfish:
    """A persistent Stockfish UCI process.

    Kept alive across many queries instead of spawning a fresh process (and
    reloading NNUE weights) per call, which matters once you're running
    hundreds of positions during fuzzing.
    """

    def __init__(self, path):
        self.proc = subprocess.Popen(
            [path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            bufsize=1,
        )
        self._send("uci")
        self._read_until("uciok")
        self._send("isready")
        self._read_until("readyok")

    def _send(self, command):
        self.proc.stdin.write(command + "\n")
        self.proc.stdin.flush()

    def _read_until(self, marker):
        lines = []
        while True:
            line = self.proc.stdout.readline()
            if not line:
                raise RuntimeError("stockfish closed its output unexpectedly")
            line = line.rstrip("\n")
            lines.append(line)
            if marker in line:
                return lines

    def _set_position(self, moves):
        command = "position startpos"
        if moves:
            command += " moves " + " ".join(moves)
        self._send(command)

    def legal_moves(self, moves):
        """Legal moves from the position reached by `moves`, via perft 1."""
        self._set_position(moves)
        self._send("go perft 1")
        lines = self._read_until("Nodes searched:")
        return [m.group(1) for line in lines if (m := MOVE_RE.match(line.strip()))]

    def perft_divide(self, moves, depth):
        """Returns (per_move_counts, total) for perft at `depth` from `moves`."""
        self._set_position(moves)
        self._send(f"go perft {depth}")
        lines = self._read_until("Nodes searched:")
        results = {}
        total = None
        for line in lines:
            m = MOVE_RE.match(line.strip())
            if m:
                results[m.group(1)] = int(m.group(2))
            if line.startswith("Nodes searched:"):
                total = int(re.findall(r"\d+", line)[-1])
        return results, total

    def close(self):
        try:
            self._send("quit")
            self.proc.wait(timeout=5)
        except Exception:
            self.proc.kill()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()


def run_engine_perft(engine_path, moves, depth):
    """Returns (per_move_counts, total) for our own engine at `depth` from `moves`."""
    command = [str(engine_path)]
    for move in moves:
        command.extend(["move", move])
    command.extend(["perft", str(depth)])

    result = subprocess.run(command, capture_output=True, text=True, timeout=300)
    results = {}
    total = None
    for line in result.stdout.split("\n"):
        m = MOVE_RE.match(line.strip())
        if m:
            results[m.group(1)] = int(m.group(2))
        if "Nodes searched:" in line:
            total = int(re.findall(r"\d+", line.replace(",", ""))[-1])
    return results, total


def diff_results(sf_results, engine_results):
    """List of (move, stockfish_count, engine_count) for every move that differs."""
    all_moves = sorted(set(sf_results) | set(engine_results))
    diffs = []
    for move in all_moves:
        sf_count = sf_results.get(move, 0)
        engine_count = engine_results.get(move, 0)
        if sf_count != engine_count:
            diffs.append((move, sf_count, engine_count))
    return diffs


def compare(stockfish, engine_path, moves, depth):
    sf_results, sf_total = stockfish.perft_divide(moves, depth)
    engine_results, engine_total = run_engine_perft(engine_path, moves, depth)
    return diff_results(sf_results, engine_results), sf_total, engine_total


def isolate(stockfish, engine_path, moves, depth):
    """Recursively follow the largest diverging move until depth 1, where the
    exact illegal (or missing) move can be read off directly."""
    path = list(moves)
    while depth > 1:
        diffs, sf_total, engine_total = compare(stockfish, engine_path, path, depth)
        print(f"depth={depth} moves={' '.join(path) or '(start)'} stockfish={sf_total} engine={engine_total} diffs={len(diffs)}")
        if not diffs:
            print("No difference at this depth; the mismatch must be above this point.")
            return path
        for move, sf_count, engine_count in diffs:
            if sf_count == 0 and engine_count > 0:
                print(f"illegal move allowed by engine: {move}")
                return path + [move]
            if engine_count == 0 and sf_count > 0:
                print(f"legal move missing from engine: {move}")
                return path
        move, sf_count, engine_count = max(diffs, key=lambda d: abs(d[1] - d[2]))
        print(f"  following {move} (stockfish={sf_count} engine={engine_count} diff={engine_count - sf_count:+d})")
        path.append(move)
        depth -= 1

    diffs, sf_total, engine_total = compare(stockfish, engine_path, path, 1)
    print(f"depth=1 moves={' '.join(path)} stockfish={sf_total} engine={engine_total}")
    for move, sf_count, engine_count in diffs:
        print(f"  {move}: stockfish={sf_count} engine={engine_count}")
    return path


def random_walk(stockfish, min_plies, max_plies):
    moves = []
    for _ in range(random.randint(min_plies, max_plies)):
        legal = stockfish.legal_moves(moves)
        if not legal:
            break
        moves.append(random.choice(legal))
    return moves


def resolve_engine_path(args):
    engine_path = Path(args.engine)
    if not engine_path.is_file():
        raise EngineNotFoundError(
            f"engine binary not found at {engine_path}\n"
            "Build it first (cmake --preset release && cmake --build --preset release), "
            "or pass --engine PATH."
        )
    return engine_path


def resolve_stockfish_path(args):
    if args.stockfish:
        return args.stockfish
    found = shutil.which("stockfish")
    if found:
        return found
    raise EngineNotFoundError(
        "stockfish not found on PATH. Install it or pass --stockfish PATH."
    )


def cmd_compare(args):
    engine_path = resolve_engine_path(args)
    stockfish_path = resolve_stockfish_path(args)

    with Stockfish(stockfish_path) as stockfish:
        if args.auto:
            isolate(stockfish, engine_path, args.moves, args.depth)
            return 0

        diffs, sf_total, engine_total = compare(stockfish, engine_path, args.moves, args.depth)

        if args.verbose:
            sf_results, _ = stockfish.perft_divide(args.moves, args.depth)
            engine_results, _ = run_engine_perft(engine_path, args.moves, args.depth)
            for move in sorted(set(sf_results) | set(engine_results)):
                sf_count = sf_results.get(move, 0)
                engine_count = engine_results.get(move, 0)
                mark = "OK" if sf_count == engine_count else "MISMATCH"
                print(f"{move:8} stockfish={sf_count:<12} engine={engine_count:<12} {mark}")

        if diffs:
            print(f"MISMATCH at depth {args.depth}, moves: {' '.join(args.moves) or '(start)'}")
            for move, sf_count, engine_count in diffs:
                print(f"  {move}: stockfish={sf_count} engine={engine_count} ({engine_count - sf_count:+d})")
            print(f"total: stockfish={sf_total} engine={engine_total}")
            return 1

        print(f"OK: {sf_total} nodes match at depth {args.depth}")
        return 0


def cmd_fuzz(args):
    engine_path = resolve_engine_path(args)
    stockfish_path = resolve_stockfish_path(args)
    if args.seed is not None:
        random.seed(args.seed)

    failures = []
    with Stockfish(stockfish_path) as stockfish:
        for trial in range(1, args.trials + 1):
            moves = random_walk(stockfish, args.min_plies, args.max_plies)
            diffs, sf_total, engine_total = compare(stockfish, engine_path, moves, args.depth)
            status = "OK" if not diffs else "MISMATCH"
            print(f"[{trial}/{args.trials}] plies={len(moves)} depth={args.depth} "
                  f"stockfish={sf_total} engine={engine_total} {status}")
            if diffs:
                print(f"    moves: {' '.join(moves)}")
                for move, sf_count, engine_count in diffs:
                    print(f"    {move}: stockfish={sf_count} engine={engine_count}")
                failures.append(moves)

    print()
    print(f"{args.trials} trials, {len(failures)} failures")
    if failures:
        print("Reproduce a failure with:")
        for moves in failures:
            print(f"  python3 {Path(__file__).name} compare --depth {args.depth} --auto {' '.join(moves)}")
        return 1
    return 0


def build_parser():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    subparsers = parser.add_subparsers(dest="mode", required=True)

    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--engine", default=str(DEFAULT_ENGINE), help="path to the sunsnake binary")
    common.add_argument("--stockfish", default=None, help="path to a stockfish binary (default: look up on PATH)")

    compare_parser = subparsers.add_parser("compare", parents=[common], help="compare a single position")
    compare_parser.add_argument("--depth", type=int, required=True)
    compare_parser.add_argument("--auto", action="store_true", help="recursively isolate the first diverging move")
    compare_parser.add_argument("--verbose", action="store_true", help="print every move, not just mismatches")
    compare_parser.add_argument("moves", nargs="*", help="moves from the starting position, e.g. e2e4 e7e5")
    compare_parser.set_defaults(func=cmd_compare)

    fuzz_parser = subparsers.add_parser("fuzz", parents=[common], help="fuzz random legal positions")
    fuzz_parser.add_argument("--trials", type=int, default=50)
    fuzz_parser.add_argument("--depth", type=int, default=5)
    fuzz_parser.add_argument("--min-plies", type=int, default=4)
    fuzz_parser.add_argument("--max-plies", type=int, default=30)
    fuzz_parser.add_argument("--seed", type=int, default=None, help="seed the RNG for a reproducible run")
    fuzz_parser.set_defaults(func=cmd_fuzz)

    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()
    try:
        return args.func(args)
    except EngineNotFoundError as e:
        print(f"error: {e}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
