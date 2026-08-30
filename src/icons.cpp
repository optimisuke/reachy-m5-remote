#include "icons.h"

#include <math.h>

namespace {

// 太い線。細い線は円形 AMOLED では見えにくいので既定で太くする。
void bar(Gfx& g, int x0, int y0, int x1, int y1, int w, uint32_t c) {
    g.drawWideLine(x0, y0, x1, y1, w, c);
}

// (x, y) を先端として、角度 deg の向きに開く三角形の矢羽。
void head(Gfx& g, int x, int y, float deg, int size, uint32_t c) {
    const float R = M_PI / 180.0f;
    float a = deg * R;
    // 先端から後方へ 140度ずつ開いた2点を取る。
    float lx = x - cosf(a - 0.6f) * size;
    float ly = y - sinf(a - 0.6f) * size;
    float rx = x - cosf(a + 0.6f) * size;
    float ry = y - sinf(a + 0.6f) * size;
    g.fillTriangle(x, y, (int)lx, (int)ly, (int)rx, (int)ry, c);
}

}  // namespace

namespace icons {

void dance(Gfx& g, DanceIcon icon, int x, int y, int size, uint32_t color) {
    const int w = size / 5;          // 線の太さ
    const int h = size;              // 矢印の半分の長さ
    const int a = size / 2;          // 矢羽の大きさ

    switch (icon) {
        case ICON_NOD:  // 縦の両矢印
            bar(g, x, y - h + a, x, y + h - a, w, color);
            head(g, x, y - h, -90, a, color);
            head(g, x, y + h, 90, a, color);
            break;

        case ICON_SWAY:  // 横の両矢印
            bar(g, x - h + a, y, x + h - a, y, w, color);
            head(g, x - h, y, 180, a, color);
            head(g, x + h, y, 0, a, color);
            break;

        case ICON_SPIN: {  // 丸い矢印。右上を開けて、開いた端に矢羽を置く
            const float R = M_PI / 180.0f;
            const float endDeg = 295.0f;
            int r = h - a / 2;
            // drawArc は輪郭線しか描かないので、帯にするには fillArc を使う。
            g.fillArc(x, y, r - w / 2, r + w / 2, 25, endDeg, color);
            // 角度が増える向き＝時計回り。その接線は角度 +90 の向きになる。
            // 帯の先から少し進めた位置に矢羽を置くと、形がはっきりする。
            float tipDeg = endDeg + 22;
            int ex = x + cosf(tipDeg * R) * r;
            int ey = y + sinf(tipDeg * R) * r;
            head(g, ex, ey, tipDeg + 90, a * 3 / 4, color);
            break;
        }

        case ICON_PECK: {  // 斜め下へ突く矢印と、つつかれる点
            int t = h - a;
            bar(g, x - t, y - t, x + t / 2, y + t / 2, w, color);
            head(g, x + t, y + t, 45, a, color);
            g.fillCircle(x + t - w, y + t + a - w / 2, w * 3 / 4, color);
            break;
        }
    }
}

void chevron(Gfx& g, int x, int y, int size, int dir, uint32_t color) {
    int w = size / 4;
    bar(g, x - size / 2 * dir, y - size, x + size / 2 * dir, y, w, color);
    bar(g, x + size / 2 * dir, y, x - size / 2 * dir, y + size, w, color);
}

void stop(Gfx& g, int x, int y, int size, uint32_t color) {
    g.fillRoundRect(x - size / 2, y - size / 2, size, size, size / 6, color);
}

void retry(Gfx& g, int x, int y, int size, uint32_t color) {
    dance(g, ICON_SPIN, x, y, size, color);
}

void blocked(Gfx& g, int x, int y, int size, uint32_t color) {
    int w = size / 5;
    int r = size;
    g.fillArc(x, y, r - w / 2, r + w / 2, 0, 360, color);
    // 斜線。太さの分を差し引いて円の内側に収める。
    float d = (r - w) * 0.60f;
    bar(g, x - d, y - d, x + d, y + d, w, color);
}

void spinner(Gfx& g, int x, int y, int r, int thickness, int step, uint32_t color) {
    int from = (step * 14) % 360;
    g.fillArc(x, y, r - thickness, r, from, from + 90, color);
}

void track(Gfx& g, int x, int y, int r, int thickness, uint32_t color) {
    g.fillArc(x, y, r - thickness, r, 0, 360, color);
}

uint32_t darken(uint32_t color, int percent) {
    int r = ((color >> 16) & 0xFF) * percent / 100;
    int gg = ((color >> 8) & 0xFF) * percent / 100;
    int b = (color & 0xFF) * percent / 100;
    return (uint32_t)((r << 16) | (gg << 8) | b);
}

}  // namespace icons
