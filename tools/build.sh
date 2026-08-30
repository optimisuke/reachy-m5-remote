#!/usr/bin/env bash
# arduino-cli でビルドする。
#
# このリポジトリは PlatformIO のディレクトリ構成（src/ と include/）なので、
# arduino-cli が要求するスケッチ構成（フォルダ名と同名の .ino、ソースは平置き）に
# 一時ディレクトリへ展開してからビルドする。
#
#   ./tools/build.sh          ビルドのみ
#   ./tools/build.sh upload   ビルドして書き込み（ポートは自動検出）
set -euo pipefail

FQBN="m5stack:esp32:m5stack_stopwatch"   # PSRAM=opi、16MB Flash が既定
SKETCH="reachy_m5_remote"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
STAGE="${TMPDIR:-/tmp}/${SKETCH}"

rm -rf "$STAGE" && mkdir -p "$STAGE"
: > "$STAGE/${SKETCH}.ino"               # setup()/loop() は main.cpp 側にある
cp "$ROOT"/src/*.cpp "$ROOT"/src/*.h "$STAGE"/
cp "$ROOT"/include/*.h "$STAGE"/ 2>/dev/null || true

if [ -f "$ROOT/include/secrets.h" ]; then
    cp "$ROOT/include/secrets.h" "$STAGE"/
else
    # 実機に書き込まないビルド確認用。ダミーなので繋がらない。
    echo "warn: include/secrets.h が無いので secrets.h.example でビルドする" >&2
    cp "$ROOT/include/secrets.h.example" "$STAGE/secrets.h"
fi

if [ "${1:-}" = "upload" ]; then
    PORT="$(arduino-cli board list --json \
        | python3 -c 'import json,sys; d=json.load(sys.stdin); print(next((p["port"]["address"] for p in d.get("detected_ports",[]) if "usb" in p["port"]["protocol"]), ""))')"
    [ -n "$PORT" ] || { echo "error: ポートが見つからない" >&2; exit 1; }
    echo "uploading to $PORT"
    arduino-cli compile -b "$FQBN" -u -p "$PORT" "$STAGE"
else
    arduino-cli compile -b "$FQBN" --warnings default "$STAGE"
fi
