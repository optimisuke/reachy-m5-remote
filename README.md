# reachy-m5-remote

M5Stack StopWatch（ESP32-S3）から Reachy Mini を Wi-Fi 経由で操作するファームウェア。

Mac もクラウドも介さず、同じ LAN 内で StopWatch から本体の daemon の REST API を
直接叩く構成にする。

## はじめに読むもの

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

```cpp
// include/secrets.h
#pragma once
#define WIFI_SSID     "your-ssid"
#define WIFI_PASSWORD "your-password"
```

## 動作確認の順序

ファームウェアを書く前に、PC の curl で疎通を確認する。

```bash
curl http://reachy-mini.local:8000/api/daemon/status
curl -X POST "http://reachy-mini.local:8000/api/daemon/start?wake_up=false"
curl -X POST http://reachy-mini.local:8000/api/motors/set_mode/enabled
curl -X POST "http://reachy-mini.local:8000/api/move/play/recorded-move-dataset/pollen-robotics%2Freachy-mini-dances-library/simple_nod"
```
