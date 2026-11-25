#!/usr/bin/env bash
# Build and run the host simulation (needs g++ or clang++ with C++17).
#   test/sim/run.sh                 # 20 seeds x 30 min (about a minute of CPU time)
#   test/sim/run.sh --seeds 5 --minutes 5
set -euo pipefail
cd "$(dirname "$0")/../.."
CXX="${CXX:-g++}"
OUT="${TMPDIR:-/tmp}/dialrally_sim"
"$CXX" -std=c++17 -O2 -Wall -Itest/sim/stubs -Isrc \
    test/sim/sim.cpp src/game/GameEngine.cpp src/game/Patterns.cpp \
    src/game/Hazard.cpp src/game/Player.cpp src/game/AutoPilot.cpp -o "$OUT"
"$OUT" "$@"
