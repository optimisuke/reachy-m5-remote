# AGENTS.md

M5Stack StopWatch（ESP32-S3）から Reachy Mini を操作するファームウェア。

## ビルドは arduino-cli を使う

```bash
./tools/build.sh          # ビルドのみ
./tools/build.sh upload   # ビルドして書き込み（ポートは自動検出）
```

ソースは `src/` と `include/` に分けてある。`tools/build.sh` が、これを arduino-cli が
要求するスケッチ構成（フォルダ名と同名の空 `.ino` ＋ ソース平置き）へ一時展開して
からビルドする。リポジトリ側の構成は崩さない。

FQBN は `m5stack:esp32:m5stack_stopwatch`。M5Stack コア 3.3.7 にこの variant があり、
**PSRAM=opi / 16MB Flash / qio が既定で正しい**ので、ボードオプションを渡す必要はない。

### 必要なもの

| | バージョン | 備考 |
| --- | --- | --- |
| `m5stack:esp32` コア | 3.3.7 以降 | `m5stack_stopwatch` variant が入っている |
| M5Unified | **0.2.21 以降** | StopWatch 対応がこのバージョンで入った |
| M5GFX | **0.2.28 以降** | 同上。M5Unified の依存で入る |
| ArduinoJson | 7.x | |

```bash
arduino-cli core install m5stack:esp32
arduino-cli lib install M5Unified@0.2.21
arduino-cli lib install ArduinoJson
```

## ビルド時の注意

- **`include/secrets.h` はコミットしない**（gitignore 済み）。無い場合
  `tools/build.sh` は `secrets.h.example` で代用してビルドを通す。これはダミー値なので
  書き込んでも繋がらない
- ビルド時に `NUM_DIGITAL_PINS redefined` 等の警告が大量に出るが、**M5Stack コア自身の
  `variants/m5stack_stopwatch/pins_arduino.h` と `Arduino.h` のマクロ重複**で上流の問題。
  自コードの警告と混ぜないよう `grep -v packages/m5stack` で絞ると見やすい
- 現状の使用量は Flash 39% / RAM 15%

## UI は Mac のプレビューで詰める

**UI をいじるたびに実機へ書き込む必要はない。**

```bash
./tools/preview/build.sh              # ウィンドウを開いて操作できる
./tools/preview/build.sh --shot out/  # 全画面を PNG に書き出して終了
./tools/preview/build.sh --scale 2    # 2倍に拡大
```

実機と同じ描画コード（`src/ui.cpp` と `src/icons.cpp`）を M5GFX の SDL バックエンドで
動かしている。M5GFX には SDL 対応が同梱されていて、`SDL.h` を先に読ませると
`#if defined(SDL_h_)` の分岐が有効になる。

- マウスのドラッグ＝スワイプ、クリック＝タップ。キーは A=きいろ、B/スペース=あお、
  T=つながらない画面、1〜4=ダンス選択、S=PNG 保存
- `--shot` で書き出した PNG を読めば、目で見て詰められる
- **arm64 の sdl2 が必要**（`brew install sdl2`）。Homebrew が `/usr/local` と
  `/opt/homebrew` の両方にある場合、`/opt/homebrew` 側でないとアーキテクチャが合わない
- ビルドから `M5GFX.cpp` と `platforms/esp32*` は外す（ESP-IDF に依存するため）

### 描画コードを書くときの制約

`src/ui.cpp` と `src/icons.cpp` は **M5Unified・WiFi・HTTPClient に依存させない**。
プレビューが M5Unified を使えないため（M5Unified に SDL 対応は無い）。型は
`src/gfx.h` の `Gfx`（= `lgfx::LovyanGFX`）を使う。状態は呼び出し側が持ち、
`ui::State` で渡す。

- `BLACK` / `WHITE` のような名前は Arduino 側の定義と衝突する。`COL_` を付ける
- **`drawArc` は輪郭線しか描かない。** 太い帯にしたいときは `fillArc`
- `readRect` を `uint8_t*` で呼ぶと 8bit カラー扱いになる。`lgfx::rgb888_t*` を使う

## シリアルログの読み方

```bash
arduino-cli monitor -p /dev/cu.usbmodem1101 -c baudrate=115200 --raw
```

**起動ログはリセット直後に流れきる。** 書き込み直後にモニタを繋いでも `[boot]` 行は
見えない。さらに `esptool` でリセットをかけると USB CDC が再列挙されてモニタの
ハンドルが切れるので、その手も使えない。代わりに待機中も 5 秒ごとに状態を出している。

```
[hb] screen=select dance=simple_nod touch=1 wifi=3 ip=192.168.1.6 heap=260252
```

`screen` は boot / select / playing / trouble、`wifi` は `WiFi.status()`（3 = 接続済み）。
`[boot]` は起動シーケンス、`[touch]` はタッチの生の判定、`[ui]` は確定した操作
（タッチ・ボタンのどちらからでも来る）、`[http]` は REST の結果。

## コード構成

| ファイル | 役割 |
| --- | --- |
| `src/main.cpp` | UI と画面遷移、入力処理（タッチとボタン） |
| `src/icons.cpp` / `.h` | 動きを表す絵などの図形描画。**UI に文字を使わない方針** |
| `src/robot.cpp` / `.h` | Reachy Mini の daemon REST（起動シーケンス、再生、停止、状態） |
| `include/dances.h` | ダンス4種の定義（id・色・絵）。増減させると点の数も自動追従 |
| `include/secrets.h` | Wi-Fi とロボットの宛先。gitignore 済み |

## ドキュメント

- `docs/ui-design.md` — UI 方針、画面、ボタン割り当て、ハードウェア対応状況、未検証項目
- `docs/handoff-m5stack-controller.md` — 実機で確定した事実、エンドポイント、
  ハードウェア仕様、マイルストーン、ハマりどころ

**実機で確定した事実はこの2つに書いてある。調べ直す前にまず読む。**

## 気をつけること

- **Wi-Fi の SSID・パスワードをソースやドキュメントに書かない**
- Reachy Mini の daemon に**認証は無い**。同じ LAN の誰でも操作できる
- 角度の単位は**ラジアン**
- データセット名のスラッシュは **`%2F`** にエンコードする
- モーション再生は非ブロッキング。完了確認は `GET /api/move/running`
- 事実と推測を区別し、未確認のことは「要検証」と明記する
