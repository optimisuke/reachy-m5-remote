// M5Stack StopWatch から Reachy Mini のダンスを再生する。
//
// UI は「一度に一つだけ大きく見せる」方式。
//   きいろボタン(KEYA/G2) = つぎのダンスへ
//   あおボタン  (KEYB/G1) = ごー！（再生中は とめる）
// 文字が読めなくても色と位置で区別できるようにしてある。

#include <M5Unified.h>
#include <WiFi.h>

#include "dances.h"
#include "robot.h"
#include "secrets.h"

namespace {

// StopWatch のボタン。M5Unified がこの機種を認識しない可能性があるので、
// 公式ドキュメントのピン番号を直接読む。KEYA=G2(黄), KEYB=G1(青)。
constexpr int PIN_KEY_A = 2;
constexpr int PIN_KEY_B = 1;

// 円形 AMOLED 466x466。
int cx = 233, cy = 233;

enum Screen { BOOT, SELECT, PLAYING, TROUBLE };
Screen screen = BOOT;

int selected = 0;
String playingUuid;
uint32_t nextPollAt = 0;
String troubleMsg;

// ---------------------------------------------------------------- ボタン

struct Key {
    int pin;
    bool prev = true;  // プルアップなので押していないときは HIGH
    uint32_t lastChange = 0;

    void begin() { pinMode(pin, INPUT_PULLUP); }

    // 押した瞬間だけ true。30ms のデバウンス付き。
    bool pressed() {
        bool now = digitalRead(pin);
        uint32_t t = millis();
        if (now != prev && t - lastChange > 30) {
            lastChange = t;
            prev = now;
            return now == LOW;
        }
        return false;
    }
};

Key keyA{PIN_KEY_A};
Key keyB{PIN_KEY_B};

// ---------------------------------------------------------------- 触覚

// StopWatch は振動モーターを内蔵しているが、駆動ピンが公式ドキュメントに
// 明記されていない（要検証: M5IOE1 経由の可能性）。判明するまでは
// 画面のフラッシュだけでフィードバックする。
void buzz(uint16_t /*ms*/) {
    // TODO: 振動モーターの制御方法が分かったらここに入れる。
}

// ---------------------------------------------------------------- 描画

void drawStatusDot(bool ok) {
    // 大人向けの小さな目印。子どもの操作には関係ないので目立たせない。
    M5.Display.fillCircle(cx + 97, cy - 123, 9, ok ? 0x0F5132 : 0x842029);
}

void drawSelect() {
    const Dance& d = DANCES[selected];
    M5.Display.fillScreen(TFT_BLACK);

    // 選んでいるダンスを画面いっぱいの丸で見せる。
    M5.Display.fillCircle(cx, 200, 135, d.color);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setFont(&fonts::lgfxJapanGothic_40);
    M5.Display.drawString(d.label, cx, 200);

    // 「4つのうちいまここ」を点で示す。
    const int spacing = 44;
    int x0 = cx - spacing * (DANCE_COUNT - 1) / 2;
    for (int i = 0; i < DANCE_COUNT; i++) {
        int x = x0 + spacing * i;
        if (i == selected) {
            M5.Display.fillCircle(x, 370, 13, TFT_WHITE);
        } else {
            M5.Display.drawCircle(x, 370, 10, 0x52525B);
        }
    }

    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.setTextColor(0xA1A1AA);
    M5.Display.drawString("きいろ→つぎ  あお→ごー", cx, 415);
    drawStatusDot(true);
}

void drawPlaying() {
    const Dance& d = DANCES[selected];
    M5.Display.fillScreen(d.color);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setFont(&fonts::lgfxJapanGothic_40);
    M5.Display.drawString("おどってる！", cx, 210);
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.drawString("あお→とめる", cx, 330);
}

void drawTrouble(const String& msg) {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextColor(0xEF4444);
    M5.Display.setFont(&fonts::lgfxJapanGothic_40);
    M5.Display.drawString("つながらない", cx, 190);
    M5.Display.setTextColor(0xA1A1AA);
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.drawString(msg, cx, 270);
    M5.Display.drawString("きいろ→もういちど", cx, 330);
    drawStatusDot(false);
}

void drawBoot(const String& line) {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setFont(&fonts::lgfxJapanGothic_24);
    M5.Display.drawString("じゅんびちゅう", cx, 200);
    M5.Display.setTextColor(0xA1A1AA);
    M5.Display.setFont(&fonts::lgfxJapanGothic_16);
    M5.Display.drawString(line, cx, 260);
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
    for (int i = 0; i < 40; i++) {  // 最大20秒
        if (WiFi.status() == WL_CONNECTED) return true;
        delay(500);
    }
    return false;
}

void startUp() {
    drawBoot("wifi...");
    if (!connectWifi()) {
        troubleMsg = "wifi ng";
        screen = TROUBLE;
        drawTrouble(troubleMsg);
        return;
    }

    drawBoot(WiFi.localIP().toString());
    delay(400);

    drawBoot("robot...");
    String detail;
    if (!robot::ensureReady(detail)) {
        troubleMsg = detail;
        screen = TROUBLE;
        drawTrouble(troubleMsg);
        return;
    }

    screen = SELECT;
    drawSelect();
}

}  // namespace

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setBrightness(160);
    cx = M5.Display.width() / 2;
    cy = M5.Display.height() / 2;

    keyA.begin();
    keyB.begin();

    startUp();
}

void loop() {
    M5.update();
    bool a = keyA.pressed();
    bool b = keyB.pressed();

    switch (screen) {
        case SELECT:
            if (a) {  // つぎのダンスへ
                buzz(30);
                selected = (selected + 1) % DANCE_COUNT;
                drawSelect();
            } else if (b) {  // ごー！
                buzz(60);
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
            if (b) {  // とめる
                buzz(60);
                robot::stopMove(playingUuid);
                screen = SELECT;
                drawSelect();
                break;
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
            if (a) {
                screen = BOOT;
                startUp();
            }
            break;

        case BOOT:
            break;
    }

    delay(10);
}
