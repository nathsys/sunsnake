import subprocess
import re
import sys
import argparse

path_to_stockfish = "/home/naat/Documents/stockfish/stockfish-ubuntu-x86-64-avx2"
path_to_own_engine = "/home/naat/dev/sunsnake/build/main"

def run_stockfish_perft(moves, depth):
    """Run perft on Stockfish and return the results as a dictionary."""
    cmd = [path_to_stockfish]
    print(f"Stockfish command: {' '.join(cmd)}")
    
    process = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, 
                              stderr=subprocess.PIPE, text=True)
    
    # Send commands to Stockfish
    commands = ["uci\n", "isready\n", "ucinewgame\n", "position startpos"]
    if moves:
        commands[-1] += " moves " + " ".join(moves)
    commands[-1] += "\n"
    commands.append("d\n")  # Display board
    commands.append(f"go perft {depth}\n")
    commands.append("quit\n")
    
    input_text = "".join(commands)
    print(f"Stockfish input commands:")
    for cmd_line in commands:
        print(f"  {cmd_line.strip()}")
    stdout, stderr = process.communicate(input=input_text)
    
    print("\nStockfish board output:")
    print(stdout)
    
    # Parse the perft output
    results = {}
    total_nodes = 0
    
    for line in stdout.split('\n'):
        # Look for move lines like "e2e4: 20"
        match = re.match(r'^([a-h][1-8][a-h][1-8][nbrq]?): (\d+)$', line.strip())
        if match:
            move, nodes = match.groups()
            results[move] = int(nodes)
        
        # Look for total nodes line
        if line.startswith("Nodes searched:") or "nodes" in line.lower():
            numbers = re.findall(r'\d+', line)
            if numbers:
                total_nodes = int(numbers[-1])
    
    return results, total_nodes

def run_custom_perft(moves, depth):
    """Run perft on custom engine and return the results as a dictionary."""
    cmd = [path_to_own_engine]
    
    # Add moves to command line
    for move in moves:
        cmd.extend(["move", move])
    
    # Add print command to show board
    cmd.extend(["print"])
    
    # Add perft command
    cmd.extend(["perft", str(depth)])

    cmd.extend(["quit"])
    
    print(f"Custom engine command: {' '.join(cmd)}")
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    print("\nCustom engine output:")
    print(result.stdout)
    
    # Parse the perft output
    results = {}
    total_nodes = 0
    
    for line in result.stdout.split('\n'):
        # Look for move lines like "e2e4: 20"
        match = re.match(r'^([a-h][1-8][a-h][1-8][nbrq]?): (\d+)$', line.strip())
        if match:
            move, nodes = match.groups()
            results[move] = int(nodes)
        
        # Look for total nodes line
        if "Nodes searched:" in line or "Total nodes:" in line:
            numbers = re.findall(r'\d+', line)
            if numbers:
                total_nodes = int(numbers[-1])
    
    return results, total_nodes


def get_stockfish_board(moves):
    """Return board as 8x8 array (rank 8..1) from Stockfish 'd' output for given moves."""
    cmd = [path_to_stockfish]
    process = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    commands = ["uci\n", "isready\n", "ucinewgame\n", "position startpos"]
    if moves:
        commands[-1] += " moves " + " ".join(moves)
    commands[-1] += "\n"
    commands.append("d\n")
    commands.append("quit\n")
    stdout, stderr = process.communicate(input="".join(commands))

    # Parse 8x8 board block: find 8 consecutive lines with 8 tokens of pieces or '.'
    board_lines = []
    for line in stdout.splitlines():
        # First try to find a FEN line which Stockfish prints as 'Fen: <fen>'
        fen_match = re.search(r'Fen:\s*(\S+)', line)
        if fen_match:
            fen = fen_match.group(1)
            # build board from fen
            ranks = fen.split('/')
            if len(ranks) == 8:
                board = []
                for r in ranks:
                    row = []
                    for ch in r:
                        if ch.isdigit():
                            row.extend(['.'] * int(ch))
                        else:
                            row.append(ch)
                    board.append(row)
                return board

        tokens = line.strip().split()
        if len(tokens) >= 8:
            # consider only tokens that look like pieces or '.'
            filtered = [t for t in tokens if re.match(r'^[rnbqkpRNBQKP\.]$', t) or t == '.' ]
            if len(filtered) == 8:
                board_lines.append(filtered)
                # try to collect 8 lines
                if len(board_lines) == 8:
                    return board_lines
        else:
            board_lines = []

    return None


def render_board(board, highlight=None):
    """Render board (8x8 list) to a printable string. highlight=(src,dst) in 'e2' format."""
    if board is None:
        return "(no board available)"

    src = dst = None
    if highlight:
        src, dst = highlight
        src_row, src_col = square_to_coords(src)
        dst_row, dst_col = square_to_coords(dst)

    out_lines = []
    files = 'a b c d e f g h'.split()
    for r in range(8):
        rank_num = 8 - r
        row = board[r][:]
        display_cells = []
        for c in range(8):
            cell = row[c]
            if highlight and r == src_row and c == src_col:
                display_cells.append(f'({cell})')
            elif highlight and r == dst_row and c == dst_col:
                display_cells.append(f'[{cell}]')
            else:
                display_cells.append(f' {cell} ')
        out_lines.append(f"{rank_num} " + " ".join(display_cells))
    out_lines.append("  " + "  ".join(files))
    return "\n".join(out_lines)


def square_to_coords(sq):
    file = sq[0]
    rank = int(sq[1])
    col = ord(file) - ord('a')
    row = 8 - rank
    return row, col

def compare_perft_results(moves, depth):
    """Compare perft results between Stockfish and custom engine."""
    print(f"\nRunning perft comparison at depth {depth}")
    if moves:
        print(f"After moves: {' '.join(moves)}")
    print("-" * 60)

    # Always print the current position as visible to Stockfish.
    board = get_stockfish_board(moves)
    if board is not None:
        print("Current position:")
        print(render_board(board))
        print("-" * 60)

    # Run both engines
    print("Running Stockfish...")
    stockfish_results, stockfish_total = run_stockfish_perft(moves, depth)

    print("Running custom engine...")
    custom_results, custom_total = run_custom_perft(moves, depth)

    # Compare results
    all_moves = set(stockfish_results.keys()) | set(custom_results.keys())
    differences = []

    print(f"\n{'Move':<8} {'Stockfish':<12} {'Custom':<12} {'Difference':<12}")
    print("-" * 50)

    for move in sorted(all_moves):
        sf_nodes = stockfish_results.get(move, 0)
        custom_nodes = custom_results.get(move, 0)
        diff = custom_nodes - sf_nodes

        status = "✓" if diff == 0 else "✗"
        # Print a concise per-move line
        print(f"{move:<8} {sf_nodes:<12} {custom_nodes:<12} {diff:+<12} {status}")

        if diff != 0:
            differences.append((move, sf_nodes, custom_nodes, diff))

    print("-" * 50)
    print(f"{'TOTAL':<8} {stockfish_total:<12} {custom_total:<12} {custom_total - stockfish_total:+<12}")

    if differences:
        print(f"\nFound {len(differences)} differences:")
        for move, sf, custom, diff in differences:
            print(f"  {move}: Stockfish={sf}, Custom={custom}, Diff={diff:+}")
    else:
        print("\n✓ All results match!")

    # Return structured results so callers can decide how to proceed
    return {
        'differences': differences,
        'stockfish_results': stockfish_results,
        'custom_results': custom_results,
        'stockfish_total': stockfish_total,
        'custom_total': custom_total,
        'all_moves': sorted(all_moves),
    }

def main():
    parser = argparse.ArgumentParser(description='Compare perft between Stockfish and custom engine.')
    parser.add_argument('depth', type=int, help='Perft depth')
    parser.add_argument('moves', nargs='*', help='Moves to apply before running perft (in UCI notation)')
    parser.add_argument('--auto', action='store_true', help='Automatically follow first differing move recursively')
    args = parser.parse_args()

    depth = args.depth
    moves = args.moves

    # Run initial comparison and print results
    res = compare_perft_results(moves, depth)

    # If user requested automatic recursion and there are differences, try to isolate
    if args.auto:
        def recursive_isolate(current_moves, current_depth):
            if current_depth <= 0:
                print("Reached depth 0 without isolating a move.")
                return False, current_moves, 'depth_limit'

            res = compare_perft_results(current_moves, current_depth)
            diffs = res.get('differences', [])
            if not diffs:
                print("No differences at this level — mismatch resolved.")
                return True, current_moves, 'resolved'

            # Check for a move that one engine allows and the other does not
            for move, sf_nodes, custom_nodes, diff in diffs:
                if sf_nodes > 0 and custom_nodes == 0:
                    print(f"Isolated illegal move for custom engine: {move}")
                    return False, current_moves + [move], 'custom_illegal'
                if custom_nodes > 0 and sf_nodes == 0:
                    print(f"Isolated illegal move for Stockfish: {move}")
                    return False, current_moves + [move], 'stockfish_illegal'

            # Otherwise follow the first differing move and recurse
            next_move = diffs[0][0]
            print(f"Following differing move {next_move} and recursing to depth {current_depth-1}...")
            return recursive_isolate(current_moves + [next_move], current_depth - 1)

        print("\nStarting automatic recursive isolation...")
        found, path, reason = recursive_isolate(moves, depth)
        if found:
            print(f"Recursive isolation succeeded. Path: {' '.join(path)}")
            sys.exit(0)
        else:
            print(f"Recursive isolation stopped. Reason: {reason}. Path: {' '.join(path)}")
            # If we isolated an illegal move for custom engine, visualize it
            if reason == 'custom_illegal' and path:
                illegal_move = path[-1]
                before_moves = path[:-1]
                print('\nBoard before illegal move (as shown by Stockfish):')
                before_board = get_stockfish_board(before_moves)
                print(render_board(before_board, highlight=(illegal_move[0:2], illegal_move[2:4])))

                print('\nBoard after illegal move applied (Stockfish allows it):')
                after_board = get_stockfish_board(path)
                print(render_board(after_board))
                # Print subtree (depth=1) allowed moves for both engines from the position before the illegal move
                print('\nSubtree (depth=1) moves from the position BEFORE the illegal move:')
                sf_moves, sf_total = run_stockfish_perft(before_moves, 1)
                custom_moves, custom_total = run_custom_perft(before_moves, 1)
                allowed_sf = sorted([m for m, n in sf_moves.items() if n > 0])
                allowed_custom = sorted([m for m, n in custom_moves.items() if n > 0])
                print(f"Stockfish allows {len(allowed_sf)} moves: {' '.join(allowed_sf)}")
                print(f"Custom allows {len(allowed_custom)} moves: {' '.join(allowed_custom)}")

                missing_moves = sorted(set(allowed_sf) - set(allowed_custom))
                print(f"\nDifference: {len(missing_moves)} move(s) missing from custom engine")
                if missing_moves:
                    first_missing = missing_moves[0]
                    print(f"First missing move: {first_missing}")
                    print(f"Board before move {first_missing}:")
                    before_move_board = get_stockfish_board(before_moves)
                    print(render_board(before_move_board, highlight=(first_missing[0:2], first_missing[2:4])))
                    print(f"\nBoard after move {first_missing}:")
                    after_move_board = get_stockfish_board(before_moves + [first_missing])
                    print(render_board(after_move_board))

            sys.exit(1)

    # Non-auto mode: decide exit based on whether differences were found
    if res.get('differences'):
        sys.exit(1)
    else:
        sys.exit(0)

if __name__ == "__main__":
    main()