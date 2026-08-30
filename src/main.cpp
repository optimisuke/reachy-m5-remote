// M5Stack StopWatch から Reachy Mini のダンスを再生する。
//
// 文字を使わない UI。色と絵と位置だけで操作できるようにしてある。
//   ・選んでいるダンス … 画面いっぱいの色の丸＋動きを表す絵
//   ・つぎのダンスへ  … 左右スワイプ / きいろボタン(M5.BtnA)
//   ・ごー！          … 画面をタップ / あおボタン(M5.BtnB)
//   ・とめる          … 再生中に画面をタップ / あおボタン
//
// 例外は「つながらない」画面の下に小さく出す英字だけ。これは大人が原因を切り分ける
// ためのもので、子どもの操作には関係ない。

#include <M5Unified.h>
#include <WiFi.h>
#include <stdlib.h>

#include "dances.h"
#include "icons.h"
#include "robot.h"
#include "secrets.h"

namespace {

// 円形 AMOLED 468x468。座標は実際の画面サイズから作る。
int cx = 234, cy = 234;
int mainY;      // 大きな丸の中心
int dotsY;      // 選択位置を示す点の列
int chevronDX;  // 左右のスワイプ目印までの距離

const int MAIN_R = 140;      // 大きな丸の半径
const int SWIPE_THRESHOLD = 50;  // これ以上横に動いたらスワイプ扱い

enum Screen { BOOT, SELECT, PLAYING, TROUBLE };
Screen screen = BOOT;

int selected = 0;
String playingUuid;
uint32_t nextPollAt = 0;
uint32_t nextHeartbeatAt = 0;
uint32_t nextAnimAt = 0;
int animStep = 0;
String troubleMsg;

// ---------------------------------------------------------------- 触覚

// StopWatch は振動モーターを内蔵しているが、M5Unified に API がなく
// 駆動方法も公式ドキュメントに明記されていない（要検証: M5IOE1 経由の可能性）。
// 判明するまでは画面のフラッシュだけでフィードバックする。
void buzz(uint16_t /*ms*/) {
    // TODO: 振動モーターの制御方法が分かったらここに入れる。
}

// ---------------------------------------------------------------- 描画

// 画面のいちばん外周に細いリングを描く。円形ディスプレイの縁をそのまま使うので
// 邪魔にならず、Wi-Fi が落ちたときだけ赤くなって気づける。
void drawEdgeRing(bool ok) {
    int r = min(cx, cy);
    M5.Display.fillArc(cx, cy, r - 5, r - 1, 0, 360, ok ? 0x1E3A2F : 0x7F1D1D);
}

void drawSelect() {
    const Dance& d = DANCES[selected];
    M5.Display.fillScreen(TFT_BLACK);

    // 選んでいるダンスを画面いっぱいの丸で見せる。丸ごとタップ範囲。
    M5.Display.fillCircle(cx, mainY, MAIN_R, d.color);
    icons::dance(M5.Display, d.icon, cx, mainY, 88, TFT_WHITE);

    // 左右にスワイプできることを示す。控えめな灰色。
    icons::chevron(M5.Display, cx - chevronDX, mainY, 22, -1, 0x52525B);
    icons::chevron(M5.Display, cx + chevronDX, mainY, 22, 1, 0x52525B);

    // 「4つのうちいまここ」を点で示す。
    const int spacing = 44;
    int x0 = cx - spacing * (DANCE_COUNT - 1) / 2;
    for (int i = 0; i < DANCE_COUNT; i++) {
        int x = x0 + spacing * i;
        if (i == selected) {
            M5.Display.fillCircle(x, dotsY, 14, DANCES[i].color);
        } else {
            M5.Display.fillCircle(x, dotsY, 8, 0x3F3F46);
        }
    }

    drawEdgeRing(WiFi.status() == WL_CONNECTED);
}

void drawPlaying() {
    const Dance& d = DANCES[selected];
    M5.Display.fillScreen(d.color);
    // 触ると止まることを ■ で示す。まわりのくるくるは loop で回す。
    icons::stop(M5.Display, cx, mainY, 96, TFT_WHITE);
    drawEdgeRing(WiFi.status() == WL_CONNECTED);
}

// 再生中のくるくる。背景色を上塗りしながら回す。
void animatePlaying() {
    const Dance& d = DANCES[selected];
    M5.Display.fillArc(cx, mainY, MAIN_R - 14, MAIN_R, 0, 360, d.color);
    icons::spinner(M5.Display, cx, mainY, MAIN_R, animStep, TFT_WHITE);
}

void drawTrouble(const String& msg) {
    M5.Display.fillScreen(TFT_BLACK);
    icons::blocked(M5.Display, cx, mainY - 10, 86, 0xEF4444);
    // 触るか きいろボタンで やり直せることを丸い矢印で示す。
    icons::retry(M5.Display, cx, dotsY - 6, 40, 0xA1A1AA);

    // ここだけ文字。大人が失敗箇所を切り分けるための最小限の情報。
    M5.Display.setFont(&fonts::Font0);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextColor(0x52525B);
    M5.Display.drawString(msg, cx, dotsY + 44);
    drawEdgeRing(false);
}

// 起動中のくるくる。step を増やしながら呼ぶ。
void drawBootProgress(int step) {
    icons::spinner(M5.Display, cx, mainY, 60, step, TFT_WHITE);
    M5.Display.fillArc(cx, mainY, 45, 46, 0, 360, TFT_BLACK);  // 内側を消す
    icons::spinner(M5.Display, cx, mainY, 60, step - 4, 0x18181B);  // 尾を消す
}

// 押したことが必ず分かるように一瞬光らせる。
void flash(uint32_t color) {
    M5.Display.fillScreen(color);
    delay(60);
}

// ---------------------------------------------------------------- 起動

// Wi-Fi は固定設定。2.4GHz の SSID しか繋がらない。
bool connectWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    for (int i = 0; i < 200; i++) {  // 最大20秒
        if (WiFi.status() == WL_CONNECTED) return true;
        drawBootProgress(i);
        delay(100);
    }
    return false;
}

void startUp() {
    Serial.printf("[boot] wifi connecting to \"%s\"\n", WIFI_SSID);
    M5.Display.fillScreen(TFT_BLACK);
    if (!connectWifi()) {
        Serial.printf("[boot] wifi ng (status=%d)\n", WiFi.status());
        troubleMsg = "wifi ng";
        screen = TROUBLE;
        drawTrouble(troubleMsg);
        return;
    }

    Serial.printf("[boot] wifi ok ip=%s rssi=%d\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
    Serial.printf("[boot] robot %s\n", ROBOT_HOST);

    String detail;
    if (!robot::ensureReady(detail)) {
        Serial.printf("[boot] robot ng: %s\n", detail.c_str());
        troubleMsg = detail;
        screen = TROUBLE;
        drawTrouble(troubleMsg);
        return;
    }

    Serial.println("[boot] ready");
    screen = SELECT;
    drawSelect();
}

// ---------------------------------------------------------------- 入力

struct Input {
    bool next = false;   // つぎのダンスへ
    bool prev = false;   // ひとつ前のダンスへ
    bool go = false;     // ごー！ / とめる
};

// ボタンとタッチをまとめて一つの意図に変換する。
Input readInput() {
    Input in;
    if (M5.BtnA.wasPressed()) in.next = true;
    if (M5.BtnB.wasPressed()) in.go = true;

    auto t = M5.Touch.getDetail();
    if (t.wasFlicked() || t.wasClicked() || t.wasReleased()) {
        int dx = t.distanceX();
        Serial.printf("[touch] release at (%d,%d) dx=%d -> %s\n", t.x, t.y, dx,
                      abs(dx) > SWIPE_THRESHOLD ? "swipe" : "tap");
        if (abs(dx) > SWIPE_THRESHOLD) {
            // 左へ払ったら次、右へ払ったら前。紙をめくる向きに合わせる。
            if (dx < 0) in.next = true; else in.prev = true;
        } else {
            in.go = true;
        }
    }
    return in;
}

}  // namespace

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setBrightness(160);

    cx = M5.Display.width() / 2;
    cy = M5.Display.height() / 2;
    mainY = cy - 30;
    dotsY = cy + 150;
    chevronDX = MAIN_R + 40;

    Serial.begin(115200);
    delay(600);  // USB CDC が繋がるのを少し待つ（待たないと最初のログが落ちる）
    Serial.printf("\n[boot] board=%d display=%dx%d touch=%d psram=%u\n",
                  (int)M5.getBoard(), M5.Display.width(), M5.Display.height(),
                  (int)M5.Touch.isEnabled(), (unsigned)ESP.getPsramSize());

    startUp();
}

void loop() {
    M5.update();
    Input in = readInput();

    switch (screen) {
        case SELECT:
            if (in.next || in.prev) {
                buzz(30);
                selected = (selected + (in.next ? 1 : DANCE_COUNT - 1)) % DANCE_COUNT;
                Serial.printf("[ui] %s -> %s\n", in.next ? "next" : "prev",
                              DANCES[selected].id);
                drawSelect();
            } else if (in.go) {
                buzz(60);
                Serial.printf("[ui] go %s\n", DANCES[selected].id);
                flash(TFT_WHITE);
                playingUuid = robot::playDance(DANCES[selected].id);
                if (playingUuid.length() == 0) {
                    troubleMsg = "play ng";
                    screen = TROUBLE;
                    drawTrouble(troubleMsg);
                    break;
                }
                screen = PLAYING;
                nextPollAt = millis() + 800;
                drawPlaying();
            }
            break;

        case PLAYING:
            if (in.go) {  // とめる
                buzz(60);
                Serial.println("[ui] stop");
                robot::stopMove(playingUuid);
                screen = SELECT;
                drawSelect();
                break;
            }
            if (millis() >= nextAnimAt) {
                nextAnimAt = millis() + 60;
                animStep++;
                animatePlaying();
            }
            // 再生は非ブロッキングなので、終わったかを定期的に聞く。
            if (millis() >= nextPollAt) {
                nextPollAt = millis() + 800;
                if (!robot::isPlaying()) {
                    screen = SELECT;
                    drawSelect();
                }
            }
            break;

        case TROUBLE:
            if (in.next || in.prev || in.go) {
                screen = BOOT;
                startUp();
            }
            break;

        case BOOT:
            break;
    }

    // 起動ログはリセット直後に流れきってしまう。後からシリアルを繋いでも
    // 状態が分かるように、待機中も定期的に現在の状態を出す。
    if (millis() >= nextHeartbeatAt) {
        nextHeartbeatAt = millis() + 5000;
        // 待機中に Wi-Fi が落ちたらリングの色で知らせる（描き直すのは縁だけ）。
        static int lastWifi = -1;
        int nowWifi = WiFi.status();
        if (nowWifi != lastWifi && screen != BOOT) {
            lastWifi = nowWifi;
            drawEdgeRing(nowWifi == WL_CONNECTED);
        }
        static const char* names[] = {"boot", "select", "playing", "trouble"};
        Serial.printf("[hb] screen=%s dance=%s touch=%d wifi=%d ip=%s heap=%u\n",
                      names[screen], DANCES[selected].id, (int)M5.Touch.isEnabled(),
                      WiFi.status(), WiFi.localIP().toString().c_str(),
                      (unsigned)ESP.getFreeHeap());
    }

    delay(10);
}
