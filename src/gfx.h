#pragma once

// 実機（M5Unified）と Mac のプレビュー（M5GFX + SDL）で同じ描画コードを使うための
// 入口。描画は lgfx::LovyanGFX& だけに依存させ、M5Unified には依存させない。
#if defined(ARDUINO)
#include <M5Unified.h>
#else
#include <M5GFX.h>
#endif

using Gfx = lgfx::LovyanGFX;
