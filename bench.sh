#!/bin/bash
#SBATCH --job-name=graphstatic-bench
#SBATCH --partition=Centaurus
#SBATCH --time=00:20:00
#SBATCH --output=logs/%x-%j.out
#SBATCH --error=logs/%x-%j.err
#SBATCH --mem=20GB
#SBATCH --cpus-per-task=8

mkdir -p logs results

echo "Building program..."
make clean
make

echo "Running benchmarks..."

./level_client "Tom Hanks" 3
./level_client "Tom Hanks" 4
./level_client "Brad Pitt" 3
./level_client "Scarlett Johansson" 2

echo "Done."