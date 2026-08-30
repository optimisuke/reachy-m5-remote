# AGENTS.md

M5Stack StopWatch（ESP32-S3）から Reachy Mini を操作するファームウェア。

## ビルドは arduino-cli を使う（PlatformIO ではない）

```bash
./tools/build.sh          # ビルドのみ
./tools/build.sh upload   # ビルドして書き込み（ポートは自動検出）
```

`tools/build.sh` が、PlatformIO 構成（`src/` と `include/`）を arduino-cli が要求する
スケッチ構成（フォルダ名と同名の空 `.ino` ＋ ソース平置き）へ一時展開してからビルドする。

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

### PlatformIO を使わない理由

`platformio.ini` は残してあるが**未検証の代替手段**。触る必要はない。

PlatformIO 公式の `espressif32` 6.12.0 が引くのは `framework-arduinoespressif32 ~3.20017.0`
= Arduino コア **2.0.17** で、StopWatch の variant が入った M5Stack コア 3.3.7 を引けない。
3.x コアを使うには pioarduino など非公式プラットフォームが必要になる。

## ビルド時の注意

- **`include/secrets.h` はコミットしない**（gitignore 済み）。無い場合
  `tools/build.sh` は `secrets.h.example` で代用してビルドを通す。これはダミー値なので
  書き込んでも繋がらない
- ビルド時に `NUM_DIGITAL_PINS redefined` 等の警告が大量に出るが、**M5Stack コア自身の
  `variants/m5stack_stopwatch/pins_arduino.h` と `Arduino.h` のマクロ重複**で上流の問題。
  自コードの警告と混ぜないよう `grep -v packages/m5stack` で絞ると見やすい
- 現状の使用量は Flash 70% / RAM 15%

## コード構成

| ファイル | 役割 |
| --- | --- |
| `src/main.cpp` | UI と画面遷移、ボタン処理 |
| `src/robot.cpp` / `.h` | Reachy Mini の daemon REST（起動シーケンス、再生、停止、状態） |
| `include/dances.h` | ダンス4種の定義（id・表示名・色）。増減させると点の数も自動追従 |
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
