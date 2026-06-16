#!/bin/sh

set -e

FILES=$(git diff --name-only --diff-filter=ACM | grep -E '\.(c|cc|cpp|cxx)$' | grep -E 'bike_computer' | grep -v 'tests/' || true)

[ -z "$FILES" ] && exit 0

. scripts/activate.sh 

WEST=/home/serge/embsys/.venv/bin/west
"$WEST" build -b native_sim bike_computer --pristine -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
mkdir -p build_clang
python3 scripts/filter_compile_commands.py build/compile_commands.json build_clang/compile_commands.json

for file in $FILES
do
    echo "Running clang-tidy on $file"
    clang-tidy -p build_clang "$file"
done