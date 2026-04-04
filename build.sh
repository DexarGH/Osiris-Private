#!/usr/bin/env bash

cmake -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build -j $(nproc --all)

# Copy SVG resources
mkdir -p build/Source/Resources
cp Resources/*.svg build/Source/Resources/ 2>/dev/null || true
