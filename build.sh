#!/usr/bin/env bash

cmake -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build -j $(nproc --all)
