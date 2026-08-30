#include "icons.h"

#include <math.h>

namespace {

// 太い線。細い線は円形 AMOLED では見えにくいので既定で太くする。
void bar(M5GFX& g, int x0, int y0, int x1, int y1, int w, uint32_t c) {
    g.drawWideLine(x0, y0, x1, y1, w, c);
}

// (x, y) を先端として、角度 deg の向きに開く三角形の矢羽。
void head(M5GFX& g, int x, int y, float deg, int size, uint32_t c) {
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

void dance(M5GFX& g, DanceIcon icon, int x, int y, int size, uint32_t color) {
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

        case ICON_SPIN: {  // 丸い矢印。上を開けて、開いた端に矢羽を置く
            int r = h - a / 2;
            g.drawArc(x, y, r - w / 2, r + w / 2, 30, 330, color);
            // 330度の位置＝右上。そこから接線方向（時計回り）に向ける
            const float R = M_PI / 180.0f;
            int ex = x + cosf(330 * R) * r;
            int ey = y + sinf(330 * R) * r;
            head(g, ex, ey, 330 - 90, a, color);
            break;
        }

        case ICON_PECK: {  // 斜め下へ突く矢印と、つつかれる点
            int t = h - a;
            bar(g, x - t, y - t, x + t / 2, y + t / 2, w, color);
            head(g, x + t, y + t, 45, a, color);
            g.fillCircle(x + t, y + t + a + w, w, color);
            break;
        }
    }
}

void chevron(M5GFX& g, int x, int y, int size, int dir, uint32_t color) {
    int w = size / 4;
    bar(g, x - size / 2 * dir, y - size, x + size / 2 * dir, y, w, color);
    bar(g, x + size / 2 * dir, y, x - size / 2 * dir, y + size, w, color);
}

void stop(M5GFX& g, int x, int y, int size, uint32_t color) {
    g.fillRoundRect(x - size / 2, y - size / 2, size, size, size / 6, color);
}

void retry(M5GFX& g, int x, int y, int size, uint32_t color) {
    dance(g, ICON_SPIN, x, y, size, color);
}

void blocked(M5GFX& g, int x, int y, int size, uint32_t color) {
    int w = size / 5;
    int r = size;
    g.drawArc(x, y, r - w / 2, r + w / 2, 0, 360, color);
    // 斜線。円の内側に収める
    float d = (r - w) * 0.707f;
    bar(g, x - d, y - d, x + d, y + d, w, color);
}

void spinner(M5GFX& g, int x, int y, int size, int step, uint32_t color) {
    int w = size / 4;
    int from = (step * 24) % 360;
    g.drawArc(x, y, size - w, size, from, from + 90, color);
}

}  // namespace icons
