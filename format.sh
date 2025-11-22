#!/usr/bin/env bash

# Format all C/C++ files in the repo using clang-format
clang-format -i $(git ls-files "cw_decoder/*.c" "cw_decoder/*.cpp" "cw_decoder/*.h" "cw_decoder/*.hpp")
echo "✨ All files formatted."

