#pragma once
#include "dances.h"
#include "gfx.h"

// 画面の描画。M5Unified・Wi-Fi・HTTP に依存しないので、実機でも Mac のプレビューでも
// 同じコードが動く。状態は呼び出し側が持ち、ここは受け取って描くだけ。
namespace ui {

enum Screen : uint8_t { BOOT, SELECT, PLAYING, TROUBLE };

// 画面サイズから決まる座標。円形 468x468 を前提に調整してある。
struct Layout {
    int cx, cy;
    int mainY;      // 大きな丸の中心
    int dotsY;      // 選択位置を示す点の列
    int chevronDX;  // 左右のスワイプ目印までの距離
    int mainR;      // 大きな丸の半径
    int edgeR;      // 外周リングの半径
};

Layout layout(int w, int h);

struct State {
    Screen screen = BOOT;
    int selected = 0;
    bool wifiOk = false;
    const char* trouble = "";  // つながらない画面に小さく出す英字
    int animStep = 0;          // くるくるの位相
};

// 画面全体を描く。状態が変わったときに呼ぶ。
void draw(Gfx& g, const Layout& L, const State& s);

// 再生中・起動中のくるくるだけを更新する。animStep を進めながら呼ぶ。
void animate(Gfx& g, const Layout& L, const State& s);

// 外周リングだけ描き直す（Wi-Fi の状態が変わったとき）。
void edgeRing(Gfx& g, const Layout& L, bool ok);

}  // namespace ui
