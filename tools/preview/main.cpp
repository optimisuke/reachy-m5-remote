// Mac 上で UI を確認するためのプレビュー。
//
// 実機と同じ描画コード（src/ui.cpp と src/icons.cpp）を M5GFX の SDL バックエンドで
// 動かす。Wi-Fi とロボットは繋がないので、再生は一定時間で終わる作りにしてある。
//
//   マウス左ドラッグ … スワイプ（横に 50px 以上動かすと切り替わる）
//   マウスクリック   … タップ（ごー！ / とめる）
//   A キー           … きいろボタン
//   B / スペース     … あおボタン
//   T キー           … つながらない画面を出す
//   1〜4             … その番号のダンスを選ぶ
//   S キー           … いまの画面を PNG に保存
//
//   --shot <dir>     … 全画面を PNG に書き出して終了（目視確認用）
//   --scale <n>      … 拡大率（既定 1）

#include <M5GFX.h>
#include <lgfx/v1/platforms/sdl/Panel_sdl.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../../src/ui.h"

namespace {

const int SCREEN_W = 468;
const int SCREEN_H = 468;
const int SWIPE_THRESHOLD = 50;   // 実機と同じ値
const int PLAY_MSEC = 3000;       // 実機のダンスは 1.8〜5.0 秒

class Sim : public lgfx::LGFX_Device {
    lgfx::Panel_sdl _panel;
public:
    Sim(int w, int h, int scale) {
        auto cfg = _panel.config();
        cfg.memory_width = cfg.panel_width = w;
        cfg.memory_height = cfg.panel_height = h;
        _panel.config(cfg);
        _panel.setScaling(scale, scale);
        _panel.setWindowTitle("reachy-m5-remote preview");
        setPanel(&_panel);
    }
};

Sim* sim = nullptr;
ui::Layout L;
ui::State st;

uint32_t playUntil = 0;
bool dragging = false;
int dragStartX = 0;
int shotIndex = 0;

// 24bit BMP で書き出す。PNG は macOS の sips で変換する。
bool writeBmp(const std::string& path) {
    int w = SCREEN_W, h = SCREEN_H;
    // uint8_t* の overload は 8bit カラー扱いになるので、必ず rgb888_t* で読む。
    std::vector<lgfx::rgb888_t> rgb(w * h);
    sim->readRect(0, 0, w, h, rgb.data());

    int rowPad = (4 - (w * 3) % 4) % 4;
    uint32_t dataSize = (w * 3 + rowPad) * h;
    uint32_t fileSize = 54 + dataSize;

    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    uint8_t hdr[54] = {};
    hdr[0] = 'B'; hdr[1] = 'M';
    memcpy(hdr + 2, &fileSize, 4);
    uint32_t off = 54; memcpy(hdr + 10, &off, 4);
    uint32_t infoSize = 40; memcpy(hdr + 14, &infoSize, 4);
    memcpy(hdr + 18, &w, 4);
    int hNeg = h;  // 下から上に書く
    memcpy(hdr + 22, &hNeg, 4);
    uint16_t planes = 1; memcpy(hdr + 26, &planes, 2);
    uint16_t bpp = 24; memcpy(hdr + 28, &bpp, 2);
    memcpy(hdr + 34, &dataSize, 4);
    fwrite(hdr, 1, 54, f);

    std::vector<uint8_t> row(w * 3 + rowPad, 0);
    for (int y = h - 1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            const lgfx::rgb888_t& px = rgb[y * w + x];
            row[x * 3 + 0] = px.b;  // BMP は BGR
            row[x * 3 + 1] = px.g;
            row[x * 3 + 2] = px.r;
        }
        fwrite(row.data(), 1, row.size(), f);
    }
    fclose(f);
    return true;
}

void save(const std::string& dir, const std::string& name) {
    std::string bmp = dir + "/" + name + ".bmp";
    if (!writeBmp(bmp)) { fprintf(stderr, "書き出せない: %s\n", bmp.c_str()); return; }
    std::string cmd = "sips -s format png '" + bmp + "' --out '" + dir + "/" + name +
                      ".png' >/dev/null 2>&1 && rm -f '" + bmp + "'";
    if (system(cmd.c_str()) != 0) fprintf(stderr, "sips で変換できない: %s\n", bmp.c_str());
    printf("saved %s/%s.png\n", dir.c_str(), name.c_str());
}

// 全画面を書き出して終わる。目視で詰めるとき用。
int shotAll(const std::string& dir) {
    st.wifiOk = true;
    // 絵あり・絵なしの両方を出して見比べられるようにする。
    for (int withIcon = 1; withIcon >= 0; withIcon--) {
        st.showIcon = withIcon;
        for (int i = 0; i < DANCE_COUNT; i++) {
            st.screen = ui::SELECT;
            st.selected = i;
            ui::draw(*sim, L, st);
            save(dir, std::string(withIcon ? "select-" : "textonly-") + DANCES[i].id);
        }
    }
    st.showIcon = true;
    st.screen = ui::PLAYING;
    st.selected = 0;
    st.animStep = 5;
    ui::draw(*sim, L, st);
    ui::animate(*sim, L, st);
    save(dir, "playing");

    st.screen = ui::TROUBLE;
    st.trouble = "wifi ng";
    ui::draw(*sim, L, st);
    save(dir, "trouble");

    st.screen = ui::BOOT;
    st.animStep = 3;
    ui::draw(*sim, L, st);
    save(dir, "boot");
    return 0;
}

// 実機の loop() と同じ組み立て。入力を意図に変えて状態を進める。
void step(bool next, bool prev, bool go) {
    switch (st.screen) {
        case ui::SELECT:
            if (next || prev) {
                st.selected = (st.selected + (next ? 1 : DANCE_COUNT - 1)) % DANCE_COUNT;
                ui::draw(*sim, L, st);
            } else if (go) {
                sim->fillScreen(0xFFFFFFu);  // 実機のフラッシュ
                lgfx::delay(60);
                st.screen = ui::PLAYING;
                playUntil = lgfx::millis() + PLAY_MSEC;
                ui::draw(*sim, L, st);
            }
            break;
        case ui::PLAYING:
            if (go) {
                st.screen = ui::SELECT;
                ui::draw(*sim, L, st);
            }
            break;
        case ui::TROUBLE:
            if (next || prev || go) {
                st.screen = ui::SELECT;
                ui::draw(*sim, L, st);
            }
            break;
        case ui::BOOT:
            break;
    }
}

int frame(bool* /*running*/) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            dragging = true;
            dragStartX = e.button.x;
        } else if (e.type == SDL_MOUSEBUTTONUP && dragging) {
            dragging = false;
            int dx = e.button.x - dragStartX;
            printf("[touch] dx=%d -> %s\n", dx,
                   abs(dx) > SWIPE_THRESHOLD ? "swipe" : "tap");
            if (abs(dx) > SWIPE_THRESHOLD) step(dx < 0, dx > 0, false);
            else step(false, false, true);
        } else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_a: step(true, false, false); break;
                case SDLK_b: case SDLK_SPACE: step(false, false, true); break;
                case SDLK_i:  // 絵ありと絵なしを切り替える
                    st.showIcon = !st.showIcon;
                    ui::draw(*sim, L, st);
                    break;
                case SDLK_t:
                    st.screen = ui::TROUBLE;
                    st.trouble = "wifi ng";
                    ui::draw(*sim, L, st);
                    break;
                case SDLK_s:
                    save(".", "shot-" + std::to_string(shotIndex++));
                    break;
                case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4: {
                    int i = e.key.keysym.sym - SDLK_1;
                    if (i < DANCE_COUNT) {
                        st.screen = ui::SELECT;
                        st.selected = i;
                        ui::draw(*sim, L, st);
                    }
                    break;
                }
            }
        }
    }

    // 再生は非ブロッキング。実機は /api/move/running を見るが、ここは時間で終わる。
    if (st.screen == ui::PLAYING && lgfx::millis() > playUntil) {
        st.screen = ui::SELECT;
        ui::draw(*sim, L, st);
    }
    if (st.screen == ui::PLAYING || st.screen == ui::BOOT) {
        st.animStep++;
        ui::animate(*sim, L, st);
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    int scale = 1;
    std::string shotDir;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--scale") && i + 1 < argc) scale = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--shot") && i + 1 < argc) shotDir = argv[++i];
    }

    sim = new Sim(SCREEN_W, SCREEN_H, scale);
    sim->init();
    L = ui::layout(SCREEN_W, SCREEN_H);
    st.wifiOk = true;
    st.screen = ui::SELECT;

    if (!shotDir.empty()) {
        lgfx::Panel_sdl::setup();
        int r = shotAll(shotDir);
        lgfx::Panel_sdl::close();
        return r;
    }

    ui::draw(*sim, L, st);
    printf("マウス: ドラッグ=スワイプ / クリック=タップ　キー: A=きいろ B=あお "
           "T=つながらない 1-4=ダンス選択 S=PNG保存\n");
    return lgfx::Panel_sdl::main(frame, 16);
}
