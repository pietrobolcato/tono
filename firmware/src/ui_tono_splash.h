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
        cv_rect(136 - x, 30 - y, 2 * x + 1, 1, ink);
        cv_rect(136 - x, 30 + y, 2 * x + 1, 1, ink);
    }
}

static int32_t tono_ease(uint32_t ms, uint32_t start, uint32_t end)
{
    if (ms <= start) return 0;
    if (ms >= end) return 256;
    int32_t left = 256 - (int32_t)((ms - start) * 256u / (end - start));
    return 256 - left * left / 256;
}

static void tono_kingfisher(int32_t dx, int32_t dy, uint16_t ink, uint16_t blue,
                           uint16_t teal, uint16_t white, uint16_t clay, uint16_t amber)
{
#define BIRD(x, y) ((tono_point){(x) + dx, (y) + dy})
    /* Dark contours sit under separate ink plates for back, crown and chest. */
    tono_triangle(BIRD(122, 77), BIRD(169, 97), BIRD(154, 102), ink);
    tono_triangle(BIRD(115, 81), BIRD(159, 95), BIRD(148, 99), blue);
    tono_triangle(BIRD(69, 55), BIRD(121, 58), BIRD(139, 91), clay);
    tono_triangle(BIRD(69, 55), BIRD(139, 91), BIRD(107, 108), clay);
    tono_triangle(BIRD(69, 55), BIRD(107, 108), BIRD(82, 95), amber);
    tono_triangle(BIRD(74, 70), BIRD(106, 94), BIRD(90, 98), amber);
    tono_triangle(BIRD(98, 56), BIRD(125, 47), BIRD(145, 73), blue);
    tono_triangle(BIRD(98, 56), BIRD(145, 73), BIRD(146, 95), blue);
    tono_triangle(BIRD(98, 56), BIRD(146, 95), BIRD(126, 99), ink);
    tono_triangle(BIRD(101, 59), BIRD(119, 57), BIRD(138, 88), teal);
    tono_triangle(BIRD(101, 59), BIRD(138, 88), BIRD(121, 79), blue);
    tono_triangle(BIRD(109, 73), BIRD(133, 83), BIRD(141, 93), teal);
    tono_triangle(BIRD(24, 57), BIRD(77, 43), BIRD(78, 61), ink);
    tono_triangle(BIRD(37, 54), BIRD(76, 47), BIRD(76, 53), blue);
    tono_triangle(BIRD(69, 49), BIRD(85, 31), BIRD(105, 30), teal);
    tono_triangle(BIRD(69, 49), BIRD(105, 30), BIRD(123, 50), blue);
    tono_triangle(BIRD(69, 49), BIRD(123, 50), BIRD(106, 65), blue);
    tono_triangle(BIRD(74, 52), BIRD(105, 54), BIRD(91, 65), white);
    tono_triangle(BIRD(74, 52), BIRD(91, 65), BIRD(74, 61), white);
    tono_triangle(BIRD(81, 37), BIRD(100, 33), BIRD(107, 42), teal);
    cv_rect(81+dx,43+dy,6,6,ink);cv_rect(82+dx,43+dy,2,2,white);
    cv_line_t(90+dx,99+dy,94+dx,111+dy,ink,2);
    cv_line_t(103+dx,103+dy,106+dx,112+dy,ink,2);
    cv_rect(94+dx,111+dy,7,2,ink);cv_rect(106+dx,111+dy,8,2,ink);
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
    uint16_t sky = mono ? ux_mix(paper, ink, 12) : RGB(208, 209, 169);
    uint16_t haze = mono ? ux_mix(paper, ink, 38) : RGB(157, 172, 137);
    uint16_t pine = mono ? ux_mix(paper, ink, 68) : RGB(60, 98, 86);
    uint16_t lake = mono ? ux_mix(paper, ink, 23) : RGB(118, 154, 136);
    uint16_t clay = mono ? ux_mix(paper, ink, 52) : TONO_CLAY;
    uint16_t amber = mono ? ux_mix(paper, ink, 38) : RGB(225, 149, 73);
    uint16_t blue = mono ? ux_mix(paper, ink, 78) : RGB(37, 97, 117);
    uint16_t teal = mono ? ux_mix(paper, ink, 58) : RGB(71, 150, 142);
    uint16_t white = mono ? paper : RGB(249, 230, 186);
    int32_t sun = tono_ease(ms, 40u, 225u);
    int32_t rise = (256 - tono_ease(ms, 100u, 370u)) * 34 / 256;

    cv_begin(TONO_ART_W, TONO_ART_H, sky);
    if (sun) tono_sun(25 * sun / 256, clay);
    tono_triangle((tono_point){-53, 112 + rise}, (tono_point){18, 55 + rise},
                  (tono_point){112, 112 + rise}, haze);
    tono_triangle((tono_point){109, 109 + rise}, (tono_point){190, 54 + rise},
                  (tono_point){201, 110 + rise}, haze);
    tono_triangle((tono_point){-23, 116 + rise}, (tono_point){-1, 76 + rise},
                  (tono_point){61, 116 + rise}, pine);
    cv_rect(0, 106, TONO_ART_W, 21, lake);
    cv_rect(13, 113, 38, 1, haze);
    cv_rect(18, 122, 49, 1, haze);
    cv_rect(73, 123, 18, 1, sky);
    cv_rect(102, 119, 45, 1, haze);
    cv_rect(148, 112, 22, 1, haze);
    tono_tree(8, 106 + rise, 21, ink);
    tono_tree(17, 105 + rise, 13, pine);
    if (ms >= 180u) {
        int32_t branch = (256 - tono_ease(ms, 180u, 440u)) * 18 / 256;
        tono_triangle((tono_point){37, 120 + branch},(tono_point){167, 104 + branch},
                      (tono_point){139, 114 + branch},ink);
        cv_line(55, 121 + branch, 75, 109 + branch, ink);
        cv_line(64, 116 + branch, 113, 112 + branch, pine);
    }
    if (ms >= 260u) {
        int32_t flight = 256 - tono_ease(ms, 260u, SPL_WRITE);
        tono_kingfisher(19 * flight / 256, -17 * flight / 256, ink, blue,
                        teal, white, clay, amber);
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
