#pragma once
#include <stdint.h>

// 子ども向けに「動きが見て分かる」ものを4種だけ選んだ。
// dances-library の19種から選定。すべて音なし、1.82〜5.00秒。
struct Dance {
    const char* id;     // daemon に渡すモーション名
    const char* label;  // 画面に出す名前（ひらがな）
    uint32_t color;     // パネルの色（RGB888）
};

static const Dance DANCES[] = {
    {"simple_nod",         "うんうん",   0x22C55E},  // 緑: うなずく
    {"side_to_side_sway",  "ゆらゆら",   0x3B82F6},  // 青: 左右に揺れる
    {"dizzy_spin",         "くるくる",   0xF59E0B},  // 黄: 回る
    {"chicken_peck",       "つんつん",   0xEF4444},  // 赤: 突く
};

static const int DANCE_COUNT = sizeof(DANCES) / sizeof(DANCES[0]);
