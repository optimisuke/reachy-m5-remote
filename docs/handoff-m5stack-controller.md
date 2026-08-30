# 引き継ぎプロンプト: M5Stack StopWatch から Reachy Mini を操作する

別フォルダで作業を始めるときに、このファイルの内容をそのまま貼って使う。

推奨フォルダ名: **`reachy-m5-remote`**

```bash
mkdir -p ~/Private/reachy-m5-remote/docs
cp ~/Private/hello-reachy-mini/docs/handoff-m5stack-controller.md ~/Private/reachy-m5-remote/docs/
cd ~/Private/reachy-m5-remote && git init
```

別案: `m5-reachy-remote`（デバイス名を先に）、`hello-reachy-m5stack`（既存リポジトリの
`hello-` に合わせる）。`reachy-` 始まりにすると `hello-reachy-mini` の隣に並ぶ。

---

## ここから下がプロンプト本文

M5Stack StopWatch（ESP32-S3）から Reachy Mini を Wi-Fi 経由で操作するファームウェアを
作りたい。以下は前提として確定している事実なので、調べ直さずに使ってよい。

### ゴール

StopWatch のボタン・タッチ・IMU で Reachy Mini を動かす。Mac もクラウドも介さず、
同じ LAN 内で StopWatch から直接ロボットへ HTTP を投げる。

### 確定事項1: 通信方式は daemon の REST を直接叩く

Reachy Mini 本体の daemon が REST API を公開している（**全100オペレーション**）。
プロキシサーバーを立てる必要はない。実機で動作確認済み。

- ベースURL: `http://reachy-mini.local:8000`
- IP: `192.168.1.247`（DHCP なので変わる可能性あり。mDNS 優先、IP をフォールバックに）
- ブラウザで一覧を見られる: `/docs`（Swagger UI・その場で試せる）、`/redoc`（俯瞰用）
- **認証は無い。** 同じネットワークにいれば誰でも操作できる
- **角度の単位はラジアン。** 度で扱うなら `deg * PI / 180`
- OpenAPI 定義の控えは `hello-reachy-mini` リポジトリの `docs/daemon-openapi.yaml`、
  よく使う操作は `docs/daemon-rest-cheatsheet.md` にある

Reachy Mini の **App（Reachy Mini Control 上のアプリ）は起動しなくてよい**。ただし次項の
初期化が必要。

### 確定事項2: 起動時に状態を確認する処理を入れる（重要）

**通常は何もしなくてよい。** 本体の電源投入後、daemon の backend は `running` で
始まるので、いきなり `/api/move/...` を叩いて動く。

ただし **Reachy Mini Control などから daemon を明示的に停止すると `stopped` になり、
その状態では無反応になる**（自動で止まるわけではなく、人が止めたときに起きる）。
ESP32 側では状態を見て必要なら起動する、という条件付き処理を入れておくと安全。
冪等なので毎回通しても害はない。

また**本体を再起動するとモーターのトルクは `disabled` から始まる**ため、
`set_mode/enabled` は毎回実行する価値がある。

```
1. GET  /api/daemon/status                  → state を確認
2. POST /api/daemon/start?wake_up=false     → state が "stopped" なら backend を起動
   （wake_up=true にすると起床モーションと音が入る）
3. POST /api/motors/set_mode/enabled        → トルクON
```

観測した罠。

- daemon プロセスは常時動いていても、その中の **backend が `stopped` だと操作できない**。
  その状態では `/api/move/goto` が **HTTP 503 `Backend not running`** を返す
  （実際に、ユーザーが Reachy Mini Control 側で停止していたときに遭遇した）
- backend が `running` でも、モーターが `disabled` だと **`goto` は uuid を返すのに
  ロボットは動かない**（成功したように見えるので気づきにくい）
- `POST /api/daemon/start` は `wake_up` クエリパラメータが**必須**。無いと 422
- 起動には数秒かかる。`/api/daemon/status` の `state` が `running` になるまで待つ

正常時の `/api/daemon/status` の見どころ。

```json
{"state":"running",
 "backend_status":{"ready":true,"motor_control_mode":"enabled",
   "control_loop_stats":{"mean_control_loop_frequency":49.6}},
 "wlan_ip":"192.168.1.247","version":"1.10.0"}
```

### 確定事項3: 使うエンドポイント

```
# ダンス・感情モーションの再生（ボディ不要。データセット名の / は %2F にエンコード）
POST /api/move/play/recorded-move-dataset/pollen-robotics%2Freachy-mini-dances-library/{move}
POST /api/move/play/recorded-move-dataset/pollen-robotics%2Freachy-mini-emotions-library/{move}
     → {"uuid":"..."} を返す非ブロッキング方式

# 姿勢を動かす（4x4行列は不要。指定した項目だけ書けばよく、他は既定0.0）
POST /api/move/goto
     {"head_pose":{"roll":0,"pitch":-0.3,"yaw":0},"antennas":[0.7,-0.7],
      "body_yaw":0.0,"duration":1.0,"interpolation":"minjerk"}
     interpolation: linear / minjerk（既定）/ ease_in_out / cartoon

# 連続制御（補間なし。IMU連動などで毎フレーム送る用）
POST /api/move/set_target
     {"target_head_pose":{"pitch":-0.2},"target_antennas":[0.3,-0.3],"target_body_yaw":0.0}

# 再生状態と停止
GET  /api/move/running                 → 再生中は [{"uuid":"..."}]、アイドルは []
POST /api/move/stop                    → {"uuid":"..."} が必須

# 状態
GET  /api/state/full                   → 姿勢・アンテナ・胴体・制御モード・音源方向

# モーター
POST /api/motors/set_mode/{enabled|disabled|gravity_compensation}
GET  /api/motors/status

# 音・顔追跡
POST /api/media/play_sound             → {"file":"wake_up.wav"}
GET  /api/media/sounds                 → 音源一覧
POST /api/media/tracking/enable        → {"weight":1.0}
POST /api/media/tracking/disable
GET  /api/media/tracking/face
POST /api/media/wobbling/enable        → 音に合わせて頭が揺れる

# 音量
POST /api/volume/set                   → {"volume":60}  (0-100)

# 起床・就寝
POST /api/move/play/wake_up
POST /api/move/play/goto_sleep
```

### 確定事項4: 再生できるモーション

daemon 起動時にプリダウンロード済みなので、追加設定なしで即再生できる。

**ダンス 19種**（すべて 1.82〜5.00秒、**音なし**）

```
yeah_nod, chin_lead, dizzy_spin, neck_recoil, pendulum_swing, interwoven_spirals,
sharp_side_tilt, polyrhythm_combo, side_to_side_sway, side_glance_flick, chicken_peck,
simple_nod, side_peekaboo, stumble_and_recover, groovy_sway_and_roll, grid_snap,
jackson_square, head_tilt_roll, uh_huh_tilt
```

**感情 85種**（**84種は音付き**。2.14〜19.76秒）。よく使いそうなもの:

```
laughing1, laughing2, proud1, proud2, proud3, amazed1, surprised1, surprised2,
cheerful1, enthusiastic1, success1, success2, grateful1, welcoming1, loving1,
thoughtful1, curious1, confused1, inquiring1, attentive1,
sad1, downcast1, lonely1, tired1, exhausted1, boredom1, impatient1, irritated1,
rage1, furious1, scared1, oops1, no1, yes1, dance1, dance2, dance3,
sleep1, mini-deep-sleep, wake-mini-up, toc-toc-toc, waiting（音なし）
```

一覧は `GET /api/move/recorded-move-datasets/list/{dataset_name}` でも取れる。

### 確定事項5: M5Stack StopWatch のハードウェア（SKU: C152）

公式ドキュメント（https://docs.m5stack.com/en/core/StopWatch ）より。

| 項目 | 内容 |
| --- | --- |
| SoC | ESP32-S3R8（LX7 デュアルコア 240MHz） |
| メモリ | 16MB Flash + 8MB PSRAM |
| 無線 | **2.4GHz Wi-Fi のみ**（5GHz 不可） |
| ディスプレイ | 1.75インチ **円形 AMOLED 466x466**（CO5300、QSPI） |
| タッチ | 静電容量式（CST820B）SDA=G47 SCL=G48 INT=G13 |
| ボタン | **KEYA=G2（黄）、KEYB=G1（青）** ＋ 電源ボタン |
| IMU | 6軸 BMI270（I2C 0x68） |
| RTC | RX8130CE（0x32） |
| 音 | ES8311 コーデック + MEMSマイク + AW8737A アンプ + スピーカー |
| 触覚 | **振動モーター内蔵** |
| 電源 | 450mAh バッテリー、USB Type-C |
| 拡張 | Grove PORT.A（G10=黄 / G11=白）、背面拡張バス |
| サイズ | 52 x 52 x 15.5mm / 39g |

開発環境は Arduino IDE（M5Unified、M5GFX、M5PM1、M5IOE1）、PlatformIO、UiFlow2、
ESP-IDF が使える。PlatformIO の設定例は `board = esp32s3box`、
`platform = espressif32 @ 6.12.0`、framework arduino、16MB パーティション。

注意: ラベルに `BAT` と誤記されたロットがあるが実際は **5V 入力**。リチウム電池を
つないではいけない。

### 開発方針

**PlatformIO + Arduino フレームワーク**で進める。ライブラリは M5Unified（ボタン・
ディスプレイ・IMU・スピーカーを統一的に扱える）、`HTTPClient`、`ArduinoJson`。

Wi-Fi の SSID とパスワードは**ソースに直書きしない**。`platformio.ini` の
`build_flags` か、gitignore した別ヘッダに置く。

### 進め方（この順で作る）

**マイルストーン1: 疎通確認**

1. Wi-Fi 接続 → 画面に IP を表示
2. `GET /api/daemon/status` を叩き、`state` を画面に出す
3. 起動シーケンス（daemon start → motors enabled）を実行し、結果を画面に出す
4. KEYA を押したら `simple_nod` を再生する

ここまでで「ボタンでロボットが動く」が達成できる。

**マイルストーン2: 操作を増やす**

- KEYB で感情モーションをランダム再生（85種から選ぶ）
- 長押しで `goto_sleep`、電源投入時に `wake_up`
- 円形 AMOLED にモーション名を表示、再生中はインジケータを出す
- 再生中に押されたら `POST /api/move/stop` で中断できるようにする

**マイルストーン3: StopWatch らしい機能**

- ストップウォッチ／タイマーとして動かし、経過時間に応じてロボットが反応する
  - 待たされている間は `impatient1`、`boredom1`
  - 完了したら `success1` や `dance1`
- ポモドーロタイマー化して、休憩開始時に踊る

**マイルストーン4: センサー連動**

- IMU（BMI270）で StopWatch の傾きを取り、`POST /api/move/set_target` で頭を追従させる
  （補間なしの直接指定なので、20〜30Hz 程度で送る。送りすぎに注意）
- タッチスクリーンで感情モーションの一覧から選べるメニューを作る
- 振動モーターで、モーション開始・完了をフィードバックする

### 実装スケッチ

```cpp
#include <M5Unified.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ROBOT = "http://reachy-mini.local:8000";
const char* DANCES = "pollen-robotics%2Freachy-mini-dances-library";
const char* EMOTIONS = "pollen-robotics%2Freachy-mini-emotions-library";

int post(const String& path, const String& body = "") {
    HTTPClient http;
    http.begin(String(ROBOT) + path);
    http.setTimeout(8000);                       // 起動系は数秒かかる
    if (body.length()) http.addHeader("Content-Type", "application/json");
    int code = http.POST(body);
    http.end();
    return code;
}

String get(const String& path) {
    HTTPClient http;
    http.begin(String(ROBOT) + path);
    http.setTimeout(8000);
    String out;
    if (http.GET() == 200) out = http.getString();
    http.end();
    return out;
}

// 起動シーケンス。これを踏まないと無反応のまま原因が分からない。
bool ensureReady() {
    String st = get("/api/daemon/status");
    if (st.indexOf("\"state\":\"running\"") < 0) {
        post("/api/daemon/start?wake_up=false");
        for (int i = 0; i < 20; i++) {            // 起動待ち（最大10秒）
            delay(500);
            if (get("/api/daemon/status").indexOf("\"state\":\"running\"") >= 0) break;
        }
    }
    return post("/api/motors/set_mode/enabled") == 200;
}

void playMove(const char* dataset, const char* name) {
    post(String("/api/move/play/recorded-move-dataset/") + dataset + "/" + name);
}

void setHeadDeg(float rollDeg, float pitchDeg, float yawDeg, float sec) {
    const float R = PI / 180.0f;                  // 単位はラジアン
    char body[192];
    snprintf(body, sizeof(body),
        "{\"head_pose\":{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f},\"duration\":%.2f}",
        rollDeg * R, pitchDeg * R, yawDeg * R, sec);
    post("/api/move/goto", body);
}
```

### ハマりどころ（実機で確認済み）

1. **backend が `stopped` だと 503。** さらにモーターが `disabled` だと uuid が返るのに
   動かない。通常は `running` で始まるが、人が停止していると起きるので状態確認を入れる。
   本体再起動後はトルクが `disabled` から始まる点にも注意
2. **データセット名のスラッシュは `%2F`** にエンコードする
3. **単位はラジアン**
4. モーション再生は非ブロッキング。長いもの（`sleep1` は19.76秒）は完了を待たない。
   完了確認は `GET /api/move/running`
5. **StopWatch の Wi-Fi は 2.4GHz のみ。** 5GHz 専用の SSID には繋がらない
6. mDNS（`reachy-mini.local`）は ESP32 で不安定なことがある。IP フォールバックを用意する
7. 認証が無いので、**LAN 内の誰でも操作できる**。公開ネットワークでは使わない
8. `HTTPClient` のタイムアウトは長め（8秒程度）に。起動系は時間がかかる
9. `set_target` を高頻度で送るとロボット側の制御ループ（50Hz）と競合しうる。
   まず 20〜30Hz 程度から試す

### 動作確認の順序

**ESP32 を書く前に、PC の curl で必ず疎通を確認する。** 問題の切り分けが速くなる。

```bash
curl http://reachy-mini.local:8000/api/daemon/status
curl -X POST "http://reachy-mini.local:8000/api/daemon/start?wake_up=false"
curl -X POST http://reachy-mini.local:8000/api/motors/set_mode/enabled
curl -X POST "http://reachy-mini.local:8000/api/move/play/recorded-move-dataset/pollen-robotics%2Freachy-mini-dances-library/simple_nod"
```

### 記録のルール

`hello-reachy-mini` リポジトリと同じ方針で、技術記事の素材になる情報を残す。

- 期待と実際の挙動の違い、発生した課題（現象・原因・解決・学び）
- ESP32 側と Reachy Mini 側のどちらの問題かを明確に分ける
- 事実と推測を区別し、未確認には「要検証」と明記する
- Wi-Fi の SSID・パスワード・トークンは記録しない

### 未検証・確認したいこと

- ~~backend が `stopped` になる条件~~ → **判明。人が Reachy Mini Control 等から停止した
  場合。アイドルによる自動停止ではない**
- ESP32 の mDNS で `reachy-mini.local` を解決できるか
- `set_target` を送れる実用的な最大周波数
- 連続してモーション再生を要求したときの挙動（キューに入るのか、弾かれるのか）
- StopWatch のマイクとスピーカーを使って、音声トリガーや音声再生ができるか
- バッテリー 450mAh でどれくらい動くか（Wi-Fi 常時接続時）
