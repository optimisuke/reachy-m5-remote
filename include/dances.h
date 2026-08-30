#pragma once
#include <stdint.h>

// 動きを表す絵。文字を使わずにダンスを区別するために描く。
enum DanceIcon : uint8_t {
    ICON_NOD,   // 縦の両矢印 … うなずく
    ICON_SWAY,  // 横の両矢印 … 左右に揺れる
    ICON_SPIN,  // 丸い矢印   … 回る
    ICON_PECK,  // 点をつつく矢印 … 突く
};

// 子ども向けに「動きが見て分かる」ものを4種だけ選んだ。
// dances-library の19種から選定。すべて音なし、1.82〜5.00秒。
struct Dance {
    const char* id;     // daemon に渡すモーション名
    const char* label;  // 画面に大きく出すひらがなの名前
    uint32_t color;     // パネルの色（RGB888）
    DanceIcon icon;     // 動きを表す絵
};

static const Dance DANCES[] = {
    {"simple_nod",        "うんうん",     0x22C55E, ICON_NOD},   // 緑
    {"side_to_side_sway", "ゆらゆら",     0x3B82F6, ICON_SWAY},  // 青
    {"dizzy_spin",        "くるくる",     0xF59E0B, ICON_SPIN},  // 黄
    {"chicken_peck",      "つんつん",     0xEF4444, ICON_PECK},  // 赤
};

static const int DANCE_COUNT = sizeof(DANCES) / sizeof(DANCES[0]);
