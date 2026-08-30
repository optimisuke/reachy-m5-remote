#!/usr/bin/env bash
# Mac 上で UI プレビューをビルドして起動する。
#
# 実機と同じ描画コード（src/ui.cpp と src/icons.cpp）を M5GFX の SDL バックエンドで
# 動かすので、UI をいじるたびに実機へ書き込む必要がない。
#
#   ./tools/preview/build.sh                    ビルドして起動
#   ./tools/preview/build.sh --shot out/        全画面を PNG に書き出して終了
#   ./tools/preview/build.sh --scale 2          2倍に拡大して起動
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${TMPDIR:-/tmp}/reachy-m5-preview"
SDL="$(/opt/homebrew/bin/brew --prefix)/opt/sdl2"
M5GFX="$(arduino-cli config get directories.user)/libraries/M5GFX/src"

[ -d "$SDL/include/SDL2" ] || { echo "error: arm64 の sdl2 が無い。brew install sdl2" >&2; exit 1; }
[ -d "$M5GFX" ] || { echo "error: M5GFX が見つからない: $M5GFX" >&2; exit 1; }

# M5GFX.cpp は ESP-IDF に依存するので外す。esp32 系のプラットフォームも要らない。
# SDL.h を先に読ませることで、M5GFX 側の SDL 対応（#if defined(SDL_h_)）が有効になる。
mkdir -p "$OUT"
g++ -std=c++17 -O1 -w -include SDL.h \
    -I"$ROOT/include" -I"$ROOT/src" \
    -I"$M5GFX" -I"$SDL/include" -I"$SDL/include/SDL2" \
    "$ROOT/tools/preview/main.cpp" "$ROOT/src/ui.cpp" "$ROOT/src/icons.cpp" \
    $(find "$M5GFX/lgfx" -name "*.cpp" | grep -vE "/esp32|/framebuffer/") \
    $(find "$M5GFX/lgfx" -name "*.c") \
    -L"$SDL/lib" -lSDL2 -o "$OUT/preview"

exec "$OUT/preview" "$@"
