import subprocess
import re
import sys

path_to_stockfish = "/home/ubuntu/dev/Stockfish/src/stockfish"
path_to_own_engine = "/home/ubuntu/dev/sunsnake/build/main"

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

def compare_perft_results(moves, depth):
    """Compare perft results between Stockfish and custom engine."""
    print(f"\nRunning perft comparison at depth {depth}")
    if moves:
        print(f"After moves: {' '.join(moves)}")
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
        print(f"{move:<8} {sf_nodes:<12} {custom_nodes:<12} {diff:+<12} {status}")
        
        if diff != 0:
            differences.append((move, sf_nodes, custom_nodes, diff))
    
    print("-" * 50)
    print(f"{'TOTAL':<8} {stockfish_total:<12} {custom_total:<12} {custom_total - stockfish_total:+<12}")
    
    if differences:
        print(f"\nFound {len(differences)} differences:")
        for move, sf, custom, diff in differences:
            print(f"  {move}: Stockfish={sf}, Custom={custom}, Diff={diff:+}")
        return False
    else:
        print("\n✓ All results match!")
        return True

def main():
    if len(sys.argv) < 2:
        print("Usage: python perft_debugger.py <depth> [move1] [move2] ...")
        print("Example: python perft_debugger.py 3")
        print("Example: python perft_debugger.py 3 e2e4 e7e5")
        sys.exit(1)
    
    depth = int(sys.argv[1])
    moves = sys.argv[2:] if len(sys.argv) > 2 else []
    
    success = compare_perft_results(moves, depth)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()