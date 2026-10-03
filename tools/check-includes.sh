#!/usr/bin/env bash
# Report (or with --fix, apply) the includes each file of src/ and tests/
# doesn't use, and those it uses only through another header, as
# clang-include-cleaner sees them.
#
# usage: tools/check-includes.sh [--fix] [FILE...]
# Needs a compile database with the tests: a build directory configured
# with ROGUE_BUILD_TESTS=ON and GCC-only flags left out (default: build).
set -euo pipefail
repo=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$repo"
mode=(--print=changes)
if [ "${1:-}" = --fix ]; then
	mode=(--edit)
	shift
fi
db=${ROGUE_COMPILE_DB:-build}
[ -f "$db/compile_commands.json" ] || { echo "no $db/compile_commands.json; set ROGUE_COMPILE_DB" >&2; exit 2; }
if [ $# -eq 0 ]; then
	mapfile -t files < <(git ls-files 'src/*.cpp' 'tests/*.cpp')
else
	files=("$@")
fi
# libstdc++'s internal headers stand for the standard headers that include
# them, and nlohmann/json_fwd.hpp for nlohmann/json.hpp. POSIX functions
# (setenv(), tzset()) come from their POSIX headers (<stdlib.h>, <time.h>).
ignore='bits/.*,nlohmann/json_fwd\.hpp'
status=0
for f in "${files[@]}"; do
	out=$(clang-include-cleaner -p "$db" "${mode[@]}" --ignore-headers="$ignore" "$f" 2>&1) || { echo "$f: $out" >&2; status=1; continue; }
	if [ -n "$out" ] && [ "${mode[0]}" = --print=changes ]; then
		echo "$f:"
		echo "$out" | sed 's/^/  /'
		status=1
	fi
done
exit $status
