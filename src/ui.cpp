#include "ui.h"

#include "icons.h"

namespace {

const uint32_t COL_BG = 0x000000u;
const uint32_t COL_FG = 0xFFFFFFu;
const uint32_t COL_DIM = 0x52525Bu;        // 控えめな灰色（スワイプ目印など）
const uint32_t COL_DOT_OFF = 0x3F3F46u;    // 選ばれていない点
const uint32_t COL_RING_OK = 0x1E3A2Fu;    // 外周リング（接続あり）
const uint32_t COL_RING_NG = 0x7F1D1Du;    // 外周リング（接続なし）
const uint32_t COL_DANGER = 0xEF4444u;
const uint32_t COL_LABEL = 0x52525Bu;      // 大人向けの小さな英字

const int RING_W = 12;    // くるくるの帯の太さ
const int BOOT_R = 110;   // 起動中のくるくるの半径

}  // namespace

namespace ui {

Layout layout(int w, int h) {
    Layout L;
    L.cx = w / 2;
    L.cy = h / 2;
    L.mainR = 140;
    L.mainY = L.cy - 30;
    L.dotsY = L.cy + 150;
    L.chevronDX = L.mainR + 40;
    L.edgeR = (w < h ? w : h) / 2;
    return L;
}

void edgeRing(Gfx& g, const Layout& L, bool ok) {
    // 画面のいちばん外周に細いリングを描く。円形ディスプレイの縁をそのまま使うので
    // 邪魔にならず、Wi-Fi が落ちたときだけ赤くなって気づける。
    g.fillArc(L.cx, L.cy, L.edgeR - 5, L.edgeR - 1, 0, 360, ok ? COL_RING_OK : COL_RING_NG);
}

namespace {

void drawSelect(Gfx& g, const Layout& L, const State& s) {
    const Dance& d = DANCES[s.selected];
    g.fillScreen(COL_BG);

    // 選んでいるダンスを画面いっぱいの丸で見せる。丸ごとタップ範囲。
    g.fillCircle(L.cx, L.mainY, L.mainR, d.color);
    icons::dance(g, d.icon, L.cx, L.mainY, 88, COL_FG);

    // 左右にスワイプできることを示す。
    icons::chevron(g, L.cx - L.chevronDX, L.mainY, 22, -1, COL_DIM);
    icons::chevron(g, L.cx + L.chevronDX, L.mainY, 22, 1, COL_DIM);

    // 「4つのうちいまここ」を点で示す。選択中はそのダンスの色。
    const int spacing = 44;
    int x0 = L.cx - spacing * (DANCE_COUNT - 1) / 2;
    for (int i = 0; i < DANCE_COUNT; i++) {
        int x = x0 + spacing * i;
        if (i == s.selected) {
            g.fillCircle(x, L.dotsY, 14, DANCES[i].color);
        } else {
            g.fillCircle(x, L.dotsY, 8, COL_DOT_OFF);
        }
    }

    edgeRing(g, L, s.wifiOk);
}

void drawPlaying(Gfx& g, const Layout& L, const State& s) {
    // 画面全部がそのダンスの色になる。外周リングは色と喧嘩するので出さない。
    uint32_t base = DANCES[s.selected].color;
    g.fillScreen(base);
    // 縁を回るリング。下地を暗く敷いてから白い帯を回すので、動きが分かる。
    icons::track(g, L.cx, L.cy, L.edgeR - 8, RING_W, icons::darken(base, 70));
    icons::spinner(g, L.cx, L.cy, L.edgeR - 8, RING_W, s.animStep, COL_FG);
    // 触ると止まることを ■ で示す。
    icons::stop(g, L.cx, L.cy, 104, COL_FG);
}

void drawTrouble(Gfx& g, const Layout& L, const State& s) {
    g.fillScreen(COL_BG);
    icons::blocked(g, L.cx, L.cy - 60, 92, COL_DANGER);
    // 触るか きいろボタンで やり直せることを丸い矢印で示す。
    icons::retry(g, L.cx, L.cy + 100, 48, 0xA1A1AAu);

    // ここだけ文字。大人が失敗箇所を切り分けるための最小限の情報。
    // 円形画面の下部は横幅が狭いので、縁から余裕を取った位置に置く。
    g.setFont(&lgfx::fonts::Font0);
    g.setTextSize(2);  // Font0 は 6x8 で小さすぎるので倍にする
    g.setTextDatum(lgfx::textdatum::middle_center);
    g.setTextColor(COL_LABEL);
    g.drawString(s.trouble, L.cx, L.cy + 168);
    g.setTextSize(1);
    edgeRing(g, L, false);
}

void drawBoot(Gfx& g, const Layout& L, const State& s) {
    g.fillScreen(COL_BG);
    icons::track(g, L.cx, L.cy, BOOT_R, RING_W, 0x27272Au);
    icons::spinner(g, L.cx, L.cy, BOOT_R, RING_W, s.animStep, COL_FG);
}

}  // namespace

void draw(Gfx& g, const Layout& L, const State& s) {
    switch (s.screen) {
        case SELECT:  drawSelect(g, L, s);  break;
        case PLAYING: drawPlaying(g, L, s); break;
        case TROUBLE: drawTrouble(g, L, s); break;
        case BOOT:    drawBoot(g, L, s);    break;
    }
}

void animate(Gfx& g, const Layout& L, const State& s) {
    // 回転している帯の通り道だけを背景で消してから描き直す。全面を塗り直すと
    // ちらつくので、リングの幅の分だけに限る。
    if (s.screen == PLAYING) {
        uint32_t base = DANCES[s.selected].color;
        icons::track(g, L.cx, L.cy, L.edgeR - 8, RING_W, icons::darken(base, 70));
        icons::spinner(g, L.cx, L.cy, L.edgeR - 8, RING_W, s.animStep, COL_FG);
    } else if (s.screen == BOOT) {
        icons::track(g, L.cx, L.cy, BOOT_R, RING_W, 0x27272Au);
        icons::spinner(g, L.cx, L.cy, BOOT_R, RING_W, s.animStep, COL_FG);
    }
}

}  // namespace ui
