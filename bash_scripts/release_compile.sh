#!/usr/bin/env bash

mkdir -p build_release
cmake -DCMAKE_BUILD_TYPE=Release -S ./ -B ./build_release/
cmake --build ./build_release/ --config Release -j