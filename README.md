# reachy-m5-remote

M5Stack StopWatch（ESP32-S3）から Reachy Mini を Wi-Fi 経由で操作するファームウェア。

Mac もクラウドも介さず、同じ LAN 内で StopWatch から本体の daemon の REST API を
直接叩いている。

## いまできること

**こども向けのダンスリモコン。実機で動作確認済み。**

ボタン2つだけで操作する。文字が読めなくても色と位置で区別できるようにしてある。

- **きいろ (KEYA)** … つぎのダンスへ
- **あお (KEYB)** … ごー！（再生中は とめる）

選んでいるダンスを円形画面いっぱいの色の丸で1つだけ見せる。再生が終わると自動で
えらぶ画面に戻る。

| 色 | ダンス | 画面の名前 |
| --- | --- | --- |
| 🟢 緑 | `simple_nod` | うんうん |
| 🔵 青 | `side_to_side_sway` | ゆらゆら |
| 🟡 黄 | `dizzy_spin` | くるくる |
| 🔴 赤 | `chicken_peck` | つんつん |

ダンスの増減は [`include/dances.h`](include/dances.h) の配列を編集するだけでよい
（画面下の点の数も自動で追従する）。設計は [`docs/ui-design.md`](docs/ui-design.md)。

タッチ・IMU・ストップウォッチ機能・感情モーション85種は意図的に入れていない。

## はじめに読むもの

- [`AGENTS.md`](AGENTS.md)
  ビルド方法、必要なライブラリのバージョン、シリアルログの読み方
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
スケッチ構成へ一時展開してからビルドしている。使用量は Flash 70% / RAM 15%。

PlatformIO は使わない。公式の `espressif32` は Arduino コア 2.0.x で止まっていて
StopWatch の variant を引けないため。

## 動いているか確かめる

```bash
arduino-cli monitor -p /dev/cu.usbmodem1101 -c baudrate=115200 --raw
```

待機中も5秒ごとに状態を出しているので、後から繋いでも今の状態が分かる。

```
[hb] screen=select dance=simple_nod wifi=3 ip=192.168.1.6 heap=260272
```

`screen` は boot / select / playing / trouble、`wifi` は `WiFi.status()`（3 = 接続済み）。
`[boot]` は起動シーケンス、`[ui]` はボタン操作、`[http]` は REST の結果。

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
| `src/main.cpp` | UI と画面遷移、ボタン処理 |
| `src/robot.cpp` / `.h` | daemon REST（起動シーケンス、再生、停止、状態） |
| `include/dances.h` | ダンス4種の定義（id・表示名・色） |
| `include/secrets.h` | Wi-Fi とロボットの宛先。gitignore 済み |
| `tools/build.sh` | arduino-cli でビルド・書き込み |
