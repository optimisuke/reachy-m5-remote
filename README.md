# reachy-m5-remote

M5Stack StopWatch（ESP32-S3）から Reachy Mini を Wi-Fi 経由で操作するファームウェア。

Mac もクラウドも介さず、同じ LAN 内で StopWatch から本体の daemon の REST API を
直接叩いている。

## いまできること

**こども向けのダンスリモコン。実機で動作確認済み。**

ひらがな・絵・色の三重で伝えるので、どれか一つ分かれば操作できる。小さい文字は
円形画面では読みにくいので使わず、操作は左右の山形と下の点で示している。
タッチとボタンのどちらでも完結する。

| したいこと | タッチ | ボタン |
| --- | --- | --- |
| つぎのダンスへ | 左へスワイプ | きいろ (KEYA) |
| ひとつ前のダンスへ | 右へスワイプ | — |
| ごー！（再生） | 画面をタップ | あお (KEYB) |
| とめる | 再生中にタップ | あお (KEYB) |

選んでいるダンスを円形画面いっぱいの色の丸で1つだけ見せる。丸の中に動きを表す絵と
ひらがなの名前を置いている。再生が終わると自動でえらぶ画面に戻る。

| 色 | ダンス | 名前 | 絵 |
| --- | --- | --- | --- |
| 🟢 緑 | `simple_nod` | うんうん | 縦の両矢印 ↕ |
| 🔵 青 | `side_to_side_sway` | ゆらゆら | 横の両矢印 ↔ |
| 🟡 黄 | `dizzy_spin` | くるくる | 丸い矢印 ↻ |
| 🔴 赤 | `chicken_peck` | つんつん | 点をつつく斜めの矢印 |

ダンスの増減は [`include/dances.h`](include/dances.h) の配列を編集するだけでよい
（画面下の点の数も自動で追従する）。

### 画面は4つだけ

| 画面 | 見え方 |
| --- | --- |
| えらぶ | 色の丸に絵とひらがな。左右に山形、下に4つの点 |
| おどってる | 全面がそのダンスの色。縁を白い帯が回り、中央に ■（触ると止まる） |
| つながらない | 赤い禁止マークと丸い矢印。下に小さく失敗箇所の英字 |
| じゅんびちゅう | 灰色の輪の上を白い帯が回る |

画面のいちばん外周に細いリングがあり、Wi-Fi が落ちると赤くなる（大人向けの目印）。

設計の詳細と実機での確認結果は [`docs/ui-design.md`](docs/ui-design.md)。
IMU・ストップウォッチ機能・感情モーション85種は意図的に入れていない。

## はじめに読むもの

- [`AGENTS.md`](AGENTS.md)
  ビルド方法、必要なライブラリのバージョン、Mac プレビュー、シリアルログの読み方、
  描画コードを書くときの制約
- [`docs/ui-design.md`](docs/ui-design.md)
  UI 方針、画面、ボタン割り当て、ハードウェア対応状況、実機確認の結果、未検証項目
- [`docs/handoff-m5stack-controller.md`](docs/handoff-m5stack-controller.md)
  実機で確定した事実、使うエンドポイント、ハードウェア仕様、マイルストーン、
  ハマりどころ

関連リポジトリ: `hello-reachy-mini`（Reachy Mini 側の検証。daemon REST の OpenAPI 定義と
チートシートがある）

## 前提

- Reachy Mini（Wireless版）が同じネットワークにいること
- StopWatch の Wi-Fi は **2.4GHz のみ**対応
- ロボットの操作には daemon の backend が `running`、モーターが `enabled` である必要がある
  → 起動時に自動で面倒を見ている（`src/robot.cpp` の `ensureReady()`）
- daemon に**認証は無い**。同じ LAN の誰でも操作できるので公開ネットワークでは使わない

## セットアップ

Wi-Fi の認証情報は `include/secrets.h` に置く。このファイルは gitignore してある。

```bash
cp include/secrets.h.example include/secrets.h
```

```cpp
// include/secrets.h
#pragma once
#define WIFI_SSID     "your-ssid"
#define WIFI_PASSWORD "your-password"
#define ROBOT_HOST    "http://reachy-mini.local:8000"
```

`reachy-mini.local` の mDNS 解決は実機で問題なく動いた。繋がらない場合は IP に
書き換える（DHCP なので変わりうる）。

## ビルドと書き込み

**arduino-cli** を使う。M5Stack コア 3.3.7 に StopWatch のボード定義
（`m5stack:esp32:m5stack_stopwatch`）が入っていて、PSRAM=opi などの設定が既定で正しい。

```bash
arduino-cli core install m5stack:esp32          # 3.3.7 以降
arduino-cli lib install M5Unified@0.2.21        # StopWatch 対応は 0.2.21 以降
arduino-cli lib install ArduinoJson

./tools/build.sh          # ビルドのみ
./tools/build.sh upload   # 書き込み（ポートは Espressif の VID で自動検出）
```

ソースは `src/` と `include/` に分けてあるので、`tools/build.sh` が arduino-cli 用の
スケッチ構成へ一時展開してからビルドしている。使用量は Flash 56% / RAM 15%。

PlatformIO は使わない。公式の `espressif32` は Arduino コア 2.0.x で止まっていて
StopWatch の variant を引けないため。

## UI を Mac で確認する

実機に書き込まずに、同じ描画コードを Mac のウィンドウで動かせる。

```bash
brew install sdl2                     # 初回だけ（arm64 側の Homebrew で）
./tools/preview/build.sh              # ウィンドウを開いて操作できる
./tools/preview/build.sh --shot out/  # 全画面を PNG に書き出して終了
```

マウスのドラッグ＝スワイプ、クリック＝タップ。キーは A=きいろ、B/スペース=あお、
T=つながらない画面、I=絵ありと絵なしの切り替え、1〜4=ダンス選択、S=PNG 保存。

M5GFX には SDL バックエンドが同梱されているので、`src/ui.cpp` と `src/icons.cpp` を
そのまま Mac で動かしている。`--shot` で書き出した PNG を見ながら詰められる。

なお **M5GFX の日本語フォントは `lgfxJapanGothic_40` が最大**で、`setTextSize()` で
拡大するとビットマップなのでギザギザになる。これより大きくしたい場合はフォントを
追加する必要がある。

## 動いているか確かめる

```bash
arduino-cli monitor -p /dev/cu.usbmodem1101 -c baudrate=115200 --raw
```

待機中も5秒ごとに状態を出しているので、後から繋いでも今の状態が分かる。

```
[hb] screen=select dance=simple_nod touch=1 wifi=3 ip=192.168.1.6 heap=260252
[touch] release at (59,135) dx=-185 -> swipe
[ui] next -> dizzy_spin
```

`screen` は boot / select / playing / trouble、`wifi` は `WiFi.status()`（3 = 接続済み）。
`[boot]` は起動シーケンス、`[touch]` はタッチ、`[ui]` は確定した操作、
`[http]` は REST の結果。

## うまく動かないときの切り分け

ESP32 側の問題かロボット側の問題かを分けるため、PC の curl で疎通を確認する。

```bash
curl http://reachy-mini.local:8000/api/daemon/status
curl -X POST "http://reachy-mini.local:8000/api/daemon/start?wake_up=false"
curl -X POST http://reachy-mini.local:8000/api/motors/set_mode/enabled
curl -X POST "http://reachy-mini.local:8000/api/move/play/recorded-move-dataset/pollen-robotics%2Freachy-mini-dances-library/simple_nod"
```

画面が「つながらない」になったときは、小さく出ている英字が失敗した場所を示している
（`wifi ng` / `daemon start ng` / `daemon timeout` / `motors ng` / `play ng`）。
`きいろ` で起動シーケンスをやり直せる。

## ファイル構成

| ファイル | 役割 |
| --- | --- |
| `src/main.cpp` | 入力（タッチとボタン）、状態遷移、通信の呼び出し |
| `src/ui.cpp` / `.h` | 画面の描画。実機と Mac プレビューで共用 |
| `src/icons.cpp` / `.h` | 動きを表す絵・くるくる・禁止マークなどの図形描画 |
| `src/gfx.h` | 実機（M5Unified）と Mac（M5GFX+SDL）で描画型を揃える |
| `src/robot.cpp` / `.h` | daemon REST（起動シーケンス、再生、停止、状態） |
| `include/dances.h` | ダンス4種の定義（id・ひらがな名・色・絵） |
| `include/secrets.h` | Wi-Fi とロボットの宛先。gitignore 済み |
| `tools/build.sh` | arduino-cli でビルド・書き込み |
| `tools/preview/` | Mac で UI を確認するプレビュー |

`ui.cpp` と `icons.cpp` は M5Unified・Wi-Fi・HTTP に依存させていない。Mac の
プレビューでそのまま動かすため（M5Unified に SDL 対応が無い）。
