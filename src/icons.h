#pragma once
#include <M5Unified.h>

#include "dances.h"

// 文字を使わずに動きを伝えるための描画。すべて図形で描く。
namespace icons {

// ダンスの動きを表す絵。(x, y) を中心に、size を目安の大きさとして描く。
void dance(M5GFX& g, DanceIcon icon, int x, int y, int size, uint32_t color);

// スワイプできることを示す山形。dir が -1 なら左向き、+1 なら右向き。
void chevron(M5GFX& g, int x, int y, int size, int dir, uint32_t color);

// 停止（■）。再生中に「触ると止まる」を示す。
void stop(M5GFX& g, int x, int y, int size, uint32_t color);

// やり直し（丸い矢印）。
void retry(M5GFX& g, int x, int y, int size, uint32_t color);

// 禁止（丸に斜線）。つながらないときに出す。
void blocked(M5GFX& g, int x, int y, int size, uint32_t color);

// 待ち中のくるくる。step を増やしながら呼ぶと回る。
void spinner(M5GFX& g, int x, int y, int size, int step, uint32_t color);

}  // namespace icons
