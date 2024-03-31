#!/bin/sh
rm -rf build && cmake -S . -B build && cmake --build build && ./build/nvidia_reallocations
