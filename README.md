# reachy-m5-remote

M5Stack StopWatch（ESP32-S3）から Reachy Mini を Wi-Fi 経由で操作するファームウェア。

Mac もクラウドも介さず、同じ LAN 内で StopWatch から本体の daemon の REST API を
直接叩く構成にする。

## いまできること

こども向けのダンスリモコン。ボタン2つだけで操作する。

- **きいろ (KEYA)** … つぎのダンスへ
- **あお (KEYB)** … ごー！（再生中は とめる）

ダンスは4種（うんうん / ゆらゆら / くるくる / つんつん）。選んでいるものを
円形画面いっぱいに色付きで表示する。設計は [`docs/ui-design.md`](docs/ui-design.md)。

## はじめに読むもの

- [`docs/ui-design.md`](docs/ui-design.md)
  UI 方針、画面、ボタン割り当て、ダンス4種、未検証項目
- [`docs/handoff-m5stack-controller.md`](docs/handoff-m5stack-controller.md)
  実機で確定した事実、使うエンドポイント、ハードウェア仕様、マイルストーン、
  実装スケッチ、ハマりどころ

関連リポジトリ: `hello-reachy-mini`（Reachy Mini 側の検証。daemon REST の OpenAPI 定義と
チートシートがある）

## 前提

- Reachy Mini（Wireless版）が同じネットワークにいること
- StopWatch の Wi-Fi は **2.4GHz のみ**対応
- ロボットの操作には daemon の backend が `running`、モーターが `enabled` である必要がある
  （詳細は上記ドキュメント）

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

ビルドと書き込みは PlatformIO。

```bash
pio run -t upload && pio device monitor
```

## 動作確認の順序

ファームウェアを書く前に、PC の curl で疎通を確認する。

```bash
curl http://reachy-mini.local:8000/api/daemon/status
curl -X POST "http://reachy-mini.local:8000/api/daemon/start?wake_up=false"
curl -X POST http://reachy-mini.local:8000/api/motors/set_mode/enabled
curl -X POST "http://reachy-mini.local:8000/api/move/play/recorded-move-dataset/pollen-robotics%2Freachy-mini-dances-library/simple_nod"
```
