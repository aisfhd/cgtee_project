#!/usr/bin/env bash

mkdir -p build_debug
cmake -DCMAKE_BUILD_TYPE=Debug -S ./ -B ./build_debug/
cmake --build ./build_debug/ --config Debug -j