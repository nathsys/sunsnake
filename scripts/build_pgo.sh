#!/usr/bin/env bash
# Two-phase profile-guided optimization build.
#
# 1. Build an instrumented binary that records which branches/functions
#    actually get hit.
# 2. Run it on a representative workload (a mid-depth perft) so it has
#    real data to record.
# 3. Rebuild using that profile so the compiler can make better inlining
#    and layout decisions for the code paths that matter.
#
# Both phases reuse the same build/pgo directory on purpose: the profile
# data is matched up by object file path, and that only lines up if the
# directory and target name stay the same between the two builds.
set -euo pipefail
cd "$(dirname "$0")/.."

echo "==> [1/3] Configuring + building instrumented binary (profile-generate)"
cmake --preset pgo-generate
cmake --build --preset pgo-generate

echo "==> [2/3] Running perft 7 to collect profile data"
printf 'perft 7\nquit\n' | ./build/pgo/sunsnake > /dev/null

echo "==> [3/3] Rebuilding using the collected profile (profile-use)"
cmake --preset pgo-use
cmake --build --preset pgo-use

echo "==> Done: build/pgo/sunsnake is now PGO-optimized"
