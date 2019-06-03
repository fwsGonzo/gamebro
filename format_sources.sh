#!/bin/sh
set -e
echo "Formatting the codebase..."
find src/ -name '*.hpp' | xargs clang-format-7 -style=file -i
find src/ -name '*.cpp' | xargs clang-format-7 -style=file -i
find libgbc/ -name '*.hpp' | xargs clang-format-7 -style=file -i
find libgbc/ -name '*.cpp' | xargs clang-format-7 -style=file -i
find service/ -name '*.hpp' | xargs clang-format-7 -style=file -i
find service/ -name '*.cpp' | xargs clang-format-7 -style=file -i
find trainer/ -name '*.hpp' | xargs clang-format-7 -style=file -i
find trainer/ -name '*.cpp' | xargs clang-format-7 -style=file -i
echo "->   Completed successfully!"
