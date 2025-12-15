#!/usr/bin/env bash

mkdir -p build_release
cmake -S ./ -B ./build_release/
cmake --build ./build_release/
start ./build_release/Debug/cgtee.exe