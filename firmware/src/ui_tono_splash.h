/* SPDX-License-Identifier: GPL-3.0-only */
/* Tono's application startup screen. The illustration is drawn from solid
 * shapes into the existing LCD strip canvas, with no image assets or heap. */
#define TONO_NAME "Tono"
#define TONO_INK RGB(23, 39, 34)
#define TONO_PAPER RGB(237, 219, 182)
#define TONO_SAGE RGB(169, 186, 163)
#define TONO_RULE RGB(92, 118, 100)
#define TONO_SELECT RGB(49, 71, 60)
#define TONO_AMBER RGB(242, 188, 118)
#define TONO_CLAY RGB(190, 84, 59)
#define SPL_WRITE 800u
#define SPL_HOLD 900u

enum { TONO_ART_X = 31, TONO_ART_Y = 27, TONO_ART_W = 178, TONO_ART_H = 127 };
static uint8_t spl_anim;
static uint32_t tono_last_frame;

typedef struct { int32_t x, y; } tono_point;

static int32_t tono_edge(tono_point a, tono_point b, int32_t y)
{
    return a.x + (b.x - a.x) * (y - a.y) / (b.y - a.y);
}

static void tono_triangle(tono_point a, tono_point b, tono_point c, uint16_t ink)
{
    tono_point swap;
    int32_t y;
    if (a.y > b.y) { swap = a; a = b; b = swap; }
    if (b.y > c.y) { swap = b; b = c; c = swap; }
    if (a.y > b.y) { swap = a; a = b; b = swap; }
    for (y = a.y; y < c.y; y++) {
        int32_t x1 = tono_edge(a, c, y);
        int32_t x2 = y < b.y ? tono_edge(a, b, y) : tono_edge(b, c, y);
        if (x1 > x2) { int32_t x = x1; x1 = x2; x2 = x; }
        cv_rect(x1, y, x2 - x1 + 1, 1, ink);
    }
}

static void tono_round(int32_t x, int32_t y, int32_t w, int32_t h,
                       int32_t corner, uint16_t ink)
{
    int32_t row;
    for (row = 0; row < h; row++) {
        int32_t inset = corner - (row < h / 2 ? row : h - 1 - row);
        if (inset < 0) inset = 0;
        cv_rect(x + inset, y + row, w - 2 * inset, 1, ink);
    }
}

static void tono_sun(int32_t radius, uint16_t ink)
{
    int32_t y, x = radius;
    for (y = 0; y <= radius; y++) {
        while (x * x + y * y > radius * radius) x--;
        cv_rect(126 - x, 30 - y, 2 * x + 1, 1, ink);
        cv_rect(126 - x, 30 + y, 2 * x + 1, 1, ink);
    }
}

static int32_t tono_ease(uint32_t ms, uint32_t start, uint32_t end)
{
    if (ms <= start) return 0;
    if (ms >= end) return 256;
    int32_t left = 256 - (int32_t)((ms - start) * 256u / (end - start));
    return 256 - left * left / 256;
}

static void tono_bird(int32_t dx, int32_t dy, uint16_t ink, uint16_t blue,
                      uint16_t white, uint16_t clay)
{
#define BIRD(x, y) ((tono_point){(x) + dx, (y) + dy})
    /* Two wings, forked tail, and a light cheek that stays legible in mono. */
    tono_triangle(BIRD(27, 23), BIRD(70, 42), BIRD(89, 70), ink);
    tono_triangle(BIRD(27, 23), BIRD(55, 54), BIRD(89, 70), blue);
    tono_triangle(BIRD(89, 70), BIRD(121, 47), BIRD(155, 41), ink);
    tono_triangle(BIRD(89, 70), BIRD(155, 41), BIRD(131, 65), blue);
    tono_triangle(BIRD(89, 70), BIRD(138, 59), BIRD(104, 80), ink);
    tono_triangle(BIRD(89, 70), BIRD(104, 80), BIRD(69, 87), ink);
    tono_triangle(BIRD(91, 72), BIRD(114, 64), BIRD(104, 79), white);
    tono_triangle(BIRD(109, 65), BIRD(118, 63), BIRD(131, 69), ink);
    tono_triangle(BIRD(114, 69), BIRD(131, 69), BIRD(108, 75), white);
    tono_triangle(BIRD(124, 67), BIRD(139, 70), BIRD(126, 72), clay);
    tono_triangle(BIRD(69, 87), BIRD(89, 70), BIRD(76, 73), blue);
    tono_triangle(BIRD(76, 73), BIRD(65, 76), BIRD(89, 70), ink);
#undef BIRD
}

static void tono_tree(int32_t x, int32_t y, int32_t height, uint16_t ink)
{
    int32_t width = height / 4;
    tono_triangle((tono_point){x, y - height}, (tono_point){x - width, y},
                  (tono_point){x + width, y}, ink);
    cv_rect(x, y, 1, 2, ink);
}

static void tono_landscape(uint32_t ms, uint16_t paper, uint16_t ink)
{
    int mono = ux.mono || settings.palette == UI_BW_INDEX;
    uint16_t sky = mono ? ux_mix(paper, ink, 12) : RGB(203, 209, 175);
    uint16_t haze = mono ? ux_mix(paper, ink, 38) : RGB(151, 174, 143);
    uint16_t pine = mono ? ux_mix(paper, ink, 68) : RGB(53, 93, 87);
    uint16_t lake = mono ? ux_mix(paper, ink, 23) : RGB(112, 154, 137);
    uint16_t clay = mono ? ux_mix(paper, ink, 52) : TONO_CLAY;
    uint16_t blue = mono ? ux_mix(paper, ink, 78) : RGB(47, 77, 87);
    uint16_t white = mono ? paper : RGB(249, 230, 186);
    int32_t sun = tono_ease(ms, 40u, 225u);
    int32_t rise = (256 - tono_ease(ms, 125u, 370u)) * 40 / 256;

    cv_begin(TONO_ART_W, TONO_ART_H, sky);
    if (sun) tono_sun(22 * sun / 256, clay);
    tono_triangle((tono_point){-34, 115 + rise}, (tono_point){41, 52 + rise},
                  (tono_point){102, 115 + rise}, haze);
    tono_triangle((tono_point){55, 115 + rise}, (tono_point){95, 66 + rise},
                  (tono_point){153, 115 + rise}, haze);
    tono_triangle((tono_point){110, 115 + rise}, (tono_point){168, 56 + rise},
                  (tono_point){205, 115 + rise}, haze);
    tono_triangle((tono_point){-20, 123 + rise}, (tono_point){-2, 70 + rise},
                  (tono_point){84, 123 + rise}, pine);
    tono_triangle((tono_point){87, 123 + rise}, (tono_point){154, 82 + rise},
                  (tono_point){196, 123 + rise}, pine);
    cv_rect(0, 111, TONO_ART_W, 16, lake);
    cv_rect(22, 114, 35, 1, haze);
    cv_rect(68, 118, 61, 1, haze);
    cv_rect(42, 123, 17, 1, haze);
    cv_rect(105, 123, 45, 1, haze);
    cv_rect(154, 114, 17, 1, haze);
    tono_tree(11, 110 + rise, 22, ink);
    tono_tree(20, 110 + rise, 14, ink);
    tono_tree(29, 111 + rise, 10, blue);
    tono_tree(146, 112 + rise, 10, blue);
    tono_tree(155, 112 + rise, 16, ink);
    tono_tree(167, 110 + rise, 22, ink);
    if (ms >= 300u) {
        int32_t flight = 256 - tono_ease(ms, 300u, SPL_WRITE);
        tono_bird(-24 * flight / 256, 22 * flight / 256, ink, blue, white, clay);
    }
    cv_blit(TONO_ART_X, TONO_ART_Y);
}

static void tono_wordmark(uint16_t paper, uint16_t ink)
{
    cv_begin(240, 68, paper);
    /* Tono, drawn at integer coordinates for a sharp native wordmark. */
    cv_rect(61, 7, 30, 6, ink);
    cv_rect(73, 7, 6, 33, ink);
    tono_round(92, 16, 25, 24, 7, ink);
    tono_round(98, 22, 13, 12, 4, paper);
    tono_round(125, 16, 23, 24, 6, ink);
    tono_round(131, 22, 11, 22, 3, paper);
    cv_rect(125, 16, 6, 24, ink);
    cv_rect(142, 26, 6, 14, ink);
    tono_round(155, 16, 25, 24, 7, ink);
    tono_round(161, 22, 13, 12, 4, paper);
    cv_text_in(0, 51, 240, &AF_S, "FM1 AMBIENT SYNTHESIZER", ink, paper);
    cv_blit(0, 163);
}

static void draw_splash(void)
{
    int mono = ux.mono || settings.palette == UI_BW_INDEX;
    uint16_t paper = mono ? T_BG : TONO_PAPER;
    uint16_t ink = mono ? T_TEXT : TONO_INK;
    tono_last_frame = spl_anim && !(ui_prefs & PREF_ANIM_OFF) ? 0u : SPL_WRITE;
    cv_oy = 0;
    lcd_fill(0, 0, 240, 240, paper);
    tono_landscape(tono_last_frame, paper, ink);
    tono_wordmark(paper, ink);
    lcd_sync();
}

static void splash_write(uint32_t ms)
{
    if (!spl_anim || (ui_prefs & PREF_ANIM_OFF) || tono_last_frame == SPL_WRITE)
        return;
    if (ms < SPL_WRITE && ms - tono_last_frame < 50u)
        return;
    tono_last_frame = ms < SPL_WRITE ? ms : SPL_WRITE;
    int mono = ux.mono || settings.palette == UI_BW_INDEX;
    tono_landscape(tono_last_frame, mono ? T_BG : TONO_PAPER, mono ? T_TEXT : TONO_INK);
    lcd_sync();
}
