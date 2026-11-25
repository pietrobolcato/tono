/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* Shared with the dictionary packer so both use identical fallback pronunciation. */
#define SYB 0x40u                /* a phoneme byte: the first of a syllable */

/* ----------------------------------------------------------------------------------- reading the words --- */
#define SW_MAX 32u               /* a word's phonemes, at most */
enum { TK_C, TK_S, TK_W };       /* a token: a consonant, a strong vowel (a e o), a weak one (i u) */
static int sy_isv(int c) { return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'; }

static int sy_es_cl(uint32_t a, uint32_t b)              /* an onset cluster: p b f k g t d + l r (not dl) */
{
    return (a == PH_P || a == PH_B || a == PH_F || a == PH_K || a == PH_G || a == PH_T || a == PH_D) &&
           (b == PH_L || b == PH_RT) && !(a == PH_D && b == PH_L);
}
/* tokens -> the word's phonemes with their syllables (gen_lyrics.py syllabify): the vowel groups (split between two
 * strong vowels) are the syllables; a group's nucleus its first strong vowel, else its last; the weak ones before it
 * glides (y w), after it offglides; between two groups one consonant goes to the next, two too when a cluster, else
 * they split (more: the last two or one to the next). Returns the phonemes, *ns the syllables (0: no vowel) */
static uint32_t sy_syl(uint8_t *tc, const uint8_t *tk, uint32_t m, uint8_t *out, uint32_t *ns)
{
    uint8_t st[SW_MAX];
    uint32_t i = 0, g = 0, pa = 0, k;
    for (k = 0; k < m; k++)
        st[k] = 0;
    while (i < m) {
        uint32_t j, nu;
        if (tk[i] == TK_C) {
            i++;
            continue;
        }
        for (j = i + 1u; j < m && tk[j] != TK_C && !(tk[j] == TK_S && tk[j - 1u] == TK_S); j++)
            ;
        for (nu = i; nu < j && tk[nu] != TK_S; nu++)
            ;
        if (nu == j)
            nu = j - 1u;
        for (k = i; k < nu; k++)
            tc[k] = tc[k] == PH_I ? PH_Y : PH_W;
        if (g) {
            uint32_t c = i - pa;
            if (!c) st[i] = 1;
            else if (c == 1u) st[pa] = 1;
            else if (c == 2u) st[sy_es_cl(tc[pa], tc[pa + 1u]) ? pa : pa + 1u] = 1;
            else st[sy_es_cl(tc[i - 2u], tc[i - 1u]) ? i - 2u : i - 1u] = 1;
        }
        g++;
        pa = j;
        i = j;
    }
    *ns = g;
    if (!g)
        return 0;
    st[0] = 1;
    for (k = 0; k < m; k++)
        out[k] = (uint8_t)(tc[k] | (st[k] ? SYB : 0u));
    return m;
}

static uint32_t sy_en_rules(const char *w, uint32_t n, uint8_t *tc, uint8_t *tk)
{
    uint32_t i, m = 0, magic = 0xFFu;
#define EN_T(c, k) do { if (m < SW_MAX) { tc[m] = (uint8_t)(c); tk[m++] = (uint8_t)(k); } } while (0)
    if (n >= 3u && w[n - 1u] == 'e' && !sy_isv(w[n - 2u]) && sy_isv(w[n - 3u]))
        magic = n - 3u;                                  /* a silent e: the vowel before says its name */
    for (i = 0; i < n; i++) {
        int c = w[i], nx = i + 1u < n ? w[i + 1u] : 0, nx2 = i + 2u < n ? w[i + 2u] : 0;
        if (i == n - 1u && c == 'e' && n > 2u) break;    /* (a final e: silent) */
        if (sy_isv(c) || c == 'y') {
            if (c == 'y' && (i == 0u || sy_isv(nx))) { EN_T(PH_Y, TK_C); continue; }
            if (c == 'y') { EN_T(i == n - 1u ? PH_I : PH_IH, TK_S); continue; }
            if (i == magic) {
                if (c == 'a') { EN_T(PH_E, TK_S); EN_T(PH_IH, TK_W); }
                else if (c == 'i') { EN_T(PH_AA, TK_S); EN_T(PH_IH, TK_W); }
                else if (c == 'o') { EN_T(PH_O, TK_S); EN_T(PH_UH, TK_W); }
                else if (c == 'u') EN_T(PH_U, TK_S);
                else EN_T(PH_I, TK_S);
                continue;
            }
            if ((c == 'e' && (nx == 'e' || nx == 'a')) ) { EN_T(PH_I, TK_S); i++; continue; }
            if (c == 'o' && nx == 'o') { EN_T(PH_U, TK_S); i++; continue; }
            if (c == 'o' && (nx == 'u' || nx == 'w')) { EN_T(PH_AA, TK_S); EN_T(PH_UH, TK_W); i++; continue; }
            if (c == 'a' && (nx == 'i' || nx == 'y')) { EN_T(PH_E, TK_S); EN_T(PH_IH, TK_W); i++; continue; }
            if (c == 'o' && nx == 'a') { EN_T(PH_O, TK_S); EN_T(PH_UH, TK_W); i++; continue; }
            if (c == 'o' && (nx == 'i' || nx == 'y')) { EN_T(PH_AO, TK_S); EN_T(PH_IH, TK_W); i++; continue; }
            if (c == 'a' && (nx == 'u' || nx == 'w')) { EN_T(PH_AO, TK_S); i++; continue; }
            if (c == 'i' && nx == 'g' && nx2 == 'h') { EN_T(PH_AA, TK_S); EN_T(PH_IH, TK_W); i += 2u; continue; }
            EN_T(c == 'a' ? PH_AE : c == 'e' ? PH_EH : c == 'i' ? PH_IH : c == 'o' ? PH_AA : PH_AH, TK_S);
            continue;
        }
        if (c == 't' && nx == 'h') { EN_T(PH_TH, TK_C); i++; continue; }
        if (c == 's' && nx == 'h') { EN_T(PH_SH, TK_C); i++; continue; }
        if (c == 'c' && nx == 'h') { EN_T(PH_CH, TK_C); i++; continue; }
        if (c == 'p' && nx == 'h') { EN_T(PH_F, TK_C); i++; continue; }
        if (c == 'w' && nx == 'h') { EN_T(PH_W, TK_C); i++; continue; }
        if (c == 'c' && nx == 'k') { EN_T(PH_K, TK_C); i++; continue; }
        if (c == 'q' && nx == 'u') { EN_T(PH_K, TK_C); EN_T(PH_W, TK_C); i++; continue; }
        if (c == 'k' && nx == 'n' && !i) continue;
        if (c == 'w' && nx == 'r') continue;
        if (c == 'g' && nx == 'h') { i++; continue; }
        if (c == 'n' && nx == 'g' && !sy_isv(nx2)) { EN_T(PH_NG, TK_C); i++; continue; }
        switch (c) {
        case 'b': EN_T(PH_B, TK_C); break;
        case 'c': EN_T(nx == 'e' || nx == 'i' || nx == 'y' ? PH_S : PH_K, TK_C); break;
        case 'd': EN_T(PH_D, TK_C); break;
        case 'f': EN_T(PH_F, TK_C); break;
        case 'g': EN_T(PH_G, TK_C); break;
        case 'h': if (sy_isv(nx)) EN_T(PH_H, TK_C); break;
        case 'j': EN_T(PH_J, TK_C); break;
        case 'k': EN_T(PH_K, TK_C); break;
        case 'l': if (nx != 'l') EN_T(PH_L, TK_C); break;
        case 'm': EN_T(PH_M, TK_C); break;
        case 'n': EN_T(PH_N, TK_C); break;
        case 'p': EN_T(PH_P, TK_C); break;
        case 'r': EN_T(PH_RA, TK_C); break;
        case 's': EN_T(i && sy_isv(w[i - 1u]) && sy_isv(nx) ? PH_Z : PH_S, TK_C); break;
        case 't': EN_T(PH_T, TK_C); break;
        case 'v': EN_T(PH_V, TK_C); break;
        case 'w': EN_T(PH_W, TK_C); break;
        case 'x': EN_T(PH_K, TK_C); EN_T(PH_S, TK_C); break;
        case 'z': EN_T(PH_Z, TK_C); break;
        default: break;
        }
    }
#undef EN_T
    return m;
}

