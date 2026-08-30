// M5Stack StopWatch から Reachy Mini のダンスを再生する。
//
// 文字を使わない UI。色と絵と位置だけで操作できるようにしてある。
//   ・つぎのダンスへ … 左へスワイプ / きいろボタン(M5.BtnA)
//   ・ひとつ前へ     … 右へスワイプ
//   ・ごー！        … 画面をタップ / あおボタン(M5.BtnB)
//   ・とめる        … 再生中にタップ / あおボタン
//
// 画面の描画は src/ui.cpp にある（Mac のプレビューと共用するため、M5Unified や
// Wi-Fi には依存させていない）。ここは入力・通信・状態遷移だけを持つ。

#include <M5Unified.h>
#include <WiFi.h>
#include <stdlib.h>

#include "dances.h"
#include "robot.h"
#include "secrets.h"
#include "ui.h"

namespace {

const int SWIPE_THRESHOLD = 50;  // これ以上横に動いたらスワイプ扱い

ui::Layout L;
ui::State st;

String playingUuid;
uint32_t nextPollAt = 0;
uint32_t nextHeartbeatAt = 0;
uint32_t nextAnimAt = 0;
String troubleMsg;

// ---------------------------------------------------------------- 触覚

// StopWatch は振動モーターを内蔵しているが、M5Unified に API がなく
// 駆動方法も公式ドキュメントに明記されていない（要検証: M5IOE1 経由の可能性）。
// 判明するまでは画面のフラッシュだけでフィードバックする。
void buzz(uint16_t /*ms*/) {
    // TODO: 振動モーターの制御方法が分かったらここに入れる。
}

// 押したことが必ず分かるように一瞬光らせる。
void flash() {
    M5.Display.fillScreen(0xFFFFFFu);
    delay(60);
}

void redraw() {
    st.wifiOk = (WiFi.status() == WL_CONNECTED);
    st.trouble = troubleMsg.c_str();
    ui::draw(M5.Display, L, st);
}

void goTrouble(const char* why) {
    troubleMsg = why;
    st.screen = ui::TROUBLE;
    redraw();
}

// ---------------------------------------------------------------- 起動

// Wi-Fi は固定設定。2.4GHz の SSID しか繋がらない。
bool connectWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    for (int i = 0; i < 200; i++) {  // 最大20秒
        if (WiFi.status() == WL_CONNECTED) return true;
        st.animStep = i;
        ui::animate(M5.Display, L, st);
        delay(100);
    }
    return false;
}

void startUp() {
    Serial.printf("[boot] wifi connecting to \"%s\"\n", WIFI_SSID);
    st.screen = ui::BOOT;
    redraw();

    if (!connectWifi()) {
        Serial.printf("[boot] wifi ng (status=%d)\n", WiFi.status());
        goTrouble("wifi ng");
        return;
    }

    Serial.printf("[boot] wifi ok ip=%s rssi=%d\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
    Serial.printf("[boot] robot %s\n", ROBOT_HOST);

    String detail;
    if (!robot::ensureReady(detail)) {
        Serial.printf("[boot] robot ng: %s\n", detail.c_str());
        goTrouble(detail.c_str());
        return;
    }

    Serial.println("[boot] ready");
    st.screen = ui::SELECT;
    redraw();
}

// ---------------------------------------------------------------- 入力

struct Input {
    bool next = false;  // つぎのダンスへ
    bool prev = false;  // ひとつ前のダンスへ
    bool go = false;    // ごー！ / とめる
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

    L = ui::layout(M5.Display.width(), M5.Display.height());

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

    switch (st.screen) {
        case ui::SELECT:
            if (in.next || in.prev) {
                buzz(30);
                st.selected = (st.selected + (in.next ? 1 : DANCE_COUNT - 1)) % DANCE_COUNT;
                Serial.printf("[ui] %s -> %s\n", in.next ? "next" : "prev",
                              DANCES[st.selected].id);
                redraw();
            } else if (in.go) {
                buzz(60);
                Serial.printf("[ui] go %s\n", DANCES[st.selected].id);
                flash();
                playingUuid = robot::playDance(DANCES[st.selected].id);
                if (playingUuid.length() == 0) {
                    goTrouble("play ng");
                    break;
                }
                st.screen = ui::PLAYING;
                nextPollAt = millis() + 800;
                redraw();
            }
            break;

        case ui::PLAYING:
            if (in.go) {  // とめる
                buzz(60);
                Serial.println("[ui] stop");
                robot::stopMove(playingUuid);
                st.screen = ui::SELECT;
                redraw();
                break;
            }
            if (millis() >= nextAnimAt) {
                nextAnimAt = millis() + 60;
                st.animStep++;
                ui::animate(M5.Display, L, st);
            }
            // 再生は非ブロッキングなので、終わったかを定期的に聞く。
            if (millis() >= nextPollAt) {
                nextPollAt = millis() + 800;
                if (!robot::isPlaying()) {
                    st.screen = ui::SELECT;
                    redraw();
                }
            }
            break;

        case ui::TROUBLE:
            if (in.next || in.prev || in.go) startUp();
            break;

        case ui::BOOT:
            break;
    }

    // 起動ログはリセット直後に流れきってしまう。後からシリアルを繋いでも
    // 状態が分かるように、待機中も定期的に現在の状態を出す。
    if (millis() >= nextHeartbeatAt) {
        nextHeartbeatAt = millis() + 5000;
        // 待機中に Wi-Fi が落ちたらリングの色で知らせる（描き直すのは縁だけ）。
        static int lastWifi = -1;
        int nowWifi = WiFi.status();
        if (nowWifi != lastWifi && st.screen != ui::BOOT) {
            lastWifi = nowWifi;
            ui::edgeRing(M5.Display, L, nowWifi == WL_CONNECTED);
        }
        static const char* names[] = {"boot", "select", "playing", "trouble"};
        Serial.printf("[hb] screen=%s dance=%s touch=%d wifi=%d ip=%s heap=%u\n",
                      names[st.screen], DANCES[st.selected].id, (int)M5.Touch.isEnabled(),
                      nowWifi, WiFi.localIP().toString().c_str(),
                      (unsigned)ESP.getFreeHeap());
    }

    delay(10);
}
