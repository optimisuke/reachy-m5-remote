# UI 設計: ダンス4種リモコン

## 方針

こどもが一人で操作できることを最優先にした。

- **文字を使わない。** 小さい文字は円形画面では読みにくい。色と絵と位置だけで伝える
- **一度に一つだけ見せる。** 選択肢を並べず、選んでいるダンスを画面いっぱいに出す
- **色が識別子。** 緑・青・黄・赤で覚える。ダンス名は画面に出さない
- **押したら必ず反応する。** 画面が一瞬白く光るので「効いたか分からない」が起きない
- **触っても押しても同じことができる。** タッチとボタンのどちらでも完結する

唯一の例外は「つながらない」画面の下に小さく出す英字。これは大人が失敗箇所を
切り分けるためのもので、こどもの操作には関係ない。

## 操作

| したいこと | タッチ | ボタン |
| --- | --- | --- |
| つぎのダンスへ | **左へスワイプ** | きいろ KEYA (`M5.BtnA`) |
| ひとつ前のダンスへ | **右へスワイプ** | — |
| ごー！（再生） | **画面をタップ** | あお KEYB (`M5.BtnB`) |
| とめる | 再生中にタップ | あお KEYB |
| もういちど接続 | つながらない画面でタップ | どちらでも |

横に 50px 以上動いたらスワイプ、それ未満はタップとして扱う。スワイプの向きは
紙をめくる感覚に合わせた（左へ払う＝次へ進む）。長押しと同時押しは使わない。

## 画面

### えらぶ画面

```
      ╭───────────╮
    ╱ ╭───────────╮ ╲   ← 外周の細いリング（接続状態）
   │ ‹  ╭───────╮  › │   ← 左右の山形 = スワイプできる
   │    │   ↕   │    │   ← 大きな色の丸 ＋ 動きを表す絵
   │    ╰───────╯    │      丸ごとタップ範囲
   │    ● ○ ○ ○      │   ← 4つのうちどれか（選択中は その色）
    ╲               ╱
      ╰───────────╯
```

動きを表す絵は図形で描いている（`src/icons.cpp`）。

| 色 | ダンス | 絵 |
| --- | --- | --- |
| 🟢 `0x22C55E` | `simple_nod` | 縦の両矢印 ↕ |
| 🔵 `0x3B82F6` | `side_to_side_sway` | 横の両矢印 ↔ |
| 🟡 `0xF59E0B` | `dizzy_spin` | 丸い矢印 ↻ |
| 🔴 `0xEF4444` | `chicken_peck` | 点をつつく斜めの矢印 |

追加・変更は `include/dances.h` の配列を編集するだけでよい。点の数も自動で追従する。

### おどってる画面

画面全体がそのダンスの色になり、中央に ■（触ると止まる）。まわりのくるくるが
回って再生中だと分かる。再生が終わると自動でえらぶ画面に戻る。

### つながらない画面

丸に斜線の禁止マークと、やり直しを示す丸い矢印。その下に小さく失敗箇所の英字
（`wifi ng` / `daemon start ng` / `daemon timeout` / `motors ng` / `play ng`）。

### 外周のリング

以前は大きな丸の右上に小さな点を置いて接続状態を示していたが、何を意味するのか
分からないので**画面のいちばん外周の細いリング**に変えた。円形ディスプレイの縁を
そのまま使うので邪魔にならず、Wi-Fi が落ちたときだけ赤くなって気づける。

## 今回やらないこと

意図的に外した。マイルストーン2以降で足す。

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
- **タッチが動く**（`touch=1`）。スワイプとタップを判別できている

```
[touch] release at (59,135) dx=-185 -> swipe   → [ui] next -> dizzy_spin
[touch] release at (93,143) dx=-135 -> swipe   → [ui] next -> chicken_peck
```
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
- 絵の見え方。図形で描いているので文字切れの心配は無くなったが、こどもが
  「どのダンスか」を絵で区別できるかは実際に使ってもらわないと分からない
- スワイプのしきい値 50px が円形 466x466 に対して適切か（今は左スワイプが
  dx=-106〜-185 で拾えている）
