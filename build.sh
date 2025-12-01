#!/usr/bin/env bash
set -e

# C++ compiler and flags
CXX=${CXX:-g++}
CXXFLAGS="-std=c++17 -Wall -Wextra -pedantic"

SRC_A="ta_marking_a_101308579_101299663.cpp"
SRC_B="ta_marking_b_101308579_101299663.cpp"

OUT_A="ta_marking_a"
OUT_B="ta_marking_b"

echo "Building Part 2.a ($SRC_A) -> $OUT_A ..."
$CXX $CXXFLAGS "$SRC_A" -o "$OUT_A"

echo "Building Part 2.b ($SRC_B) -> $OUT_B ..."
$CXX $CXXFLAGS "$SRC_B" -o "$OUT_B"

echo "Build complete."

