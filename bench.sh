#!/bin/bash
#SBATCH --job-name=graphstatic-bench
#SBATCH --partition=Centaurus
#SBATCH --time=00:20:00
#SBATCH --output=logs/%x-%j.out
#SBATCH --error=logs/%x-%j.err
#SBATCH --mem=20GB
#SBATCH --cpus-per-task=8

set -euo pipefail
mkdir -p logs results

echo "Building programs..."
make clean
make

echo
echo "===== REQUIRED CASE: Tom Hanks / 3 ====="
echo "--- Sequential ---"
srun --cpu-bind=cores ./seq_client "Tom Hanks" 3
echo "--- Parallel ---"
srun --cpu-bind=cores ./level_client "Tom Hanks" 3

echo
echo "===== MORE TESTS (different nodes/depths) ====="

TRIALS=3

run_trials () {
  local label="$1"
  local bin="$2"
  local node="$3"
  local depth="$4"

  echo
  echo "[$label] node=\"$node\" depth=$depth"
  for t in $(seq 1 "$TRIALS"); do
    echo "  trial $t:"
    srun --cpu-bind=cores "$bin" "$node" "$depth"
  done
}

# Pick a small set so you don't blow time limits
run_trials "SEQ" ./seq_client "Tom Hanks" 1
run_trials "SEQ" ./seq_client "Tom Hanks" 2
run_trials "SEQ" ./seq_client "Tom Hanks" 4
run_trials "SEQ" ./seq_client "Brad Pitt" 3
run_trials "SEQ" ./seq_client "Scarlett Johansson" 2

run_trials "PAR" ./level_client "Tom Hanks" 1
run_trials "PAR" ./level_client "Tom Hanks" 2
run_trials "PAR" ./level_client "Tom Hanks" 4
run_trials "PAR" ./level_client "Brad Pitt" 3
run_trials "PAR" ./level_client "Scarlett Johansson" 2

echo
echo "Done."
echo "Check logs in logs/%x-%j.out and logs/%x-%j.err"