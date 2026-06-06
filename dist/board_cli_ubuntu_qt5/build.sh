#!/bin/sh

set -e

echo "Step 1: checking qmake"
qmake -v

echo "Step 2: generating Makefile"
qmake board_cli.pro

echo "Step 3: building"
make -j4

echo "Build finished."
echo "Run with: ./board_cli"
