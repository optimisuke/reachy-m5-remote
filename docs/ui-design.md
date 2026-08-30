# UI 設計: ダンス4種リモコン（マイルストーン1）

## 方針

こどもが一人で操作できることを最優先にした。

- **一度に一つだけ見せる。** 選択肢を並べず、選んでいるダンスを画面いっぱいに出す
- **色で区別する。** 文字が読めなくても緑・青・黄・赤で覚えられる
- **押したら必ず反応する。** 画面が一瞬白く光るので「効いたか分からない」が起きない
- **迷う余地を作らない。** ボタンは2つ、機能も2つ（つぎへ／ごー）
- **状態表示は隠す。** 接続状態は右上の小さな点だけ。大人が見る用

## 画面

### えらぶ画面

```
      ╭───────────╮
    ╱            ● ╲      ● 右上の小さな点 = 接続OK(緑) / NG(赤)
   │   ╭───────╮    │
   │   │ うんうん │    │     大きな色の丸 = いま選んでいるダンス
   │   ╰───────╯    │
   │   ● ○ ○ ○      │     4つのうちどれかを点で示す
    ╲               ╱
      ╰───────────╯
   きいろ→つぎ  あお→ごー
```

### おどってる画面

画面全体がそのダンスの色になる。`あお` で止められる。

```
      ╭───────────╮
    ╱   （緑一色）    ╲
   │   おどってる！    │
   │                │
   │   あお→とめる    │
    ╲               ╱
      ╰───────────╯
```

### つながらない画面

Wi-Fi・daemon・モーターのどこで失敗したかを小さく英字で出す（大人向け）。
`きいろ` で起動シーケンスをやり直す。

## ボタン割り当て

| ボタン | えらぶ画面 | おどってる画面 | つながらない画面 |
| --- | --- | --- | --- |
| きいろ KEYA (G2) | つぎのダンスへ | — | もういちど接続 |
| あお KEYB (G1) | ごー！（再生） | とめる | — |

長押し・同時押しは使わない。こどもには覚えられないため。

## ダンス4種

`pollen-robotics/reachy-mini-dances-library` の19種から、動きが見て分かるものを選んだ。
すべて音なし、1.82〜5.00秒。

| 色 | id | 画面の名前 |
| --- | --- | --- |
| 🟢 `0x22C55E` | `simple_nod` | うんうん |
| 🔵 `0x3B82F6` | `side_to_side_sway` | ゆらゆら |
| 🟡 `0xF59E0B` | `dizzy_spin` | くるくる |
| 🔴 `0xEF4444` | `chicken_peck` | つんつん |

追加・変更は `include/dances.h` の配列を編集するだけでよい。点の数も自動で追従する。

## 今回やらないこと

意図的に外した。マイルストーン2以降で足す。

- タッチ操作（`M5.Touch` で使えることは確認済み。ボタン2つで足りるので入れていない）
- 感情モーション85種のメニュー
- IMU 連動、ストップウォッチ機能
- 振動フィードバック（後述の未検証項目のため）

## ハードウェア対応状況（M5Unified のソースで確認済み）

M5Unified 0.2.21 / M5GFX 0.2.28 時点で StopWatch は正式サポートされている。
パネルやタッチの設定を手書きする必要はない。

| 項目 | 状況 | 根拠 |
| --- | --- | --- |
| ボード自動判別 | ○ | `board_M5StopWatch = 30`（M5GFX 0.2.26 以降）。I2C で CST820 の有無を見て判別 |
| 画面 | ○ `M5.Display` | `Panel_CO5300` 派生の `Panel_StopWatch`。468x468、`offset_x = 6`、QSPI |
| タッチ | ○ `M5.Touch` | `Touch_CST816S` を I2C `0x15`（SDA=G47 / SCL=G48）で初期化済み。CST820 互換 |
| ボタン | ○ `M5.BtnA` / `M5.BtnB` | `BtnA = GPIO2`（きいろ）、`BtnB = GPIO1`（あお） |
| スピーカー・マイク | ○ | ES8311 の電源制御を M5IOE1 経由で行う専用コールバックあり |
| 振動モーター | **API なし** | M5Unified に `vibration` / `motor` の実装が無い。M5IOE1 の空きピン経由の可能性（要検証） |

### PSRAM の設定が必須

M5GFX が起動時にこの警告を出す。

```
M5StopWatch: OPI-PSRAM is disabled; the display falls back to direct drawing,
which may render incorrectly when the drawing origin is at an odd coordinate.
```

ESP32-S3**R8** は OPI PSRAM なので `PSRAM=opi` が必要。これが無いと描画が崩れる。
arduino-cli の `m5stack:esp32:m5stack_stopwatch` は既定でそうなっているため、
ボードオプションを渡す必要はない。

## 実機での動作確認（マイルストーン1 達成）

書き込んで一通り動作した。シリアルログの実際の出力。

```
[http] GET /api/daemon/status -> 200 (753 bytes)
[robot] enabling motors
[http] POST /api/motors/set_mode/enabled -> 200
[boot] ready
[hb] screen=select dance=simple_nod wifi=3 ip=192.168.1.6 heap=260272
[ui] next
[ui] go side_to_side_sway
[http] POST /api/move/play/recorded-move-dataset/pollen-robotics%2F...%2Fside_to_side_sway -> 200
[http] GET /api/move/running -> 200 (49 bytes)   ← 再生中
[http] GET /api/move/running -> 200 (2 bytes)    ← [] になったので えらぶ画面へ戻る
```

確認できたこと。

- Wi-Fi 固定設定で接続、起動シーケンス（daemon status → motors enabled）が通る
- ボタン（`M5.BtnA` / `M5.BtnB`）が拾える。画面遷移も想定どおり
- ダンス再生 → 完了検知 → えらぶ画面に自動で戻る
- ヒープは 260KB 前後で安定。リークの兆候なし

### 解決した未検証項目

- **ESP32 の mDNS。** `reachy-mini.local:8000` で全リクエストが 200 を返した。
  引き継ぎドキュメントで懸念されていた不安定さは出ず、IP フォールバックは不要だった
- **`GET /api/move/running` の 800ms 間隔。** 問題なし。再生中は 49 バイト（uuid 入り）、
  終了後は 2 バイト（`[]`）が返り、遅延なく検知できる

## シリアルログ

起動ログはリセット直後に流れきってしまい、後からシリアルを繋いでも見えない。
そのため待機中も 5 秒ごとに現在の状態を出している。

```
[hb] screen=select dance=simple_nod wifi=3 ip=192.168.1.6 heap=260272
```

`screen` は boot / select / playing / trouble、`wifi` は `WiFi.status()`
（3 = `WL_CONNECTED`）。

## 未検証・要検証

- **振動モーターの駆動方法。** M5Unified に API が無い。M5IOE1（`M5.getIOExpander(0)`）の
  未使用ピンを叩く必要がありそう。`buzz()` は現状スタブ
- 日本語フォント `lgfxJapanGothic_*` の文字幅。円形画面の下部は横幅が狭いので、
  実機の目視で切れていないか確認する（ログでは分からない）
- タッチは使えると分かったが、UI に足すかは未決（ボタン2つで足りている）
