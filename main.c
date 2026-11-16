/* ¿Quieres ser mi novia? - Homebrew para PSP (PSPSDK)
 * Pantalla 480x272, dibujo por software, música 8 bits por audio de la PSP.
 * Controles: D-Pad izq/der = elegir, X o O = confirmar. HOME = salir. */
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspaudio.h>
#include <math.h>
#include <string.h>

PSP_MODULE_INFO("NoviaPSP", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

#define W 480
#define H 272
#define STRIDE 512
/* Color desde 0xRRGGBB al formato ABGR de la PSP */
#define C(h) (0xFF000000u | (((h) & 0xFF) << 16) | ((h) & 0xFF00) | (((h) >> 16) & 0xFF))

static u32 fb[STRIDE * H];

static void R(u32 col, int x, int y, int w, int h)
{
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > W ? W : x + w, y1 = y + h > H ? H : y + h;
    for (int j = y0; j < y1; j++)
        for (int i = x0; i < x1; i++)
            fb[j * STRIDE + i] = col;
}

/* ---------- Callback de salida (botón HOME) ---------- */
static int exit_callback(int a, int b, void *c) { sceKernelExitGame(); return 0; }
static int callback_thread(SceSize args, void *argp)
{
    int id = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(id);
    sceKernelSleepThreadCB();
    return 0;
}
static void setup_callbacks(void)
{
    int t = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (t >= 0) sceKernelStartThread(t, 0, 0);
}

/* ---------- Fuente 5x7 ---------- */
/* A-Z (0-25), ? (26), ¿ (27), corazón (28) */
static const unsigned char FONT[29][7] = {
 {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
 {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},{0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},
 {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
 {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},{0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
 {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},{0x07,0x02,0x02,0x02,0x02,0x12,0x0C},
 {0x11,0x12,0x14,0x18,0x14,0x12,0x11},{0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
 {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},{0x11,0x11,0x19,0x15,0x13,0x11,0x11},
 {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
 {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
 {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},{0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
 {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},{0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
 {0x11,0x11,0x11,0x15,0x15,0x15,0x0A},{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
 {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
 {0x0E,0x11,0x01,0x02,0x04,0x00,0x04},{0x04,0x00,0x04,0x08,0x10,0x11,0x0E},
 {0x0A,0x1F,0x1F,0x1F,0x0E,0x04,0x00}
};

/* En los textos: [ = Í, ] = Ó, _ = ¿, ^ = corazón */
static void glyph(char ch, int x, int y, int s, u32 col)
{
    int idx = -1, accent = 0;
    if (ch >= 'A' && ch <= 'Z') idx = ch - 'A';
    else if (ch == '[') { idx = 8; accent = 1; }
    else if (ch == ']') { idx = 14; accent = 1; }
    else if (ch == '?') idx = 26;
    else if (ch == '_') idx = 27;
    else if (ch == '^') idx = 28;
    if (idx < 0) return;
    for (int r = 0; r < 7; r++)
        for (int c = 0; c < 5; c++)
            if (FONT[idx][r] & (0x10 >> c)) R(col, x + c * s, y + r * s, s, s);
    if (accent) R(col, x + 3 * s, y - 2 * s, s, s);
}

static void text(const char *str, int cx, int cy, int s, u32 col)
{
    int n = strlen(str), w = n * 6 * s - s, x = cx - w / 2, y = cy - (7 * s) / 2;
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < n; i++) {
            if (pass == 0) glyph(str[i], x + i * 6 * s + s, y + s, s, C(0x2a1a3a));
            else glyph(str[i], x + i * 6 * s, y, s, col);
        }
}

/* ---------- Música 8 bits ---------- */
#define SAMPLES 1024
#define STEPLEN 7497           /* 0.17 s a 44100 Hz */
#define NOTE_LEN 3969          /* 0.09 s */
static const int MEL[32] = {0,4,7,12,7,4,7,99, 5,9,12,17,12,9,12,99,
                            7,11,14,19,14,11,14,99, 12,7,4,7,12,99,99,99};
static const int BASS[4] = {-12,-7,-5,-12};
static const int SFXY[7] = {0,4,7,12,16,19,24};
static const int SFXN[3] = {-5,-7,-9};
static volatile int running = 1, sfxMode = 0, sfxPos = 0;

static int audio_thread(SceSize args, void *argp)
{
    int ch = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, SAMPLES, PSP_AUDIO_FORMAT_STEREO);
    if (ch < 0) return 0;
    static short buf[SAMPLES * 2];
    unsigned int n = 0;
    float p1 = 0, p2 = 0, p3 = 0, f1 = 0, f2 = 0;
    while (running) {
        for (int i = 0; i < SAMPLES; i++, n++) {
            unsigned int pos = n % STEPLEN;
            int st = (n / STEPLEN) % 32;
            if (pos == 0) {
                f1 = MEL[st] == 99 ? 0 : 523.25f * powf(2.0f, MEL[st] / 12.0f);
                f2 = (st % 2 == 0) ? 523.25f * powf(2.0f, (BASS[st / 8] - 12) / 12.0f) : 0;
            }
            float env = 1.0f - pos / (STEPLEN * 1.6f);
            float s = 0;
            if (f1 > 0) { p1 += f1 / 44100.0f; if (p1 >= 1) p1 -= 1; s += (p1 < .5f ? 1 : -1) * .07f * env; }
            if (f2 > 0) { p2 += f2 / 44100.0f; if (p2 >= 1) p2 -= 1; s += (4 * fabsf(p2 - .5f) - 1) * .16f * env; }
            if (sfxMode) {
                int idx = sfxPos / NOTE_LEN, cnt = sfxMode == 1 ? 7 : 3;
                if (idx >= cnt) sfxMode = 0;
                else {
                    int nt = sfxMode == 1 ? SFXY[idx] : SFXN[idx];
                    p3 += 523.25f * powf(2.0f, nt / 12.0f) / 44100.0f; if (p3 >= 1) p3 -= 1;
                    float e = 1.0f - (sfxPos % NOTE_LEN) / (float)NOTE_LEN;
                    s += (sfxMode == 1 ? (p3 < .5f ? 1 : -1) : (2 * p3 - 1)) * .10f * e;
                    sfxPos++;
                }
            }
            short v = (short)(s * 32000);
            buf[i * 2] = buf[i * 2 + 1] = v;
        }
        sceAudioOutputBlocking(ch, PSP_AUDIO_VOLUME_MAX, buf);
    }
    return 0;
}
static void sfx(int mode) { sfxPos = 0; sfxMode = mode; }

/* ---------- Dibujo de la escena ---------- */
static float T = 0;
static float cx_[3] = {40, 220, 380}, cy_[3] = {30, 55, 22}, cw_[3] = {56, 40, 64}, cs_[3] = {.12f, .08f, .15f};
static int flx[46], fly[46];
static const u32 FLC[4] = {C(0xff6b9a), C(0xfff27a), C(0xffffff), C(0xb58cff)};

static int rnd(void) { static unsigned int s = 12345; s = s * 1103515245u + 12345u; return (s >> 16) & 0x7fff; }
static int rr(float v) { return (int)floorf(v + 0.5f); }

static void background(void)
{
    static const u32 SKY[5] = {C(0x6ec6ff), C(0x86d0ff), C(0xa0dbff), C(0xbde6ff), C(0xd6f1ff)};
    for (int i = 0; i < 5; i++) R(SKY[i], 0, i * 26, W, 26);
    R(C(0xfff3a0), 410, 18, 22, 22); R(C(0xffe14a), 414, 22, 14, 14);
    for (int i = 0; i < 3; i++) {
        cx_[i] += cs_[i]; if (cx_[i] > W + 10) cx_[i] = -cw_[i] - 10;
        int x = (int)cx_[i], y = (int)cy_[i], w = (int)cw_[i];
        R(C(0xffffff), x + 8, y, w - 16, 8); R(C(0xffffff), x, y + 8, w, 10); R(C(0xdbeeff), x + 4, y + 16, w - 8, 3);
    }
    for (int x = 0; x < W; x += 4) R(C(0x5fae5a), x, (int)(105 + sinf(x / 55.0f) * 14), 4, 60);
    for (int x = 0; x < W; x += 4) R(C(0x4a9e4a), x, (int)(122 + sinf(x / 38.0f + 1.5f) * 10), 4, 60);
    R(C(0x58b84f), 0, 140, W, H - 140);
    for (int y = 140; y < H; y += 10)
        for (int x = ((y / 10) % 2) * 8; x < W; x += 16) {
            int sw = rr(sinf(T * 2 + x / 20.0f + y / 30.0f));
            R(C(0x6fd05f), x + sw, y, 2, 5); R(C(0x3f9a3f), x + 6 + sw, y + 4, 2, 4);
        }
    for (int i = 0; i < 46; i++) {
        int b = rr(sinf(T * 2 + i));
        R(C(0x2f7d3a), flx[i], fly[i] + 3 + b, 1, 4);
        R(FLC[i % 4], flx[i] - 1, fly[i] + b, 3, 3);
        R(C(0xffd23f), flx[i], fly[i] + 1 + b, 1, 1);
    }
}

static void boy(int x, int y)
{
    R(C(0x3a8a3a), x - 2, y + 30, 20, 3);
    R(C(0x5b3a1e), x + 3, y, 10, 5); R(C(0x5b3a1e), x + 2, y + 2, 2, 4);
    R(C(0xffd7b0), x + 4, y + 5, 8, 7); R(C(0x222222), x + 5, y + 8, 1, 2); R(C(0x222222), x + 10, y + 8, 1, 2);
    R(C(0xe87a7a), x + 7, y + 10, 2, 1);
    R(C(0x3a6fd8), x + 2, y + 12, 12, 10); R(C(0xffd7b0), x, y + 12, 2, 9); R(C(0xffd7b0), x + 14, y + 12, 2, 9);
    R(C(0x2b3a6b), x + 3, y + 22, 4, 8); R(C(0x2b3a6b), x + 9, y + 22, 4, 8);
    R(C(0xffffff), x + 3, y + 29, 4, 1); R(C(0xffffff), x + 9, y + 29, 4, 1);
}
static void girl(int x, int y)
{
    R(C(0x3a8a3a), x - 2, y + 30, 20, 3);
    R(C(0xc0742a), x + 2, y, 12, 6); R(C(0xc0742a), x + 1, y + 5, 3, 15); R(C(0xc0742a), x + 12, y + 5, 3, 15);
    R(C(0xffd7b0), x + 4, y + 5, 8, 7); R(C(0x222222), x + 5, y + 8, 1, 2); R(C(0x222222), x + 10, y + 8, 1, 2);
    R(C(0xe87a7a), x + 7, y + 10, 2, 1); R(C(0xff4d6d), x + 11, y, 4, 3);
    R(C(0xff8fb3), x + 3, y + 12, 10, 6); R(C(0xff8fb3), x + 2, y + 18, 12, 6); R(C(0xff8fb3), x + 1, y + 24, 14, 3);
    R(C(0xffd7b0), x + 5, y + 27, 2, 4); R(C(0xffd7b0), x + 9, y + 27, 2, 4);
    R(C(0xffd7b0), x + 1, y + 12, 2, 6); R(C(0xffd7b0), x + 13, y + 12, 2, 6);
}
static void heart(int x, int y, int s, u32 col)
{
    static const char *P[6] = {"0110110","1111111","1111111","0111110","0011100","0001000"};
    for (int j = 0; j < 6; j++) for (int i = 0; i < 7; i++) if (P[j][i] == '1') R(col, x + i * s, y + j * s, s, s);
}
static void scene(void)
{
    background();
    boy(186, 168 + rr(sinf(T * 3) * 1.5f));
    girl(278, 168 + rr(sinf(T * 3 + 1.6f) * 1.5f));
    heart(229, 150 + rr(sinf(T * 3) * 3), 3, C(0xff3d6e));
}

/* ---------- Fuegos artificiales ---------- */
typedef struct { float x, y, vx, vy, l; u32 c; } P;
static P fw[600]; static int fwn = 0; static float fwT = 0;
static void burst(void)
{
    static const u32 COL[5] = {C(0xff4d6d), C(0xffd23f), C(0x6ee7ff), C(0xb58cff), C(0x7dff8a)};
    float x = 60 + rnd() % 360, y = 30 + rnd() % 110; u32 c = COL[rnd() % 5];
    for (int i = 0; i < 28 && fwn < 600; i++) {
        float a = i / 28.0f * 6.2832f, s = 40 + rnd() % 30;
        fw[fwn++] = (P){x, y, cosf(a) * s, sinf(a) * s, 1.0f, c};
    }
}
static void fireworks(float dt)
{
    fwT -= dt; if (fwT <= 0) { burst(); fwT = .45f; }
    for (int i = 0; i < fwn; i++) {
        fw[i].x += fw[i].vx * dt; fw[i].y += fw[i].vy * dt; fw[i].vy += 60 * dt; fw[i].l -= dt * .7f;
        R(fw[i].c, (int)fw[i].x, (int)fw[i].y, 2, 2);
    }
    int k = 0; for (int i = 0; i < fwn; i++) if (fw[i].l > 0) fw[k++] = fw[i]; fwn = k;
}

/* ---------- Carteles ---------- */
static float modeT = 0;
static void sign(const char *l1, const char *l2, u32 col)
{
    R(C(0x2a1a3a), 90, 86, 300, 100); R(C(0x8a5a2b), 94, 90, 292, 92); R(C(0xc58a45), 98, 94, 284, 84);
    R(C(0xa56d2f), 98, 136, 284, 2);
    if (l2[0]) { text(l1, 240, 118, 3, col); text(l2, 240, 156, 2, C(0xfff6d6)); }
    else text(l1, 240, 136, 4, col);
    if (modeT > 1 && ((int)(T * 2)) % 2 == 0) text("PULSA UN BOT]N", 240, 200, 1, C(0xffffff));
}

/* ---------- Principal ---------- */
enum { ASK, YES, NO };

int main(void)
{
    setup_callbacks();
    sceDisplaySetMode(0, W, H);
    void *vram = (void *)0x44000000;
    sceDisplaySetFrameBuf((void *)0x04000000, STRIDE, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_NEXTFRAME);
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);

    for (int i = 0; i < 46; i++) {
        flx[i] = ((i * 9301 + 49297) % 233280) * W / 233280;
        fly[i] = 150 + ((i * 7919 + 13) % 1000) * 110 / 1000;
    }
    int at = sceKernelCreateThread("audio", audio_thread, 0x12, 0x10000, PSP_THREAD_ATTR_USER, NULL);
    if (at >= 0) sceKernelStartThread(at, 0, NULL);

    int mode = ASK, sel = 0;
    unsigned int old = 0;
    SceCtrlData pad;
    const float dt = 1.0f / 60.0f;

    while (1) {
        sceCtrlPeekBufferPositive(&pad, 1);
        unsigned int press = pad.Buttons & ~old; old = pad.Buttons;
        T += dt; modeT += dt;

        if (mode == ASK) {
            if (press & (PSP_CTRL_LEFT | PSP_CTRL_RIGHT)) sel = 1 - sel;
            if (press & (PSP_CTRL_CROSS | PSP_CTRL_CIRCLE)) {
                if (sel == 0) { mode = YES; sfx(1); } else { mode = NO; sfx(2); }
                modeT = 0; fwn = 0; fwT = 0;
            }
        } else if (modeT > 1 && (press & (PSP_CTRL_CROSS | PSP_CTRL_CIRCLE | PSP_CTRL_TRIANGLE | PSP_CTRL_SQUARE))) {
            mode = ASK; modeT = 0; fwn = 0;
        }

        scene();
        if (mode == ASK) {
            R(C(0x2a1a3a), 50, 10, 380, 40); R(C(0xfff6d6), 54, 14, 372, 32); R(C(0xffb3c8), 54, 14, 372, 3);
            text("_QUIERES SER MI NOVIA?", 240, 32, 2, C(0xc2185b));
            for (int i = 0; i < 2; i++) {
                int x = i == 0 ? 150 : 330, on = (sel == i);
                R(C(0x2a1a3a), x - 50, 222, 100, 30); R(on ? C(0xff4d6d) : C(0xfff6d6), x - 46, 226, 92, 22);
                text(i == 0 ? "S[" : "NO", x, 238, 2, on ? C(0xffffff) : C(0x7a4a6a));
                if (on) {
                    int k = ((int)(T * 4)) % 2 * 2;
                    static const int wd[7] = {1, 2, 3, 4, 3, 2, 1};
                    for (int r = 0; r < 7; r++) R(C(0xffffff), x - 66 + k, 231 + r * 2, wd[r] * 2, 2);
                }
            }
        } else if (mode == YES) {
            fireworks(dt);
            sign("TE AMO ^", "", C(0xd81b60));
        } else {
            sign("OPCI]N NO", "DISPONIBLE", C(0xc62828));
        }

        sceDisplayWaitVblankStart();
        memcpy(vram, fb, sizeof(fb));
    }
    return 0;
}
