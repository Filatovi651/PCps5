#include "../sdk/pcps5_sdk.h"

#define MAP_W 20
#define MAP_H 15
#define TILE  16

char map[MAP_H][MAP_W + 1];

void draw_tile(int gx, int gy, uint32_t color) {
    uint32_t* fb = FB_ADDR;
    for (int y = 0; y < TILE; ++y) {
        for (int x = 0; x < TILE; ++x) {
            fb[(gy * TILE + y) * SCREEN_W + (gx * TILE + x)] = color;
        }
    }
}

void _start(void) {
    int64_t fd = pcps5_open("level1.map");
    if (fd < 0) pcps5_exit(1);

    for (int y = 0; y < MAP_H; ++y) {
        pcps5_read(fd, map[y], MAP_W + 1);
    }
    pcps5_close(fd);

    int px = 1, py = 1;
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            if (map[y][x] == 'P') { px = x; py = y; map[y][x] = '.'; }
        }
    }

    int cooldown = 0;
    pcps5_play_sound(500, 100);

    while (1) {
        for (int y = 0; y < MAP_H; ++y) {
            for (int x = 0; x < MAP_W; ++x) {
                char c = map[y][x];
                if (c == '#') draw_tile(x, y, 0xFF603030);
                else if (c == 'E') draw_tile(x, y, 0xFF30D0FF);
                else draw_tile(x, y, 0xFF221818);
            }
        }
        draw_tile(px, py, 0xFF50FF50);

        uint64_t keys = pcps5_present_frame();
        if (cooldown > 0) { cooldown--; continue; }

        int nx = px, ny = py;
        if (keys & KEY_UP) ny--;
        else if (keys & KEY_DOWN) ny++;
        else if (keys & KEY_LEFT) nx--;
        else if (keys & KEY_RIGHT) nx++;

        if ((nx != px || ny != py) && map[ny][nx] != '#') {
            px = nx;
            py = ny;
            cooldown = 6;
            pcps5_play_sound(700, 20);
            if (map[py][px] == 'E') {
                pcps5_play_sound(1000, 250);
                px = 1; py = 1;
            }
        }
    }
}
