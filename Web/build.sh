#!/usr/bin/env bash
# Builds the browser client: Web/out/{index.html,client.js,client.wasm}.
# usage: Web/build.sh [-O0|-O2]   (from anywhere; needs C:/emsdk)
set -euo pipefail
OPT=${1:--O2}
HERE="$(cd "$(dirname "$0")" && pwd)"
CLIENT="$(cd "$HERE/.." && pwd)"
EM=/c/emsdk/upstream/emscripten
OBJ="${HB_WEB_OBJ:-$HERE/obj}$OPT"
OUT="$HERE/out"
mkdir -p "$OBJ" "$OUT"

# Same sources as Client.vcxproj, with the Win32 entry point and WinSock socket swapped for web versions.
SRC=(char/CharInfo.cpp char/Curse.cpp char/Magic.cpp char/Skill.cpp char/item/BuildItem.cpp char/item/Item.cpp
     char/item/ItemName.cpp DirectX/DXC_ddraw.cpp DirectX/DXC_dinput.cpp DirectX/DXC_dsound.cpp DirectX/Effect.cpp
     DirectX/Mydib.cpp DirectX/SoundBuffer.cpp DirectX/Sprite.cpp DirectX/TileSpr.cpp DirectX/YWSound.cpp
     Map/MapData.cpp Map/Tile.cpp UI/MouseInterface.cpp Net/Msg.cpp Game.cpp Misc.cpp StrTok.cpp
     Net/XSocket_web.cpp Web/WebMain.cpp Web/win32_web.cpp Web/ddraw_web.cpp Web/dsound_web.cpp Web/dinput_web.cpp)

CXXFLAGS=(-std=c++14 "$OPT" -w -fwasm-exceptions -fno-delete-null-pointer-checks -fms-extensions
          -DNDEBUG -D_WINDOWS -I"$CLIENT/Web/include")

compile() {
    local src=$1 obj="$OBJ/$(echo "$1" | tr '/' '_').o"
    # shortcut: any header newer than the object rebuilds it (no per-file dependency tracking)
    if [[ ! -f "$obj" || "$CLIENT/$src" -nt "$obj" || -n "${HB_REBUILD:-}" ||
          -n "$(find "$CLIENT" -name '*.h' -newer "$obj" -print -quit)" ]]; then
        "$EM/em++.exe" "${CXXFLAGS[@]}" -c "$CLIENT/$src" -o "$obj" 2>&1 | sed "s|^|[$src] |" || return 1
        [[ -f "$obj" ]] || return 1
    fi
}
export -f compile; export EM OBJ CLIENT; export CXXFLAGS_STR="${CXXFLAGS[*]}"
fail=0
for s in "${SRC[@]}"; do compile "$s" & done
wait || true
for s in "${SRC[@]}"; do [[ -f "$OBJ/$(echo "$s" | tr '/' '_').o" ]] || { echo "missing object for $s"; fail=1; }; done
[[ $fail == 0 ]] || exit 1

"$EM/em++.exe" "$OPT" -fwasm-exceptions $(for s in "${SRC[@]}"; do echo "$OBJ/$(echo "$s" | tr '/' '_').o"; done) \
    -lwebsocket.js -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=256MB -sSTACK_SIZE=4MB \
    -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$stringToNewUTF8,$UTF8ToString' -sEXPORTED_RUNTIME_METHODS=HEAPU8,HEAPU16,HEAP16 \
    --embed-file "$HERE/data/trebuchet16.hbf@/web/trebuchet16.hbf" \
    --shell-file "$HERE/shell.html" -o "$OUT/index.html"
ls -la "$OUT"
