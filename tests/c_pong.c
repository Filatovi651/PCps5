#include "../sdk/pcps5_sdk.h"

void draw_rect(int x0, int y0, int w, int h, uint32_t color) {
    uint32_t* fb = FB_ADDR;
    for (int y = y0; y < y0 + h; ++y) {
        if (y < 0 || y >= SCREEN_H) continue;
        for (int x = x0; x < x0 + w; ++x) {
            if (x < 0 || x >= SCREEN_W) continue;
            fb[y * SCREEN_W + x] = color;
        }
    }
}

void clear_screen(uint32_t color) {
    uint32_t* fb = FB_ADDR;
    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        fb[i] = color;
    }
}

void _start(void) {
    int pad_y = 95;
    int ball_x = 160, ball_y = 120;
    int vx = 3, vy = 2;

    pcps5_play_sound(600, 100);

    while (1) {
        clear_screen(0xFF1E1212); // Темно-синий корт

        // Сетка по центру
        for (int y = 0; y < SCREEN_H; y += 16) {
            draw_rect(159, y, 2, 8, 0xFF504040);
        }

        // Ракетка игрока (слева) и мяч
        draw_rect(12, pad_y, 6, 44, 0xFF50FF50);
        draw_rect(ball_x, ball_y, 7, 7, 0xFF40D0FF);

        uint64_t keys = pcps5_present_frame();

        if ((keys & KEY_UP) && pad_y > 4) pad_y -= 4;
        if ((keys & KEY_DOWN) && pad_y < SCREEN_H - 48) pad_y += 4;

        ball_x += vx;
        ball_y += vy;

        // Отскок от верхней, нижней и правой стенки
        if (ball_y <= 2 || ball_y >= SCREEN_H - 9) {
            vy = -vy;
            pcps5_play_sound(440, 25);
        }
        if (ball_x >= SCREEN_W - 9) {
            vx = -vx;
            pcps5_play_sound(440, 25);
        }

        // Отскок от ракетки игрока
        if (ball_x <= 18 && ball_x >= 10 && ball_y + 7 >= pad_y && ball_y <= pad_y + 44) {
            vx = -vx;
            ball_x = 19;
            pcps5_play_sound(880, 40);
        }

        // Пропуск мяча -> перезапуск в центр
        if (ball_x < 0) {
            ball_x = 160;
            ball_y = 120;
            vx = 3;
            pcps5_play_sound(220, 150);
        }
    }
}
