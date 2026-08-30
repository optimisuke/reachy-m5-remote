#pragma once
#include "dances.h"
#include "gfx.h"

// 文字を使わずに動きを伝えるための描画。すべて図形で描く。
namespace icons {

// ダンスの動きを表す絵。(x, y) を中心に、size を目安の大きさとして描く。
void dance(Gfx& g, DanceIcon icon, int x, int y, int size, uint32_t color);

// スワイプできることを示す山形。dir が -1 なら左向き、+1 なら右向き。
void chevron(Gfx& g, int x, int y, int size, int dir, uint32_t color);

// 停止（■）。再生中に「触ると止まる」を示す。
void stop(Gfx& g, int x, int y, int size, uint32_t color);

// やり直し（丸い矢印）。
void retry(Gfx& g, int x, int y, int size, uint32_t color);

// 禁止（丸に斜線）。つながらないときに出す。
void blocked(Gfx& g, int x, int y, int size, uint32_t color);

// 待ち中のくるくる。step を増やしながら呼ぶと回る。
void spinner(Gfx& g, int x, int y, int r, int thickness, int step, uint32_t color);

// くるくるの下地になる輪。回っている帯の通り道を示す。
void track(Gfx& g, int x, int y, int r, int thickness, uint32_t color);

// 色を暗くする。下地のリングなど、背景になじませたいときに使う。
uint32_t darken(uint32_t color, int percent);

}  // namespace icons
