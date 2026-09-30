#!/usr/bin/env bash
# Build scratch copies of rogue++ for replays, each with the stairs check of
# d_level() patched out so that '>' works anywhere:
#
#   DIR/base   the committed tree at REF (default: main)
#   DIR/new    the working tree
#   DIR/asan   the working tree with AddressSanitizer and UBSan
#
# usage: tools/replay/make-trees.sh DIR [REF]
# The binaries are DIR/{base,new,asan}/build/rogue++.
set -euo pipefail

dir=$(realpath -m "${1:?usage: make-trees.sh DIR [REF]}")
ref=${2:-main}
repo=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
json="$repo/build/_deps/nlohmann_json-src"

rm -rf "$dir/base" "$dir/new" "$dir/asan"
mkdir -p "$dir/base" "$dir/new" "$dir/asan"
git -C "$repo" archive "$ref" | tar -x -C "$dir/base"
for t in new asan; do
	cp -r "$repo/CMakeLists.txt" "$repo/src" "$repo/tests" "$dir/$t/"
done

for t in base new asan; do
	# d_level() is in misc.cpp up to phase 13.1, then in game/PlayerCommands.cpp
	misc="$dir/$t/src/misc.cpp"
	[ -f "$misc" ] || misc="$dir/$t/src/game/PlayerCommands.cpp"
	# d_level()'s test, before and after phase 10.7 removed chat() and hero
	sed -i -e 's/if (chat(hero.y, hero.x) != STAIRS)/if (false)/' \
		-e 's/if (game().level.at(player.body.t_pos) != STAIRS)/if (false)/' "$misc"
	grep -q 'if (false)' "$misc" || { echo "the stairs patch no longer applies to $misc" >&2; exit 1; }
done

# Reuse the nlohmann/json the main build fetched, if there is one
common=(-DROGUE_BUILD_TESTS=OFF)
[ -d "$json" ] && common+=("-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=$json")
san="-fsanitize=address,undefined -fno-omit-frame-pointer"

cmake -S "$dir/base" -B "$dir/base/build" "${common[@]}" >/dev/null
cmake -S "$dir/new" -B "$dir/new/build" "${common[@]}" >/dev/null
cmake -S "$dir/asan" -B "$dir/asan/build" "${common[@]}" -DCMAKE_BUILD_TYPE=Debug \
	"-DCMAKE_CXX_FLAGS=$san -O1" "-DCMAKE_EXE_LINKER_FLAGS=$san" >/dev/null
# One tree at a time, on at most JOBS cores (default 8)
for t in base new asan; do
	cmake --build "$dir/$t/build" -j "${JOBS:-8}" --target rogue++
done
ls "$dir"/{base,new,asan}/build/rogue++
