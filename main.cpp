// ============================================================
//  ТИМУР и АрТеМ-DUBAI  —  2D игра
//  C++17 / Win32 GDI, без внешних библиотек
// ============================================================
#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <random>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <cstdio>

static const int W = 960, H = 540;
static const double PI = 3.14159265358979323846;

static HWND    gWnd = nullptr;
static HDC     gDC  = nullptr;   // back buffer
static HBITMAP gBmp = nullptr;
static bool    gKeys[256] = {};
static int     gMouseX = 0, gMouseY = 0;
static double  gTime = 0.0;
static bool    gMuted = false;
static double  gBanner2 = 0.0;

static std::mt19937 rng((unsigned)time(nullptr));
static double rnd(double a, double b) { return a + (b - a) * (double)(rng() % 100000) / 100000.0; }
static int    rndi(int a, int b) { if (b <= a) return a; return a + (int)(rng() % (unsigned)(b - a + 1)); }

// ------------------------------------------------------------
//  Цвет / примитивы
// ------------------------------------------------------------
static inline COLORREF CR(int r, int g, int b) { return RGB(r, g, b); }
static COLORREF mul(COLORREF c, float a) {
    int r = (int)(GetRValue(c) * a), g = (int)(GetGValue(c) * a), b = (int)(GetBValue(c) * a);
    return RGB(std::min(255, std::max(0, r)), std::min(255, std::max(0, g)), std::min(255, std::max(0, b)));
}
static COLORREF mix(COLORREF a, COLORREF b, float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    int r  = (int)(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t);
    int g  = (int)(GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t);
    int bl = (int)(GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t);
    return RGB(r, g, bl);
}

static HBRUSH brushFor(COLORREF c) {
    static std::map<COLORREF, HBRUSH> cache;
    auto it = cache.find(c);
    if (it != cache.end()) return it->second;
    HBRUSH b = CreateSolidBrush(c);
    cache[c] = b;
    return b;
}
static void fill(int x, int y, int w, int h, COLORREF c) {
    if (w <= 0 || h <= 0) return;
    RECT r = { x, y, x + w, y + h };
    FillRect(gDC, &r, brushFor(c));
}
static void fillRound(int x, int y, int w, int h, int rad, COLORREF c) {
    if (w <= 0 || h <= 0) return;
    HGDIOBJ ob = SelectObject(gDC, brushFor(c));
    HGDIOBJ op = SelectObject(gDC, GetStockObject(NULL_PEN));
    RoundRect(gDC, x, y, x + w, y + h, rad * 2, rad * 2);
    SelectObject(gDC, ob); SelectObject(gDC, op);
}
static void fillEllipse(int cx, int cy, int rx, int ry, COLORREF c) {
    if (rx <= 0 || ry <= 0) return;
    HGDIOBJ ob = SelectObject(gDC, brushFor(c));
    HGDIOBJ op = SelectObject(gDC, GetStockObject(NULL_PEN));
    Ellipse(gDC, cx - rx, cy - ry, cx + rx, cy + ry);
    SelectObject(gDC, ob); SelectObject(gDC, op);
}
static void line(int x1, int y1, int x2, int y2, COLORREF c, int wd = 1) {
    HPEN p = CreatePen(PS_SOLID, wd, c);
    HGDIOBJ op = SelectObject(gDC, p);
    MoveToEx(gDC, x1, y1, nullptr);
    LineTo(gDC, x2, y2);
    SelectObject(gDC, op);
    DeleteObject(p);
}
static void arcStroke(int cx, int cy, int rx, int ry, double a0, double a1, COLORREF col, int wd) {
    if (rx <= 0 || ry <= 0) return;
    HPEN p = CreatePen(PS_SOLID, std::max(1, wd), col);
    HGDIOBJ op = SelectObject(gDC, p);
    HGDIOBJ ob = SelectObject(gDC, GetStockObject(NULL_BRUSH));
    Arc(gDC, cx - rx, cy - ry, cx + rx, cy + ry,
        (int)(cx + rx * cos(a0)), (int)(cy + ry * sin(a0)),
        (int)(cx + rx * cos(a1)), (int)(cy + ry * sin(a1)));
    SelectObject(gDC, op); SelectObject(gDC, ob);
    DeleteObject(p);
}
static void fillA(int x, int y, int w, int h, COLORREF c, int a) {
    if (w <= 0 || h <= 0 || a <= 0) return;
    static std::map<COLORREF, HBITMAP> bits;
    static HDC sdc = nullptr;
    if (!sdc) sdc = CreateCompatibleDC(gDC);
    HBITMAP bm;
    auto it = bits.find(c);
    if (it == bits.end()) {
        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = 1;
        bi.bmiHeader.biHeight = 1;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        void* pv = nullptr;
        bm = CreateDIBSection(gDC, &bi, DIB_RGB_COLORS, &pv, nullptr, 0);
        if (!bm) return;
        DWORD* px = (DWORD*)pv;
        *px = (0xFFu << 24) | (GetBValue(c) << 16) | (GetGValue(c) << 8) | GetRValue(c);
        bits[c] = bm;
    } else bm = it->second;
    HGDIOBJ old = SelectObject(sdc, bm);
    BLENDFUNCTION bf;
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = (BYTE)std::min(255, std::max(0, a));
    bf.AlphaFormat = 0;
    AlphaBlend(gDC, x, y, w, h, sdc, 0, 0, 1, 1, bf);
    SelectObject(sdc, old);
}
static void star(int cx, int cy, int r1, int r2, int n, COLORREF c) {
    if (r1 <= 0) return;
    std::vector<POINT> pts;
    for (int i = 0; i < n * 2; i++) {
        double a = -PI / 2 + i * PI / n;
        int r = (i % 2 == 0) ? r1 : r2;
        pts.push_back({ (LONG)(cx + cos(a) * r), (LONG)(cy + sin(a) * r) });
    }
    HGDIOBJ ob = SelectObject(gDC, brushFor(c));
    HGDIOBJ op = SelectObject(gDC, GetStockObject(NULL_PEN));
    Polygon(gDC, pts.data(), (int)pts.size());
    SelectObject(gDC, ob); SelectObject(gDC, op);
}
static bool ptIn(int x, int y, int w, int h) {
    return gMouseX >= x && gMouseX <= x + w && gMouseY >= y && gMouseY <= y + h;
}

// ------------------------------------------------------------
//  Звук (waveOut, без библиотек)
// ------------------------------------------------------------
static HWAVEOUT gWave = nullptr;
static const int SR = 22050;

struct SndBuf {
    WAVEHDR hdr;
    std::vector<short> data;
    bool prepared = false;
};
static std::vector<SndBuf*> gSnds;

static void audioInit() {
    if (gWave) return;
    WAVEFORMATEX wf = {};
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = 1;
    wf.nSamplesPerSec = SR;
    wf.wBitsPerSample = 16;
    wf.nBlockAlign = 2;
    wf.nAvgBytesPerSec = SR * 2;
    wf.cbSize = 0;
    if (waveOutOpen(&gWave, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
        gWave = nullptr;
}
static void sfx(double freq, double dur = 0.08, int type = 0, double vol = 0.18, double slide = 0.0) {
    if (gMuted) return;
    audioInit();
    if (!gWave) return;
    if (dur > 0.30) dur = 0.30;
    int n = (int)(SR * dur);
    if (n < 20) n = 20;

    SndBuf* b = nullptr;
    if (gSnds.size() < 10) {
        b = new SndBuf();
        b->hdr = {};
        gSnds.push_back(b);
    } else {
        for (auto* s : gSnds) {
            if (!s->prepared || (s->hdr.dwFlags & WHDR_DONE)) { b = s; break; }
        }
        if (!b) b = gSnds[rndi(0, (int)gSnds.size() - 1)];
    }
    if (b->prepared) {
        waveOutUnprepareHeader(gWave, &b->hdr, sizeof(WAVEHDR));
        b->prepared = false;
    }
    b->data.resize(n);
    double ph = 0.0;
    for (int i = 0; i < n; i++) {
        double t = (double)i / SR;
        double f = freq + slide * (t / dur);
        if (f < 25) f = 25;
        if (f > 9000) f = 9000;
        ph += 2.0 * PI * f / SR;
        double v = 0.0;
        if (type == 0)      v = (sin(ph) >= 0) ? 1.0 : -1.0;                    // square
        else if (type == 1) v = sin(ph);                                         // sine
        else if (type == 2) v = 2.0 * fabs(2.0 * (ph / (2 * PI) - floor(ph / (2 * PI) + 0.5))) - 1.0; // triangle
        else                v = 2.0 * (ph / (2 * PI) - floor(ph / (2 * PI) + 0.5)); // saw
        double env = std::min(1.0, t / 0.005) * std::min(1.0, (dur - t) / 0.02);
        double s = v * vol * env * 32767.0;
        b->data[i] = (short)std::max(-32000.0, std::min(32000.0, s));
    }
    b->hdr.lpData = (LPSTR)b->data.data();
    b->hdr.dwBufferLength = (DWORD)(b->data.size() * sizeof(short));
    b->hdr.dwFlags = 0;
    b->hdr.dwLoops = 0;
    waveOutPrepareHeader(gWave, &b->hdr, sizeof(WAVEHDR));
    b->prepared = true;
    waveOutWrite(gWave, &b->hdr, sizeof(WAVEHDR));
}
static void audioShutdown() {
    if (!gWave) return;
    waveOutReset(gWave);
    for (auto* s : gSnds) {
        if (s->prepared) waveOutUnprepareHeader(gWave, &s->hdr, sizeof(WAVEHDR));
        delete s;
    }
    gSnds.clear();
    waveOutClose(gWave);
    gWave = nullptr;
}

// ------------------------------------------------------------
//  Шрифты и текст
// ------------------------------------------------------------
static HFONT fontFor(int px, bool heavy, const std::wstring& fam) {
    static std::map<std::wstring, HFONT> cache;
    wchar_t key[160];
    swprintf(key, 160, L"%d|%d|%s", px, heavy ? 1 : 0, fam.c_str());
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    HFONT f = CreateFontW(-px, 0, 0, 0, heavy ? FW_HEAVY : FW_BOLD, FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, fam.c_str());
    cache[key] = f;
    return f;
}
static int textW(const std::wstring& s, int px, bool heavy = false, const std::wstring& fam = L"Trebuchet MS") {
    HFONT f = fontFor(px, heavy, fam);
    HGDIOBJ old = SelectObject(gDC, f);
    SIZE sz = { 0, 0 };
    GetTextExtentPoint32W(gDC, s.c_str(), (int)s.size(), &sz);
    SelectObject(gDC, old);
    return sz.cx;
}
static void drawText(const std::wstring& s, int x, int y, int px, COLORREF col,
                     int align = 0, bool heavy = false, const std::wstring& fam = L"Trebuchet MS") {
    if (s.empty()) return;
    int w = textW(s, px, heavy, fam);
    int dx = (align == 1) ? x - w / 2 : (align == 2) ? x - w : x;
    HFONT f = fontFor(px, heavy, fam);
    HGDIOBJ old = SelectObject(gDC, f);
    SetBkMode(gDC, TRANSPARENT);
    SetTextColor(gDC, col);
    TextOutW(gDC, dx, y, s.c_str(), (int)s.size());
    SelectObject(gDC, old);
}
static int clampI(int v, int a, int b) { return v < a ? a : (v > b ? b : v); }
static std::vector<std::wstring> wrapText(const std::wstring& s, int px, int maxW, bool heavy = false,
                                          const std::wstring& fam = L"Trebuchet MS") {
    std::vector<std::wstring> out;
    std::wstring cur, word;
    auto flush = [&]() {
        if (word.empty()) return;
        if (cur.empty()) cur = word;
        else {
            std::wstring t = cur + L" " + word;
            if (textW(t, px, heavy, fam) > maxW) { out.push_back(cur); cur = word; }
            else cur = t;
        }
        word.clear();
    };
    for (wchar_t ch : s) {
        if (ch == L' ') flush();
        else word += ch;
    }
    flush();
    if (!cur.empty()) out.push_back(cur);
    return out;
}
// ------------------------------------------------------------
//  Персонажи
// ------------------------------------------------------------
struct CharCfg {
    COLORREF jacket, jacket2, pants, hair, skin;
    bool headphones = false, chain = false, buzz = true;
    int  eye = 0;      // 0 нет, 1 бирюзовый, 2 золотой, 3 красный
    int  mouth = 0;    // 0 прямая, 1 ухмылка, 2 улыбка
    float wide = 1.0f;
    bool glasses = false, stain = false, beard = false;
    float tall = 1.0f;
};
static CharCfg CFG_TIMUR, CFG_ARTEM, CFG_LEHA, CFG_GRISHA, CFG_DIMA, CFG_RENAT;

static void initChars() {
    CFG_TIMUR  = { CR(46,111,142), CR(78,157,187), CR(38,50,74),  CR(36,26,18),  CR(242,200,162), false, false, false, 0, 1, 1.00f };
    CFG_ARTEM  = { CR(95,143,42),  CR(134,184,63), CR(43,47,58),  CR(28,28,34),  CR(232,185,143), true,  true,  true,  1, 2, 1.00f };
    CFG_LEHA   = { CR(29,29,36),   CR(47,47,60),   CR(21,21,27),  CR(16,16,20),  CR(217,169,127), false, false, true,  0, 0, 1.28f };
    CFG_GRISHA = { CR(18,16,24),   CR(42,33,48),   CR(13,12,18),  CR(10,10,14),  CR(201,162,122), false, false, true,  2, 0, 1.06f };
    CFG_DIMA   = { CR(150,178,168), CR(190,208,198), CR(54,62,74), CR(58,42,30), CR(238,206,180), false, false, false, 0, 0, 0.86f, true, true, false, 1.16f };
    CFG_RENAT  = { CR(70,54,60),   CR(96,76,84),   CR(34,30,36),  CR(30,24,22),  CR(214,176,142), false, false, true,  0, 0, 1.10f, false, false, true, 1.0f };
}

static void arcPoly(int cx, int cy, int rx, int ry, double a0, double a1, COLORREF col, int wd, int seg = 14) {
    for (int i = 0; i < seg; i++) {
        double t0 = a0 + (a1 - a0) * i / seg;
        double t1 = a0 + (a1 - a0) * (i + 1) / seg;
        line(cx + (int)(rx * cos(t0)), cy + (int)(ry * sin(t0)),
             cx + (int)(rx * cos(t1)), cy + (int)(ry * sin(t1)), col, wd);
    }
}

static void drawChar(int x, int y, float s, const CharCfg& c, bool flip, float alpha, double t, bool bob = true) {
    float w = c.wide;
    double bobY = bob ? sin(t * 3.2) * 2.0 : 0.0;

    auto X = [&](float lx) { return (int)lround(x + (flip ? -lx : lx) * s * w); };
    auto Y = [&](float ly) { return (int)lround(y + ly * s); };
    auto B = [&](float ly) { return (int)lround(y + (ly + (float)bobY) * s); };
    auto C = [&](COLORREF col) { return mul(col, alpha); };
    auto FW = [&](float k) { return (int)std::max(1.0f, k * s * w); };
    auto RR = [&](float lx, float ly, int ww, int hh, int rad, COLORREF col) {
        int xx = flip ? X(lx) - ww : X(lx);
        fillRound(xx, B(ly), ww, hh, rad, col);
    };
    auto RF = [&](float lx, float ly, int ww, int hh, COLORREF col) {
        int xx = flip ? X(lx) - ww : X(lx);
        fill(xx, B(ly), ww, hh, col);
    };

    fillEllipse(X(0), Y(0), (int)(36 * s * w), (int)(9 * s), mul(CR(0, 0, 0), alpha * 0.35f));

    // ноги и обувь
    RR(-24, -62, FW(20), (int)(58 * s), (int)(6 * s), C(c.pants));
    RR(4,   -62, FW(20), (int)(58 * s), (int)(6 * s), C(c.pants));
    RR(-28, -12, FW(26), (int)(12 * s), (int)(4 * s), C(CR(21, 21, 29)));
    RR(2,   -12, FW(26), (int)(12 * s), (int)(4 * s), C(CR(21, 21, 29)));

    // куртка
    RR(-30, -140, FW(60), (int)(86 * s), (int)(14 * s), C(c.jacket));
    RR(-30, -140, FW(60), (int)(16 * s), (int)(8 * s),  C(c.jacket2));
    RF(-2, -136, FW(4), (int)(74 * s), mul(CR(0, 0, 0), alpha * 0.25f));
    if (c.stain) {
        fillEllipse(X(6), B(-108), (int)(17 * s * w), (int)(14 * s), C(mix(c.jacket, CR(196, 196, 120), 0.45f)));
        fillEllipse(X(2), B(-96), (int)(9 * s * w), (int)(7 * s), C(mix(c.jacket, CR(196, 196, 120), 0.30f)));
    }

    // руки
    RR(-44, -134, FW(16), (int)(64 * s), (int)(8 * s), C(c.jacket));
    RR(28,  -134, FW(16), (int)(64 * s), (int)(8 * s), C(c.jacket));
    fillEllipse(X(-36), B(-66), (int)(9 * s * w), (int)(9 * s), C(c.skin));
    fillEllipse(X(36),  B(-66), (int)(9 * s * w), (int)(9 * s), C(c.skin));

    // шея и голова
    RF(-9, -152, FW(18), (int)(16 * s), C(c.skin));
    fillEllipse(X(0), B(-176), (int)(30 * s * w), (int)(33 * s), C(c.skin));
    fillEllipse(X(-29), B(-172), (int)(6 * s * w), (int)(8 * s), C(c.skin));
    fillEllipse(X(29),  B(-172), (int)(6 * s * w), (int)(8 * s), C(c.skin));

    // волосы
    if (c.buzz) {
        fillEllipse(X(0), B(-200), (int)(31 * s * w), (int)(20 * s), C(c.hair));
    } else {
        fillEllipse(X(0), B(-198), (int)(33 * s * w), (int)(24 * s), C(c.hair));
        fillEllipse(X(-14), B(-183), (int)(16 * s * w), (int)(10 * s), C(c.hair));
    }

    // борода
    if (c.beard) {
        fillEllipse(X(-8), B(-160), (int)(10 * s * w), (int)(5 * s), C(c.hair));
        fillEllipse(X(8),  B(-160), (int)(10 * s * w), (int)(5 * s), C(c.hair));
        fillEllipse(X(0),  B(-148), (int)(13 * s * w), (int)(6 * s), C(c.hair));
        fillEllipse(X(-20), B(-164), (int)(6 * s * w), (int)(10 * s), C(c.hair));
        fillEllipse(X(20),  B(-164), (int)(6 * s * w), (int)(10 * s), C(c.hair));
    }

    // глаза
    fillEllipse(X(-11), B(-171), (int)(3.4 * s * w), (int)(3.8 * s), C(CR(30, 18, 22)));
    if (c.eye) {
        COLORREF ec = (c.eye == 1) ? CR(73, 230, 255) : (c.eye == 2) ? CR(255, 210, 74) : CR(255, 68, 68);
        for (int k = 3; k >= 1; k--) {
            float tt = 0.16f * (4 - k);
            fillEllipse(X(11), B(-171), (int)((4 + k * 3) * s * w), (int)((4 + k * 3) * s), C(mix(c.skin, ec, tt)));
        }
        fillEllipse(X(11), B(-171), (int)(5 * s * w), (int)(5 * s), C(ec));
        fillEllipse(X(11), B(-171), (int)(2 * s * w), (int)(2 * s), C(CR(255, 255, 255)));
    } else {
        fillEllipse(X(11), B(-171), (int)(3.4 * s * w), (int)(3.8 * s), C(CR(30, 18, 22)));
    }

    // очки
    if (c.glasses) {
        COLORREF gc = C(CR(32, 32, 40));
        arcPoly(X(-12), B(-172), (int)(10 * s * w), (int)(10 * s), 0, 2 * PI, gc, std::max(1, (int)(2 * s)), 16);
        arcPoly(X(12),  B(-172), (int)(10 * s * w), (int)(10 * s), 0, 2 * PI, gc, std::max(1, (int)(2 * s)), 16);
        line(X(-2), B(-172), X(2), B(-172), gc, std::max(1, (int)(2 * s)));
        line(X(-22), B(-176), X(-30), B(-180), gc, std::max(1, (int)(2 * s)));
        line(X(22),  B(-176), X(30),  B(-180), gc, std::max(1, (int)(2 * s)));
    }

    // рот
    {
        int mw = (int)(8 * s * w), mh = (int)(4 * s);
        int mx = X(0), my = B(-154);
        COLORREF mc = C(CR(120, 58, 58));
        int wd = std::max(1, (int)(2.5 * s));
        if (c.mouth == 0) line(mx - mw, my, mx + mw, my, mc, wd);
        else if (c.mouth == 1) { line(mx - mw, my, mx + mw / 3, my + mh, mc, wd); line(mx + mw / 3, my + mh, mx + mw, my - mh / 2, mc, wd); }
        else { line(mx - mw, my, mx - mw / 2, my + mh, mc, wd); line(mx - mw / 2, my + mh, mx + mw / 2, my + mh, mc, wd); line(mx + mw / 2, my + mh, mx + mw, my, mc, wd); }
    }

    // наушники
    if (c.headphones) {
        arcPoly(X(0), B(-196), (int)(38 * s * w), (int)(38 * s), PI * 1.04, PI * 1.96,
                C(CR(220, 52, 63)), std::max(2, (int)(7 * s)));
        RR(-42, -202, FW(16), (int)(30 * s), (int)(7 * s), C(CR(216, 52, 63)));
        RR(26,  -202, FW(16), (int)(30 * s), (int)(7 * s), C(CR(216, 52, 63)));
    }

    // цепь
    if (c.chain) {
        arcPoly(X(0), B(-138), (int)(15 * s * w), (int)(13 * s), PI * 0.12, PI * 0.88,
                C(CR(255, 210, 74)), std::max(1, (int)(3 * s)), 8);
    }
}

static void drawAvatar(int cx, int cy, int rad, const CharCfg& c, double t) {
    fillEllipse(cx, cy, rad, rad, CR(24, 32, 50));
    HRGN rgn = CreateEllipticRgn(cx - rad, cy - rad, cx + rad, cy + rad);
    SelectClipRgn(gDC, rgn);
    drawChar(cx, cy + (int)(176.0f * rad / 48.0f), (float)rad / 48.0f, c, false, 1.0f, t, true);
    SelectClipRgn(gDC, nullptr);
    DeleteObject(rgn);
    // обводка кольцом
    arcPoly(cx, cy, rad, rad, 0, 2 * PI, CR(90, 110, 150), 3, 40);
}

// ------------------------------------------------------------
//  Фон: город / школа / туалет
// ------------------------------------------------------------
struct Flake { float x, y, r, vy, vx; };
static std::vector<Flake> gSnow;
static void initSnow() {
    gSnow.clear();
    for (int i = 0; i < 70; i++) gSnow.push_back({ (float)rnd(0, W), (float)rnd(0, H), (float)rnd(1, 2.8), (float)rnd(16, 46), (float)rnd(-10, 10) });
}
static void updateSnow(double dt) {
    for (auto& f : gSnow) {
        f.y += f.vy * (float)dt;
        f.x += f.vx * (float)dt;
        if (f.y > H + 4) { f.y = -4; f.x = (float)rnd(0, W); }
        if (f.x < -4) f.x = (float)W; if (f.x > W + 4) f.x = 0;
    }
}
static void drawSnow() {
    for (auto& f : gSnow) fillEllipse((int)f.x, (int)f.y, (int)f.r, (int)f.r, CR(215, 224, 240));
}

static void bgCity() {
    for (int y = 0; y < H; y += 6) {
        float t = (float)y / H;
        fill(0, y, W, 6, mix(CR(9, 8, 20), CR(40, 28, 62), t));
    }
    fillEllipse(790, 108, 56, 56, mix(mix(CR(9, 8, 20), CR(40, 28, 62), 0.2f), CR(255, 240, 200), 0.16f));
    fillEllipse(790, 108, 46, 46, CR(255, 240, 200));
    fillEllipse(790, 108, 40, 40, CR(255, 248, 224));
    fillEllipse(760, 96, 8, 8, CR(226, 218, 190));
    fillEllipse(802, 124, 6, 6, CR(230, 222, 194));

    // панельки
    for (int i = 0; i < 9; i++) {
        int bw = 118;
        int bh = 130 + ((i * 47) % 150);
        int x = -30 + i * 112;
        int y = 430 - bh;
        fill(x, y, bw, bh + 60, CR(17, 15, 32));
        fill(x, y, bw, 8, CR(30, 26, 52));
        for (int wy = y + 16; wy < 410; wy += 26) {
            for (int wx = x + 12; wx < x + bw - 14; wx += 24) {
                int k = (wx * 31 + wy * 17) % 13;
                if (k < 3) fill(wx, wy, 11, 13, CR(214, 186, 110));
                else if (k < 5) fill(wx, wy, 11, 13, CR(96, 108, 150));
            }
        }
    }
    // дорога
    fill(0, 430, W, H - 430, CR(28, 28, 36));
    fill(0, 430, W, 4, CR(52, 52, 64));
    for (int x = 0; x < W; x += 90) fill(x, 492, 52, 5, CR(170, 160, 120));

    // неоновая вывеска
    fillRound(24, 104, 220, 48, 8, CR(24, 10, 26));
    drawText(L"БАРНАУЛ", 134, 112, 26, CR(255, 90, 160), 1, true, L"Arial Black");
    drawText(L"БАРНАУЛ", 134, 112, 26, CR(255, 160, 200), 1, true, L"Arial Black");

    drawSnow();
}

static void bgSchool() {
    for (int y = 0; y < 356; y += 8) fill(0, y, W, 8, mix(CR(38, 46, 68), CR(96, 106, 136), (float)y / 356));
    // дальние дома
    for (int i = 0; i < 8; i++) {
        int h = 90 + ((i * 53) % 80);
        fill(-40 + i * 132, 350 - h, 122, h + 20, CR(30, 34, 50));
        for (int wy = 350 - h + 14; wy < 330; wy += 26)
            for (int wx = -30 + i * 132; wx < -30 + i * 132 + 104; wx += 24)
                if ((wx * 13 + wy * 7) % 17 < 3) fill(wx, wy, 10, 12, CR(190, 170, 100));
    }
    // школа
    fill(40, 44, 880, 296, CR(94, 70, 78));
    fill(30, 32, 900, 16, CR(58, 42, 50));
    fill(40, 44, 880, 10, CR(120, 92, 100));
    for (int r = 0; r < 3; r++) for (int c = 0; c < 8; c++) {
        int x = 76 + c * 108, y = 84 + r * 66;
        bool lit = ((c * 7 + r * 3) % 5) < 3;
        fill(x - 3, y - 3, 72, 52, CR(70, 50, 58));
        fill(x, y, 66, 46, lit ? CR(226, 204, 138) : CR(48, 58, 80));
        fill(x + 29, y, 8, 46, CR(94, 70, 78));
    }
    fillRound(352, 52, 256, 46, 8, CR(150, 30, 40));
    drawText(L"ШКОЛА №125", 480, 60, 26, CR(255, 238, 224), 1, true, L"Arial Black");

    // вход
    fill(418, 222, 124, 118, CR(58, 44, 50));
    fill(434, 236, 92, 104, CR(150, 178, 186));
    fill(470, 236, 20, 104, CR(58, 44, 50));
    fill(440, 244, 24, 40, CR(196, 214, 214));
    fill(496, 244, 24, 40, CR(196, 214, 214));
    fill(396, 340, 168, 10, CR(128, 128, 136));
    fill(388, 350, 184, 10, CR(108, 108, 118));

    // земля
    fill(0, 360, W, H - 360, CR(52, 52, 62));
    fill(0, 340, W, 18, CR(96, 96, 106));
    for (int i = 0; i < 18; i++) {
        int x = (i * 137) % W, y = 380 + (i * 71) % 130;
        fillEllipse(x, y, 18 + (i * 13) % 24, 6 + (i * 7) % 8, CR(70, 72, 86));
    }
    // сугробы
    for (int i = 0; i < 12; i++) {
        int x = (i * 173 + 40) % W, y = 400 + (i * 61) % 120;
        fillEllipse(x, y, 26, 8, CR(200, 208, 224));
    }
    // фонари
    for (int lx : { 130, 830 }) {
        fill(lx - 4, 220, 8, 150, CR(40, 42, 54));
        fill(lx - 16, 214, 32, 10, CR(40, 42, 54));
        fillEllipse(lx, 226, 26, 26, CR(70, 66, 52));
        fillEllipse(lx, 226, 12, 12, CR(255, 238, 170));
        fillA(lx - 30, 210, 60, 60, CR(255, 230, 150), 26);
    }
    // деревья
    for (int tx : { 60, 900 }) {
        fill(tx - 6, 300, 12, 70, CR(48, 36, 30));
        fillEllipse(tx, 288, 44, 50, CR(30, 48, 40));
        fillEllipse(tx - 14, 268, 22, 18, CR(215, 222, 236));
        fillEllipse(tx + 16, 300, 18, 14, CR(215, 222, 236));
    }
    drawSnow();
}

static void bgToilet() {
    fill(0, 0, W, H, CR(66, 82, 81));
    for (int y = 0; y < 404; y += 48) {
        for (int x = 0; x < W; x += 48) {
            bool alt = (((x / 48) + (y / 48)) % 2 == 0);
            fill(x + 1, y + 1, 46, 46, alt ? CR(78, 96, 95) : CR(70, 87, 86));
            fill(x + 1, y + 1, 46, 2, CR(96, 116, 114));
        }
    }
    fill(0, 0, W, 34, CR(52, 66, 65));
    fill(0, 404, W, H - 404, CR(50, 59, 58));
    for (int y = 404; y < H; y += 42) fill(0, y, W, 2, CR(38, 46, 45));
    for (int x = -420; x < W + 420; x += 110)
        line(x, 404, (int)(480 + (x - 480) * 2.6f), H, CR(40, 48, 47), 2);

    // кабинки
    fill(688, 132, 236, 272, CR(58, 73, 72));
    for (int i = 0; i < 2; i++) {
        fill(706 + i * 112, 152, 96, 240, CR(89, 107, 104));
        fill(714 + i * 112, 168, 80, 10, CR(56, 68, 66));
        fillEllipse(752 + i * 112, 172, 5, 5, CR(40, 48, 47));
        fill(730 + i * 112, 300, 52, 6, CR(56, 68, 66));
    }
    // раковины
    fill(24, 236, 128, 20, CR(146, 156, 156));
    fill(24, 256, 128, 30, CR(120, 130, 130));
    fillRound(40, 250, 80, 22, 8, CR(178, 188, 188));
    fillRound(40, 250, 80, 10, 7, CR(200, 210, 210));
    fill(76, 232, 8, 20, CR(150, 160, 160));
    fill(28, 286, 120, 6, CR(90, 100, 100));

    // свет
    fill(420, 0, 120, 14, CR(186, 204, 192));
    fillA(360, 0, 240, 300, CR(200, 226, 206), 16);
    fillEllipse(480, 40, 70, 26, CR(96, 112, 110));

    // граффити
    drawText(L"АрТеМ + DUBAI", 806, 92, 18, CR(128, 44, 76), 1);
    drawText(L"дубай", 300, 330, 16, CR(96, 120, 100), 0);
    // «творчество» АрТеМа
    line(360, 300, 372, 322, CR(120, 60, 80), 3);
    line(372, 322, 384, 300, CR(120, 60, 80), 3);
    fillEllipse(370, 316, 4, 7, CR(120, 60, 80));
    fillEllipse(378, 316, 4, 7, CR(120, 60, 80));
}
// ------------------------------------------------------------
//  Сцены: каркас
// ------------------------------------------------------------
enum Scene { S_TITLE, S_CUTSCENE, S_WALK, S_CARDS, S_FIGHT, S_END, S_QUEUE, S_DIMA, S_REMATCH, S_END2 };
static Scene gScene = S_TITLE;

static void saveBMP(const wchar_t* path);
static void autoStep(double dt);
static void autoStep2(double dt);
static bool gAuto = false;
static bool gAuto2 = false;
static double gAutoT = 0;
static int gAutoStep = 0;

struct Card { int v = 0; int s = 0; bool joker = false; };

// forward
static void startIntro();
static void startCards();
static void startFight();
static void restartGame();
static void meetStart();
static void startQueue();
static void startShopScene();
static void startPart2();
static void startDima();
static void startRematch();
static void goShop();
static void drawQueue();
static void drawDima();
static void drawRematch();
static void drawEnd2();
static void updateQueue(double dt);
static void updateDima(double dt);
static void updateRematch(double dt);
static void onQueuePush();
static void onDimaClick();
static void onRematchClick(int x, int y);
static void drawCardShape(int x, int y, int w, int h, const Card& c, bool faceDown, bool dim, bool glitch);
static void bgCorridor();
static void bgCanteen();
static void bgShopStreet();
static void bgHome();
static void bgClass302();

// ------------------------------------------------------------
//  Катсцены
// ------------------------------------------------------------
struct Line { std::wstring who, text, cast, bg; };

static std::wstring csBg;
static std::vector<Line> csLines;
static int csIdx = 0;
static double csShown = 0;
static std::function<void()> csDone;

static const CharCfg* cfgFor(const std::wstring& id) {
    if (id == L"timur")  return &CFG_TIMUR;
    if (id == L"artem")  return &CFG_ARTEM;
    if (id == L"leha")   return &CFG_LEHA;
    if (id == L"grisha") return &CFG_GRISHA;
    if (id == L"dima")   return &CFG_DIMA;
    if (id == L"renat")  return &CFG_RENAT;
    return nullptr;
}
static std::wstring speakerName(const std::wstring& id) {
    if (id == L"timur") return L"ТИМУР";
    if (id == L"artem") return L"АрТеМ-DUBAI";
    if (id == L"leha")  return L"ПЛАТОН-ЛЁХА";
    if (id == L"dima")  return L"ДиМа-zxcpipin31";
    if (id == L"renat") return L"РЕНАТ (отец)";
    if (id == L"grisha") return L"ГрИшКа-ЗоЛоТоЙ-гЛаЗ";
    if (id == L"mom")   return L"МАМА (из окна)";
    return L"";
}
static COLORREF speakerColor(const std::wstring& id) {
    if (id == L"timur") return CR(79, 210, 255);
    if (id == L"artem") return CR(125, 255, 94);
    if (id == L"leha")  return CR(255, 122, 217);
    if (id == L"dima")  return CR(190, 208, 160);
    if (id == L"renat") return CR(255, 184, 77);
    if (id == L"grisha") return CR(255, 210, 74);
    if (id == L"mom")   return CR(255, 184, 77);
    return CR(160, 152, 200);
}
static std::wstring defaultCast(const std::wstring& bg) {
    if (bg == L"school") return L"timur,artem";
    if (bg == L"toilet") return L"timur,artem";
    if (bg == L"canteen") return L"timur,dima";
    if (bg == L"class302") return L"timur,grisha";
    if (bg == L"home") return L"timur,renat";
    if (bg == L"shop") return L"timur,dima";
    return L"timur";
}
static void drawBG(const std::wstring& name) {
    if (name == L"school") bgSchool();
    else if (name == L"toilet") bgToilet();
    else if (name == L"corridor") bgCorridor();
    else if (name == L"canteen") bgCanteen();
    else if (name == L"shop") bgShopStreet();
    else if (name == L"home") bgHome();
    else if (name == L"class302") bgClass302();
    else bgCity();
}
static void playCS(std::vector<Line> lines, const std::wstring& bg, std::function<void()> done) {
    csLines = std::move(lines);
    csIdx = 0;
    csShown = 0;
    csBg = bg;
    csDone = done;
    gScene = S_CUTSCENE;
}
static void advanceCS() {
    if (csIdx < 0 || csIdx >= (int)csLines.size()) return;
    const std::wstring& t = csLines[csIdx].text;
    if (csShown < (double)t.size()) { csShown = (double)t.size(); sfx(700, 0.03, 1, 0.05); return; }
    csIdx++;
    csShown = 0;
    if (csIdx >= (int)csLines.size()) {
        auto d = csDone;
        csDone = nullptr;
        csLines.clear();
        if (d) d();
    } else sfx(440, 0.04, 1, 0.06);
}
static void updateCS(double dt) {
    if (csIdx < 0 || csIdx >= (int)csLines.size()) return;
    double full = (double)csLines[csIdx].text.size();
    if (csShown < full) csShown = std::min(full, csShown + dt * 58.0);
}
static void drawCS() {
    if (csIdx < 0 || csIdx >= (int)csLines.size()) return;
    const Line& L = csLines[csIdx];
    if (!L.bg.empty()) csBg = L.bg;
    drawBG(csBg);
    fillA(0, 0, W, H, CR(0, 0, 0), 70);

    std::wstring cast = L.cast.empty() ? defaultCast(csBg) : L.cast;
    std::vector<std::wstring> ids;
    {
        std::wstring cur;
        for (size_t i = 0; i <= cast.size(); i++) {
            if (i == cast.size() || cast[i] == L',') { if (!cur.empty()) ids.push_back(cur); cur.clear(); }
            else cur += cast[i];
        }
    }
    for (size_t i = 0; i < ids.size(); i++) {
        const CharCfg* cfg = cfgFor(ids[i]);
        if (!cfg) continue;
        int x = (ids.size() == 1) ? 250 : (i == 0 ? 190 : 770);
        bool speak = (L.who == ids[i]);
        drawChar(x, 474, cfg->tall, *cfg, x > 480, speak ? 1.0f : 0.40f, gTime, speak);
    }

    fillRound(46, 386, 868, 144, 18, CR(88, 74, 138));
    fillRound(50, 390, 860, 136, 15, CR(8, 8, 16));

    if (L.who != L"narr") {
        std::wstring nm = speakerName(L.who);
        if (!nm.empty()) {
            int tw = textW(nm, 20, true) + 30;
            fillRound(72, 362, tw, 36, 10, speakerColor(L.who));
            drawText(nm, 72 + 15, 368, 20, CR(10, 10, 18), 0, true);
        }
    }

    std::wstring vis = L.text.substr(0, (size_t)csShown);
    auto rows = wrapText(vis, 22, 800);
    int yy = 412;
    for (auto& s : rows) { drawText(s, 78, yy, 22, CR(234, 230, 246)); yy += 32; }

    if (csShown >= (double)L.text.size() && (int)(gTime * 2.4) % 2 == 0)
        drawText(L"▼", 872, 490, 18, CR(150, 140, 200), 2);
    drawText(L"ПРОБЕЛ", 78, 496, 13, CR(96, 90, 130));
}

// ------------------------------------------------------------
//  Титульный экран
// ------------------------------------------------------------
struct SuitFall { float x, y, vy; wchar_t ch; COLORREF col; int size; };
static std::vector<SuitFall> gFall;
static void initTitle() {
    gFall.clear();
    const wchar_t* suits = L"♠♥♦♣";
    for (int i = 0; i < 16; i++) {
        SuitFall f;
        f.x = (float)rnd(20, W - 20);
        f.y = (float)rnd(-H, H);
        f.vy = (float)rnd(22, 70);
        f.ch = suits[i % 4];
        f.col = (i % 4 == 1 || i % 4 == 2) ? CR(120, 40, 62) : CR(48, 52, 78);
        f.size = rndi(18, 40);
        gFall.push_back(f);
    }
}
static void updateTitle(double dt) {
    for (auto& f : gFall) {
        f.y += f.vy * (float)dt;
        if (f.y > H + 40) { f.y = -40; f.x = (float)rnd(20, W - 20); }
    }
    updateSnow(dt);
}
static void drawTitle() {
    bgCity();
    fillA(0, 0, W, H, CR(5, 4, 12), 110);
    for (auto& f : gFall) drawText(std::wstring(1, f.ch), (int)f.x, (int)f.y, f.size, f.col, 1);

    drawText(L"ТИМУР", 484, 124, 96, CR(70, 46, 10), 1, true, L"Arial Black");
    drawText(L"ТИМУР", 480, 120, 96, CR(255, 198, 61), 1, true, L"Arial Black");
    drawText(L"и АрТеМ-DUBAI", 480, 216, 44, CR(255, 77, 141), 1, true, L"Arial Black");

    fill(300, 274, 360, 3, CR(70, 60, 110));
    drawText(L"история одного проигрыша в карты", 480, 288, 21, CR(196, 188, 220), 1);
    drawText(L"БАРНАУЛ  ·  ШКОЛА №125  ·  −30 °C", 480, 322, 15, CR(120, 112, 156), 1);

    if ((int)(gTime * 2) % 2 == 0)
        drawText(L"НАЖМИ ПРОБЕЛ", 480, 418, 26, CR(255, 236, 190), 1, true);
    drawText(L"WASD — ходить · мышь — выбирать · пробел — дальше", 480, 468, 14, CR(96, 90, 130), 1);
    drawText(L"2D драма в трёх актах", 480, 492, 13, CR(72, 68, 100), 1);
}

// ------------------------------------------------------------
//  Прогулка по Барнаулу
// ------------------------------------------------------------
static struct { float x = 480, y = 486; bool faceLeft = false; bool moving = false; double phase = 0; } gPlayer;
static int gWalkMode = 0;   // 0 — двор школы, 1 — коридор, 2 — магазин

static void updateWalk(double dt) {
    float dx = 0, dy = 0;
    if (gKeys['A'] || gKeys[VK_LEFT])  dx -= 1;
    if (gKeys['D'] || gKeys[VK_RIGHT]) dx += 1;
    if (gKeys['W'] || gKeys[VK_UP])    dy -= 1;
    if (gKeys['S'] || gKeys[VK_DOWN])  dy += 1;
    if (dx != 0 || dy != 0) {
        float l = sqrtf(dx * dx + dy * dy);
        dx /= l; dy /= l;
        gPlayer.x += dx * 230.0f * (float)dt;
        gPlayer.y += dy * 230.0f * (float)dt;
        if (dx != 0) gPlayer.faceLeft = dx < 0;
        gPlayer.moving = true;
        gPlayer.phase += dt * 11;
    } else gPlayer.moving = false;

    gPlayer.x = (float)clampI((int)gPlayer.x, 56, W - 56);
    gPlayer.y = (float)clampI((int)gPlayer.y, 372, 512);

    updateSnow(dt);
    if (gPlayer.y < 388 && fabs(gPlayer.x - 480) < 95) {
        if (gWalkMode == 0) meetStart();
        else if (gWalkMode == 1) startQueue();
        else startShopScene();
    }
}
static void drawWalk() {
    if (gWalkMode == 0) bgSchool();
    else if (gWalkMode == 1) bgCorridor();
    else bgShopStreet();

    // маркер у входа
    int by = 300 + (int)(sin(gTime * 3) * 6);
    star(480, by, 14, 5, 4, CR(120, 255, 140));
    star(480, by, 9, 3, 4, CR(230, 255, 230));
    if (gWalkMode == 0) drawText(L"ШКОЛА №125", 480, 262, 17, CR(210, 255, 210), 1, true);
    else if (gWalkMode == 2) drawText(L"МАГАЗИН", 480, 262, 17, CR(210, 255, 210), 1, true);

    drawChar((int)gPlayer.x, (int)gPlayer.y, 0.34f, CFG_TIMUR, gPlayer.faceLeft, 1.0f, gTime, gPlayer.moving);

    fillRound(16, 14, 340, 56, 12, CR(10, 12, 22));
    drawText(L"ТИМУР", 32, 22, 20, CR(79, 210, 255), 0, true);
    drawText(gWalkMode == 0 ? L"новенький · карточный гений"
           : gWalkMode == 1 ? L"глава 2 · идёт обедать"
                            : L"глава 2 · идёт за семечками", 32, 46, 13, CR(150, 146, 180));

    fillRound(16, 470, 500, 54, 12, CR(10, 12, 22));
    drawText(gWalkMode == 0 ? L"ЗАДАЧА: дойти до входа в школу №125"
           : gWalkMode == 1 ? L"ЗАДАЧА: дойти до столовой"
                            : L"ЗАДАЧА: дойти до магазина", 32, 480, 16, CR(255, 236, 190));
    drawText(L"WASD / стрелки", 32, 502, 13, CR(140, 134, 172));
}
// ------------------------------------------------------------
//  Тексты катсцен
// ------------------------------------------------------------
static std::vector<Line> CS_INTRO() {
    return {
        { L"narr",  L"Барнаул. Минус тридцать. Город, где даже ветер играет на деньги.", L"timur", L"street" },
        { L"timur", L"Я — Тимур. Я никогда не проигрывал в карты. Никогда. Это не хвастовство, это медицинский факт.", L"timur", L"street" },
        { L"mom",   L"ТИМУР! ХВАТИТ ИГРАТЬ В ШКОЛЕ! ТЕБЕ ПОРА УЧИТЬСЯ В ШКОЛЕ НОМЕР 125!", L"timur", L"street" },
        { L"timur", L"Родители заставили меня учиться. В школе. Номер 125. Что ж... найдём эту школу.", L"timur", L"street" },
    };
}
static std::vector<Line> CS_MEET() {
    return {
        { L"narr",  L"На крыльце школы номер 125 стоял пацан. Он качал головой. Он слушал музыку. Он что-то рисовал на парте.", L"timur,artem", L"school" },
        { L"artem", L"Йо. Ты новенький? Я АрТеМ. С большой Т. И с приставкой DUBAI.", L"timur,artem", L"school" },
        { L"timur", L"Ты... рисуешь письки на парте?", L"timur,artem", L"school" },
        { L"artem", L"Это не письки. Это авангард. И вообще — мы в одном классе, вообще-то.", L"timur,artem", L"school" },
        { L"timur", L"Слушай, АрТеМ. А сыграем в карты? На деньги?", L"timur,artem", L"school" },
        { L"artem", L"Денег у меня нет. Семья нищая. Но у мамы есть золотая свинья-копилка...", L"timur,artem", L"school" },
        { L"narr",  L"АрТеМ отрезал от свиньи кусочек золота. Мама об этом не узнает. Наверное.", L"timur,artem", L"school" },
        { L"artem", L"Триста рублей. Ставлю всё. Погнали в туалет, там никто не спалит.", L"timur,artem", L"school" },
    };
}
static std::vector<Line> CS_RAGE() {
    return {
        { L"timur", L"Стоп. Ты... ты жульничал. У тебя в глазу — программа?!", L"timur,artem", L"toilet" },
        { L"artem", L"Это не жульничество. Это апгрейд. Папа ГеНкА поставил. Спасибо, папа.", L"timur,artem", L"toilet" },
        { L"timur", L"Я никогда не проигрывал. НИКОГДА.", L"timur,artem", L"toilet" },
        { L"narr",  L"Что-то щёлкнуло. Не в глазу у АрТеМа. В голове у Тимура.", L"timur,artem", L"toilet" },
    };
}
static std::vector<Line> CS_LEHA() {
    return {
        { L"narr",  L"Дверь школьного туалета с грохотом открылась.", L"timur", L"toilet" },
        { L"leha",  L"Э. Э. Э. СТОПЭ. Кто тут бьёт моего АрТеМа-DUBAI'я?", L"timur,leha", L"toilet" },
        { L"artem", L"Лёха... он мне трицепс... мой ахуенный трицепс...", L"artem,leha", L"toilet" },
        { L"leha",  L"Слышь, новенький. Ещё раз тронешь АрТеМа-DUBAI'я — будешь иметь дело не только со мной, но и с моим повелителем — Гришка золотой глаз.", L"timur,leha", L"toilet" },
        { L"timur", L"...", L"timur,leha", L"toilet" },
        { L"narr",  L"Тимур молчал. Впервые в жизни он проиграл. В карты. В драке. И по жизни.", L"timur", L"toilet" },
    };
}

// ------------------------------------------------------------
//  Карточная партия
// ------------------------------------------------------------
static std::vector<Card> gDeck, gHand;
static Card gPlayedP, gPlayedA;
static bool gHasP = false, gHasA = false, gArtemShown = false;
static int  gMoneyT = 1000, gMoneyA = 300;
static int  gPot = 0, gBetT = 0, gBetA = 0;
static int  gRound = 0;
static int  gStage = 0;             // 0 — выбор, 1 — вскрытие, 2 — итог
static double gTimer = 0, gCheatT = 0;
static bool gCheat = false;
static std::wstring gMsg;
static int  gLast = 0;
static int  gHandRects[3][4] = {};
static int  gHandCount = 0;

struct RoundCfg { int ante; bool cheat; bool allin; const wchar_t* intro; };
static RoundCfg gRounds[4] = {
    { 100, false, false, L"АрТеМ ставит кусочек золотой свиньи. На столе 100 ₽." },
    { 100, false, false, L"Тимур даже не смотрит в карты. Ему скучно." },
    { 100, true,  false, L"АрТеМ проигрывает. АрТеМ нервничает. АрТеМ лезет в глаз..." },
    { -1,  true,  true,  L"АрТеМ: «ВА-БАНК. Всё, что есть. Или ты, или я»." },
};

static std::wstring rankName(int v) {
    if (v >= 2 && v <= 10) return std::to_wstring(v);
    if (v == 11) return L"J";
    if (v == 12) return L"Q";
    if (v == 13) return L"K";
    if (v == 14) return L"A";
    return L"15";
}
static wchar_t suitChar(int s) { return (s == 0) ? L'♠' : (s == 1) ? L'♥' : (s == 2) ? L'♦' : L'♣'; }
static bool suitRed(int s) { return s == 1 || s == 2; }

static void buildDeck() {
    gDeck.clear();
    for (int s = 0; s < 4; s++)
        for (int v = 6; v <= 14; v++) gDeck.push_back({ v, s, false });
    std::shuffle(gDeck.begin(), gDeck.end(), rng);
}
static Card takeCard() {
    if (gDeck.empty()) buildDeck();
    Card c = gDeck.back();
    gDeck.pop_back();
    return c;
}
static Card losingCard(const Card& p) {
    int v = p.v - rndi(1, 4);
    if (v < 2) v = 2;
    return { v, rndi(0, 3), false };
}
static Card cheatCard(const Card& p) {
    if (p.v >= 14) return { 15, 0, true };
    return { p.v + 1, rndi(0, 3), false };
}

static void beginRound() {
    RoundCfg& cfg = gRounds[gRound];
    while ((int)gHand.size() < 3) gHand.push_back(takeCard());
    if (cfg.allin) { gBetT = gMoneyT; gBetA = gMoneyA; }
    else { gBetT = std::min(cfg.ante, gMoneyT); gBetA = std::min(cfg.ante, gMoneyA); }
    gMoneyT -= gBetT;
    gMoneyA -= gBetA;
    gPot = gBetT + gBetA;
    gHasP = gHasA = gArtemShown = false;
    gStage = 0;
    gTimer = 0;
    gLast = 0;
    gMsg = cfg.intro;
    sfx(660, 0.05, 1, 0.08);
    sfx(880, 0.06, 1, 0.06);
}

static void resolveRound() {
    int pv = gPlayedP.v;
    int av = gPlayedA.joker ? 15 : gPlayedA.v;
    bool win = pv > av;
    gLast = win ? 1 : -1;
    if (win) { gMoneyT += gPot; gMsg = L"ТЫ ЗАБИРАЕШЬ БАНК"; }
    else {
        gMoneyA += gPot;
        gMsg = gCheat ? L"АрТеМ забирает всё. В глазу мигнуло: «СПАСИБО, ПАПА»" : L"АрТеМ забирает банк";
    }
    gStage = 2;
    gTimer = win ? 1.5 : 1.9;
    if (win) { sfx(660, 0.12, 2, 0.16, 240); sfx(990, 0.12, 2, 0.10, 120); }
    else { sfx(220, 0.22, 3, 0.16, -120); }
}

static void pickCard(int i) {
    if (gStage != 0 || i < 0 || i >= (int)gHand.size()) return;
    Card c = gHand[i];
    gHand.erase(gHand.begin() + i);
    gPlayedP = c; gHasP = true;
    RoundCfg& cfg = gRounds[gRound];
    gPlayedA = cfg.cheat ? cheatCard(c) : losingCard(c);
    gHasA = true;
    if (cfg.cheat && !gCheat) {
        gCheat = true;
        gCheatT = 0;
        gMsg = L"«ГЕНКА-ОС» активирован. АрТеМ видит твои карты.";
        sfx(90, 0.22, 3, 0.20, 500);
        sfx(1400, 0.05, 0, 0.10, -900);
    }
    gStage = 1;
    gTimer = 1.25;
    gArtemShown = false;
    sfx(520, 0.05, 2, 0.10, 160);
}

static void updateCards(double dt) {
    if (gCheat) gCheatT += dt;
    if (gStage == 1) {
        gTimer -= dt;
        if (gTimer <= 0.60) gArtemShown = true;
        if (gTimer <= 0) resolveRound();
    } else if (gStage == 2) {
        gTimer -= dt;
        if (gTimer <= 0) {
            gRound++;
            if (gRound >= 4) playCS(CS_RAGE(), L"toilet", []() { startFight(); });
            else beginRound();
        }
    }
}

static void drawProfileCard(int x, int y, int w, int h, const CharCfg& cfg, const std::wstring& name, int money) {
    fillRound(x, y, w, h, 14, CR(9, 12, 22));
    fillRound(x + 2, y + 2, w - 4, h - 4, 12, CR(15, 21, 36));
    drawAvatar(x + 50, y + 52, 38, cfg, gTime);
    drawText(name, x + 98, y + 24, 20, CR(240, 236, 250), 0, true);
    drawText(std::to_wstring(money) + L" ₽", x + 98, y + 54, 23, CR(255, 198, 61), 0, true);
}

static void drawEyeHud() {
    int x = 16, y = 126, w = 272, h = 52;
    fillRound(x, y, w, h, 12, gCheat ? CR(46, 10, 16) : CR(12, 16, 26));
    if (gCheat) {
        drawText(L"ГЕНКА-ОС v0.1", x + 14, y + 7, 16, CR(255, 96, 96), 0, true);
        drawText(L"ЧИТ: АКТИВЕН", x + 14, y + 27, 14, CR(255, 176, 176));
        for (int i = 0; i < 4; i++) {
            int gy = y + 6 + ((int)(gCheatT * 70) + i * 13) % 40;
            fill(x + 150, gy, 108, 3, CR(255, 60, 80));
        }
    } else {
        drawText(L"ГЕНКА-ОС v0.1", x + 14, y + 7, 16, CR(90, 96, 120), 0, true);
        drawText(L"статус: спит", x + 14, y + 27, 14, CR(80, 86, 110));
    }
}

static void drawCardShape(int x, int y, int w, int h, const Card& c, bool faceDown, bool dim, bool glitch) {
    fillRound(x + 4, y + 6, w, h, 10, CR(0, 0, 0));
    if (faceDown) {
        fillRound(x, y, w, h, 10, CR(22, 28, 52));
        fillRound(x + 6, y + 6, w - 12, h - 12, 7, CR(34, 44, 84));
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 7; j++)
                if (((i + j) & 1) == 0)
                    fill(x + 12 + i * ((w - 24) / 5), y + 12 + j * ((h - 24) / 7), 6, 6, CR(48, 62, 112));
        drawText(L"?", x + w / 2, y + h / 2 - 24, 34, CR(78, 96, 156), 1, true);
        return;
    }
    if (c.joker) {
        fillRound(x, y, w, h, 10, CR(58, 12, 26));
        fillRound(x + 4, y + 4, w - 8, h - 8, 8, CR(28, 8, 18));
        drawText(L"15", x + w / 2, y + h / 2 - 44, (int)(w * 0.56), CR(255, 88, 96), 1, true, L"Arial Black");
        drawText(L"★", x + w / 2, y + h / 2 + 6, (int)(w * 0.42), CR(255, 196, 80), 1, true);
        drawText(L"ЧИТ", x + w / 2, y + 10, (int)(w * 0.17), CR(255, 130, 130), 1, true);
        return;
    }
    COLORREF face = dim ? CR(196, 194, 186) : CR(246, 243, 232);
    fillRound(x, y, w, h, 10, face);
    fillRound(x + 3, y + 3, w - 6, h - 6, 8, dim ? CR(188, 186, 178) : CR(252, 250, 241));
    COLORREF col = suitRed(c.s) ? CR(196, 44, 64) : CR(30, 30, 42);
    std::wstring r = rankName(c.v);
    std::wstring su(1, suitChar(c.s));
    drawText(r, x + 9, y + 3, (int)(w * 0.26), col, 0, true);
    drawText(su, x + 9, y + (int)(h * 0.23), (int)(w * 0.23), col, 0);
    drawText(su, x + w / 2, y + h / 2 - (int)(h * 0.23), (int)(w * 0.50), col, 1);
    drawText(su, x + w - 9 - textW(su, (int)(w * 0.20)), y + h - (int)(h * 0.22), (int)(w * 0.20), col, 0);
}

static void drawCards() {
    bgToilet();
    fillA(0, 0, W, H, CR(4, 10, 8), 130);
    fillRound(40, 132, 880, 420, 24, CR(12, 36, 27));
    fillRound(48, 138, 864, 408, 20, CR(18, 52, 39));
    fillRound(48, 138, 864, 10, 8, CR(26, 70, 52));

    drawProfileCard(16, 14, 272, 104, CFG_ARTEM, L"АрТеМ-DUBAI", gMoneyA);
    drawProfileCard(672, 14, 272, 104, CFG_TIMUR, L"ТИМУР", gMoneyT);
    drawEyeHud();

    drawText(L"РАУНД " + std::to_wstring(gRound + 1) + L" / 4", 480, 22, 15, CR(140, 190, 160), 1, true);
    drawText(std::to_wstring(gPot) + L" ₽", 480, 48, 28, CR(255, 210, 90), 1, true);
    for (int i = 0; i < 3; i++)
        fillEllipse(480 - 20 + i * 20, 88 - i * 3, 11, 6, i == 0 ? CR(255, 216, 74) : CR(226, 184, 58));
    drawText(L"БАНК", 480, 92, 12, CR(160, 150, 100), 1);

    if (gHasP) drawCardShape(354, 150, 106, 148, gPlayedP, false, false, false);
    else fillRound(354, 150, 106, 148, 10, CR(0, 0, 0));
    if (gHasA) {
        if (!gArtemShown && gStage == 1) drawCardShape(500, 150, 106, 148, gPlayedA, true, false, false);
        else drawCardShape(500, 150, 106, 148, gPlayedA, false, !gArtemShown, false);
    } else fillRound(500, 150, 106, 148, 10, CR(0, 0, 0));

    drawText(L"ТВОЯ КАРТА", 407, 303, 12, CR(140, 190, 160), 1);
    drawText(L"КАРТА АрТеМа", 553, 303, 12, CR(140, 190, 160), 1);

    {
        COLORREF mc = (gStage == 2) ? (gLast > 0 ? CR(140, 255, 150) : CR(255, 120, 120)) : CR(226, 220, 240);
        int px = (gStage == 2) ? 21 : 16;
        auto rows = wrapText(gMsg, px, 760, false);
        int yy = 320;
        for (auto& s : rows) { drawText(s, 480, yy, px, mc, 1); yy += 26; }
    }

    // рука
    gHandCount = 0;
    int hovered = -1;
    for (int i = 0; i < (int)gHand.size() && i < 3; i++) {
        int w = 106, h = 148, gap = 118;
        int x = 480 + (i - 1) * gap - w / 2;
        int y = 376;
        bool hov = gStage == 0 && ptIn(x, y - 18, w, h + 18);
        if (hov) hovered = i;
        gHandRects[i][0] = x; gHandRects[i][1] = y - 20; gHandRects[i][2] = w; gHandRects[i][3] = h + 20;
        gHandCount++;
    }
    for (int i = 0; i < (int)gHand.size() && i < 3; i++) {
        if (i == hovered) continue;
        int w = 106, h = 148, gap = 118;
        int x = 480 + (i - 1) * gap - w / 2;
        drawCardShape(x, 376, w, h, gHand[i], false, false, false);
        drawText(std::to_wstring(i + 1), x + 8, 376 + h - 20, 13, CR(120, 120, 140));
    }
    if (hovered >= 0) {
        int w = 106, h = 148, gap = 118;
        int x = 480 + (hovered - 1) * gap - w / 2;
        drawCardShape(x, 376 - 18, w, h, gHand[hovered], false, false, false);
        drawText(std::to_wstring(hovered + 1), x + 8, 376 - 18 + h - 20, 13, CR(120, 120, 140));
    }

    if (gStage == 0)
        drawText(L"выбери карту: клик мышью или 1 / 2 / 3", 480, 527, 12, CR(120, 150, 130), 1);

    if (gCheat && gCheatT < 2.4) {
        int a = (int)(70 * (1.0 - gCheatT / 2.4));
        fillA(0, 0, W, H, CR(90, 0, 14), a);
        if (gCheatT < 1.6) {
            drawText(L"ГЕНКА-ОС", 480, 200, 54, CR(255, 90, 90), 1, true, L"Arial Black");
            drawText(L"ЗАПУЩЕН", 480, 256, 30, CR(255, 170, 170), 1, true);
        }
    }
}

static void startCards() {
    gMoneyT = 1000; gMoneyA = 300;
    gCheat = false; gCheatT = 0; gRound = 0; gPot = 0;
    gHand.clear(); buildDeck();
    gMsg.clear();
    gScene = S_CARDS;
    beginRound();
}
// ------------------------------------------------------------
//  Драка в туалете
// ------------------------------------------------------------
static float fHp = 100, fPunch = 0, fShake = 0;
static double fDoneT = -1;
static int fHits = 0;
static float fIntro = 0;

struct Fx { float x, y, vy; double t; std::wstring s; COLORREF c; };
static std::vector<Fx> gFx;

static void startFight() {
    fHp = 100; fPunch = 0; fShake = 0; fDoneT = -1; fHits = 0; fIntro = 2.2f;
    gFx.clear();
    gScene = S_FIGHT;
    sfx(160, 0.18, 3, 0.20, -60);
}

static void fightHit() {
    if (fDoneT >= 0) return;
    fPunch = 0.22f;
    fShake = 0.30f;
    fHp -= (float)rnd(8, 13);
    fHits++;
    const wchar_t* words[] = { L"БАМ!", L"ХРЯСЬ!", L"ШМЯК!", L"ТРЕСЬ!", L"БУМ!", L"ХЛОБЫСЬ!" };
    Fx f;
    f.x = 600 + (float)rnd(-26, 26);
    f.y = 372 + (float)rnd(-22, 22);
    f.vy = (float)rnd(-70, -40);
    f.t = 0;
    f.s = words[rndi(0, 5)];
    f.c = (rndi(0, 2) == 0) ? CR(255, 230, 110) : CR(255, 150, 120);
    gFx.push_back(f);
    sfx(rnd(90, 150), 0.09, 0, 0.22, -70);
    if (fHp <= 0) {
        fHp = 0;
        fDoneT = 0;
        fShake = 0.55f;
        sfx(110, 0.30, 3, 0.25, -80);
    }
}

static void updateFight(double dt) {
    if (fPunch > 0) { fPunch -= (float)dt; if (fPunch < 0) fPunch = 0; }
    if (fShake > 0) { fShake -= (float)dt * 2.0f; if (fShake < 0) fShake = 0; }
    if (fIntro > 0) fIntro -= (float)dt;
    for (auto& f : gFx) { f.t += dt; f.y += f.vy * (float)dt; }
    for (int i = (int)gFx.size() - 1; i >= 0; i--)
        if (gFx[i].t > 1.0) gFx.erase(gFx.begin() + i);

    if (fDoneT >= 0) {
        fDoneT += dt;
        if (fDoneT > 1.7) {
            fDoneT = -1;
            playCS(CS_LEHA(), L"toilet", []() { gScene = S_END; });
        }
    }
}

static void drawFight() {
    bgToilet();
    fillA(0, 0, W, H, CR(10, 22, 16), 90);

    int sx = 0, sy = 0;
    if (fShake > 0) {
        sx = (int)(rnd(-1, 1) * fShake * 34);
        sy = (int)(rnd(-1, 1) * fShake * 34);
    }

    CharCfg artem = CFG_ARTEM;
    artem.eye = 3;
    int lean = 0;
    if (fPunch > 0) lean = (int)(sin((0.22 - fPunch) / 0.22 * PI) * 12);
    if (fHp <= 0) lean = 20;

    drawChar(320 + sx, 486 + sy, 1.02f, CFG_TIMUR, false, 1.0f, gTime, true);
    drawChar(690 + sx + lean, 486 + sy, 1.02f, artem, true, 1.0f, gTime, fHp > 0);

    // рука Тимура
    if (fPunch > 0) {
        float p = 1.0f - fPunch / 0.22f;
        int ax = 356 + sx, ay = 372 + sy;
        int bx = ax + (int)(238 * p), by = 336 + sy;
        line(ax, ay, bx, by, CR(46, 111, 142), 22);
        line(ax, ay, bx, by, CR(78, 157, 187), 10);
        fillEllipse(bx, by, 17, 17, CR(242, 200, 162));
        int r = (int)(46 * sin(p * PI)) + 16;
        star(612 + sx, 368 + sy, r, r / 3, 5, CR(255, 214, 80));
        star(612 + sx, 368 + sy, r * 2 / 3, r / 4, 5, CR(255, 255, 220));
    }

    for (auto& f : gFx) {
        int a = (int)(255 * (1.0 - f.t));
        std::wstring s = f.s;
        drawText(s, (int)f.x, (int)f.y, 30, mul(f.c, 0.4f + 0.6f * (float)(1.0 - f.t)), 1, true, L"Arial Black");
        (void)a;
    }

    // полоса трицепса
    fillRound(280, 44, 400, 24, 12, CR(28, 16, 20));
    int bw = (int)(400 * (fHp / 100.0f));
    fillRound(280, 44, bw, 24, 12, mix(CR(255, 60, 60), CR(255, 204, 70), fHp / 100.0f));
    drawText(L"ТРИЦЕПС АрТеМа", 480, 14, 18, CR(244, 236, 248), 1, true);
    drawText(std::to_wstring((int)fHp) + L"%", 480, 46, 15, CR(20, 10, 14), 1, true);

    if (fHp > 0) {
        drawText(L"КЛИКАЙ МЫШЬЮ или ЖМИ ПРОБЕЛ — БЕЙ ПО ТРИЦЕПСУ", 480, 496, 16,
                 (int)(gTime * 3) % 2 ? CR(255, 236, 190) : CR(220, 200, 160), 1, true);
        drawText(L"ШКОЛЬНЫЙ ТУАЛЕТ · РАЗБОРКА", 480, 520, 12, CR(130, 150, 130), 1);
    } else {
        drawText(L"ТРИЦЕПС ПОВЕРЖЕН", 480, 140, 40, CR(255, 210, 90), 1, true, L"Arial Black");
        drawText(L"АрТеМ: «МОЙ АХУЕННЫЙ ТРИЦЕПС...»", 480, 196, 18, CR(240, 200, 200), 1);
    }

    if (fIntro > 0 && (int)(fIntro * 4) % 2 == 0) {
        drawText(L"ДРАКА", 480, 118, 60, CR(255, 70, 70), 1, true, L"Arial Black");
    }
}

// ------------------------------------------------------------
//  Финал
// ------------------------------------------------------------
static void drawEnd() {
    bgCity();
    fillA(0, 0, W, H, CR(4, 3, 10), 150);

    CharCfg grisha = CFG_GRISHA;
    grisha.eye = 2;
    drawChar(824, 508, 0.92f, grisha, true, 0.9f, gTime, true);

    drawText(L"КОНЕЦ", 480, 92, 92, CR(255, 198, 61), 1, true, L"Arial Black");
    drawText(L"ПЕРВОЕ ПОРАЖЕНИЕ ТИМУРА", 480, 202, 26, CR(240, 236, 250), 1, true);
    drawText(L"АрТеМ-DUBAI и Платон-Лёха ушли в закат. С кусочком маминой золотой свиньи.", 480, 248, 17, CR(176, 170, 206), 1);
    drawText(L"Гришка золотой глаз наблюдает.", 480, 278, 18, CR(255, 122, 170), 1);
    drawText(L"Тимур впервые проиграл. Но он ещё вернётся.", 480, 316, 16, CR(140, 136, 170), 1);
    drawText(L"ПРОДОЛЖЕНИЕ СЛЕДУЕТ", 480, 352, 17, CR(255, 198, 61), 1, true);

    if ((int)(gTime * 2) % 2 == 0)
        drawText(L"ENTER — ГЛАВА 2: РЕНАТ-ОС", 480, 408, 24, CR(255, 236, 190), 1, true);
    drawText(L"R — сыграть заново", 480, 446, 15, CR(150, 146, 180), 1);
    drawText(L"ТИМУР · АрТеМ-DUBAI · ПЛАТОН-ЛЁХА · ПАПА ГЕNКА · ГРИШКА ЗОЛОТОЙ ГЛАЗ", 480, 508, 12, CR(94, 90, 126), 1);
}

// ------------------------------------------------------------
//  Ввод и переходы
// ------------------------------------------------------------
// ------------------------------------------------------------
//  Тексты главы 2
// ------------------------------------------------------------
static std::vector<Line> CS_P2_TOILET() {
    return {
        { L"narr",  L"АрТеМ-DUBAI и Платон-Лёха ушли. Тимур остался один в школьном туалете.", L"timur", L"toilet" },
        { L"timur", L"Я проиграл не из-за скилла. Я проиграл из-за читов. Это, блять, разные вещи.", L"timur", L"toilet" },
        { L"timur", L"Так. Расписание. Столовая. Надо поесть и успокоиться.", L"timur", L"toilet" },
        { L"narr",  L"Тимур вышел из туалета, посмотрел на расписание и пошёл в столовую.", L"timur", L"toilet" },
    };
}
static std::vector<Line> CS_TABLE() {
    return {
        { L"narr",  L"Через пару минут они уже сидели за одним столом.", L"timur,dima", L"canteen" },
        { L"narr",  L"Парня звали ДиМа-justkillme-psihokid-КаРиНа-СоНя-ВиКа-zxcpipin31. Но все звали его просто ДиМа.", L"timur,dima", L"canteen" },
        { L"timur", L"Короче. Я проиграл не по скиллу. У него в глазу читы. ГЕНКА-ОС, папина разработка.", L"timur,dima", L"canteen" },
        { L"dima",  L"Понятно. А вот у меня... АХФЦВХ. А Я ВОТ УЖЕ... ДВЕ НЕДЕЛИ... БЕЗ СЕКСА.", L"timur,dima", L"canteen" },
        { L"narr",  L"Все резко замолчали. В столовой стало слышно, как остывает котлета.", L"timur,dima", L"canteen" },
        { L"timur", L"Пошёл нахуй, ДиМа.", L"timur,dima", L"canteen" },
        { L"narr",  L"Тимур был оскорблён такими высказываниями и в спешке ушёл, крича это уже в след.", L"timur,dima", L"canteen" },
    };
}
static std::vector<Line> CS_302() {
    return {
        { L"narr",   L"Кабинет 302. Тимур вошёл и увидел его: ГрИшКа-ЗоЛоТоЙ-гЛаЗ.", L"timur,grisha", L"class302" },
        { L"grisha", L"...", L"timur,grisha", L"class302" },
        { L"narr",   L"Гришка сидел неподвижно и смотрел в одну точку. Можно было подумать, что он конченный.", L"timur,grisha", L"class302" },
        { L"grisha", L"Я вижу тебя насквозь, Тимур. И твой проигрыш. И твою будущую победу.", L"timur,grisha", L"class302" },
        { L"timur",  L"Э... спасибо?", L"timur,grisha", L"class302" },
        { L"narr",   L"Но в конце концов Тимур отучился и пошёл домой.", L"timur,grisha", L"class302" },
    };
}
static std::vector<Line> CS_HOME() {
    return {
        { L"narr",  L"Дома его никто не встретил. В этой семье так принято.", L"timur,renat", L"home" },
        { L"narr",  L"На столе — молоко с хурмой. На сковородке — огурцы. Всё как у всех.", L"timur,renat", L"home" },
        { L"timur", L"Пап. Я опозорил нашу семью.", L"timur,renat", L"home" },
        { L"renat", L"И?", L"timur,renat", L"home" },
        { L"timur", L"Чтобы впредь такого не повторялось, я хочу себе глаз бога под названием «РЕНАТ-ОС».", L"timur,renat", L"home" },
        { L"renat", L"Сходи за семечками. А я сделаю.", L"timur,renat", L"home" },
    };
}
static std::vector<Line> CS_SHOP() {
    return {
        { L"narr",  L"У магазина Тимур встретил ДиМу.", L"timur,dima", L"shop" },
        { L"dima",  L"О. Это ты. Тоже за семечками?", L"timur,dima", L"shop" },
        { L"timur", L"Пошёл нахуй, ДиМа.", L"timur,dima", L"shop" },
        { L"narr",  L"Тимур купил семечки и пошёл домой. ДиМа остался стоять у магазина и выглядел как-то загадочно.", L"timur,dima", L"shop" },
    };
}
static std::vector<Line> CS_NIGHT() {
    return {
        { L"narr",  L"Ночью Ренат вставил сыну глаз бога под названием РЕНАТ-ОС.", L"timur,renat", L"home" },
        { L"renat", L"Готово. Теперь ты видишь всё. Даже то, что видеть не должен.", L"timur,renat", L"home" },
        { L"timur", L"Спасибо, отец.", L"timur,renat", L"home" },
        { L"narr",  L"На следующий день Тимур пришёл в школу уже с РЕНАТ-ОС и сразу вызвал АрТеМа-DUBAI'я на реванш.", L"timur,renat", L"home" },
    };
}
static std::vector<Line> CS_P2_WIN() {
    return {
        { L"artem", L"Это невозможно! У меня же ГЕНКА-ОС!", L"timur,artem", L"canteen" },
        { L"timur", L"А у меня РЕНАТ-ОС. Спасибо, папа.", L"timur,artem", L"canteen" },
        { L"narr",  L"Тимур выебал АрТеМа-DUBAI'я. В карты, конечно.", L"timur,artem", L"canteen" },
        { L"artem", L"Я расскажу Лёхе!", L"timur,artem", L"canteen" },
        { L"timur", L"Пошёл нахуй.", L"timur,artem", L"canteen" },
    };
}
// ============================================================
//  ГЛАВА 2
// ============================================================

// ------------------------------------------------------------
//  Фоны главы 2
// ------------------------------------------------------------
static void bgCorridor() {
    fill(0, 0, W, 404, CR(74, 82, 96));
    for (int i = 0; i < 9; i++) {
        int x = -40 + i * 118;
        fill(x, 60, 104, 344, CR(86, 94, 110));
        fill(x + 6, 70, 92, 324, CR(96, 104, 122));
    }
    fill(0, 0, W, 60, CR(58, 64, 76));
    for (int i = 0; i < 5; i++) {
        fill(110 + i * 190, 34, 80, 9, CR(206, 214, 224));
        fillA(90 + i * 190, 40, 120, 220, CR(255, 244, 206), 12);
    }
    fill(0, 404, W, H - 404, CR(62, 60, 68));
    for (int y = 404; y < H; y += 34) fill(0, y, W, 2, CR(48, 46, 54));
    for (int x = -400; x < W + 400; x += 100)
        line(x, 404, (int)(480 + (x - 480) * 2.7f), H, CR(52, 50, 58), 2);

    // расписание
    fillRound(96, 116, 214, 172, 8, CR(228, 224, 208));
    drawText(L"РАСПИСАНИЕ", 203, 124, 17, CR(60, 52, 44), 1, true);
    drawText(L"1. МАТЕМАТИКА", 112, 156, 14, CR(70, 62, 54));
    drawText(L"2. РУССКИЙ", 112, 180, 14, CR(70, 62, 54));
    fillRound(106, 202, 194, 26, 5, CR(160, 40, 50));
    drawText(L"3. ОБЕД В СТОЛОВОЙ", 203, 207, 13, CR(255, 236, 220), 1);
    drawText(L"4. ГЕОМЕТРИЯ", 112, 240, 14, CR(70, 62, 54));
    drawText(L"ПЕРЕМЕНА 20 МИН", 112, 266, 11, CR(120, 108, 92));

    // дверь столовой
    fill(418, 140, 144, 264, CR(96, 72, 50));
    fill(434, 156, 112, 248, CR(140, 110, 74));
    fill(486, 156, 8, 248, CR(96, 72, 50));
    fillEllipse(522, 282, 5, 5, CR(60, 46, 34));
    fill(408, 134, 164, 8, CR(120, 92, 62));
    fillRound(358, 94, 244, 38, 8, CR(150, 40, 50));
    drawText(L"СТОЛОВАЯ", 480, 101, 24, CR(255, 236, 220), 1, true, L"Arial Black");

    // шкафчики
    for (int i = 0; i < 5; i++) {
        fill(700 + i * 46, 212, 40, 160, CR(70, 96, 110));
        fill(704 + i * 46, 218, 32, 148, CR(84, 112, 126));
        fillEllipse(720 + i * 46, 292, 4, 4, CR(50, 66, 76));
    }
    // бачок
    fillRound(60, 300, 50, 70, 6, CR(90, 110, 100));
}

static void bgCanteen() {
    for (int y = 0; y < 300; y += 40) fill(0, y, W, 40, (y / 40) % 2 ? CR(196, 172, 140) : CR(188, 164, 132));
    fill(0, 300, W, 110, CR(150, 128, 102));
    fill(0, 300, W, 8, CR(120, 100, 80));
    for (int i = 0; i < 4; i++) {
        fill(60 + i * 230, 70, 170, 126, CR(140, 176, 196));
        fill(60 + i * 230, 128, 170, 7, CR(120, 110, 96));
        fill(141 + i * 230, 70, 8, 126, CR(120, 110, 96));
        fill(56 + i * 230, 66, 178, 8, CR(110, 96, 84));
    }
    fillRound(648, 62, 264, 112, 8, CR(70, 56, 44));
    drawText(L"МЕНЮ", 780, 68, 17, CR(255, 220, 160), 1, true);
    drawText(L"КОТЛЕТА ..... 45 ₽", 664, 96, 14, CR(230, 210, 180));
    drawText(L"ПЮРЕ .......... 30 ₽", 664, 118, 14, CR(230, 210, 180));
    drawText(L"КОМПОТ ....... 15 ₽", 664, 140, 14, CR(230, 210, 180));

    // раздача
    fill(26, 244, 520, 66, CR(168, 170, 172));
    fill(26, 236, 520, 12, CR(200, 202, 204));
    for (int i = 0; i < 3; i++) fillRound(70 + i * 150, 218, 100, 22, 8, CR(214, 216, 218));
    for (int i = 0; i < 3; i++) fillEllipse(112 + i * 150, 206, 26, 9, CR(150, 150, 152));

    // пол и столы
    fill(0, 400, W, H - 400, CR(122, 100, 80));
    for (int y = 400; y < H; y += 36) fill(0, y, W, 2, CR(102, 84, 66));
    for (int i = 0; i < 3; i++) {
        int tx = 100 + i * 300;
        fillRound(tx, 452, 230, 16, 6, CR(182, 150, 110));
        fill(tx + 20, 468, 10, 66, CR(120, 100, 80));
        fill(tx + 200, 468, 10, 66, CR(120, 100, 80));
    }
    drawText(L"СТОЛОВАЯ №125", 480, 336, 14, CR(110, 92, 74), 1);
}

static void bgShopStreet() {
    for (int y = 0; y < 360; y += 8)
        fill(0, y, W, 8, mix(CR(40, 46, 66), CR(84, 94, 120), (float)y / 360));
    fill(180, 60, 600, 300, CR(88, 70, 66));
    fill(168, 48, 624, 16, CR(58, 46, 44));
    fillRound(300, 78, 360, 54, 8, CR(40, 80, 70));
    drawText(L"ПРОДУКТЫ 24", 480, 87, 30, CR(150, 255, 190), 1, true, L"Arial Black");
    fill(228, 168, 184, 124, CR(140, 176, 190));
    fill(548, 168, 184, 124, CR(140, 176, 190));
    fill(318, 168, 8, 124, CR(120, 104, 88));
    fill(228, 226, 184, 7, CR(120, 104, 88));
    fill(548, 226, 184, 7, CR(120, 104, 88));
    fill(414, 218, 132, 142, CR(70, 90, 100));
    fill(428, 230, 104, 130, CR(170, 200, 210));
    drawText(L"ОТКРЫТО", 480, 288, 12, CR(60, 80, 70), 1);

    fill(0, 360, W, H - 360, CR(208, 214, 226));
    fill(0, 360, W, 10, CR(236, 240, 248));
    for (int i = 0; i < 10; i++) fillEllipse((i * 117) % W, 420 + (i * 53) % 90, 60, 14, CR(226, 232, 242));
    drawSnow();
}

static void bgHome() {
    fill(0, 0, W, 412, CR(150, 132, 112));
    for (int x = 0; x < W; x += 46) fill(x, 0, 2, 412, CR(140, 122, 104));
    for (int y = 40; y < 412; y += 46) fill(0, y, W, 2, CR(142, 124, 106));

    fill(70, 84, 280, 190, CR(60, 76, 96));
    fill(82, 96, 256, 166, CR(102, 138, 164));
    fill(206, 84, 8, 190, CR(120, 104, 88));
    fill(70, 174, 280, 8, CR(120, 104, 88));
    fill(70, 270, 280, 10, CR(126, 108, 90));

    // стол
    fill(300, 330, 600, 26, CR(126, 92, 64));
    fill(318, 356, 18, 140, CR(98, 72, 52));
    fill(864, 356, 18, 140, CR(98, 72, 52));

    // сковородка с огурцами
    fillEllipse(520, 330, 96, 24, CR(58, 58, 62));
    fillEllipse(520, 325, 84, 18, CR(38, 38, 42));
    for (int i = 0; i < 5; i++) {
        fillRound(468 + i * 26, 316 + (i % 2) * 6, 36, 13, 6, CR(96, 156, 60));
        fillRound(474 + i * 26, 318 + (i % 2) * 6, 26, 6, 3, CR(140, 196, 90));
    }
    fill(604, 326, 60, 6, CR(90, 90, 94));

    // молоко с хурмой
    fillRound(648, 284, 48, 48, 4, CR(240, 240, 236));
    fill(648, 298, 48, 10, CR(120, 170, 210));
    drawText(L"МОЛОКО", 672, 316, 9, CR(90, 90, 90), 1);
    fillEllipse(716, 320, 21, 18, CR(240, 150, 40));
    fillEllipse(716, 304, 14, 6, CR(96, 128, 54));

    fill(0, 412, W, H - 412, CR(110, 84, 62));
    for (int y = 412; y < H; y += 30) fill(0, y, W, 2, CR(96, 72, 52));
    fillA(280, 0, 400, 300, CR(255, 230, 170), 12);
}

static void bgClass302() {
    fill(0, 0, W, 412, CR(120, 138, 130));
    fill(0, 412, W, H - 412, CR(96, 74, 56));
    for (int y = 412; y < H; y += 30) fill(0, y, W, 2, CR(80, 62, 46));

    fill(84, 64, 430, 216, CR(236, 232, 218));
    fill(96, 76, 406, 192, CR(46, 78, 62));
    drawText(L"302", 300, 112, 58, CR(222, 240, 222), 1, true, L"Arial Black");
    drawText(L"контрольная по геометрии", 300, 190, 16, CR(190, 220, 190), 1);

    fill(672, 76, 226, 164, CR(150, 186, 206));
    fill(782, 76, 8, 164, CR(120, 104, 88));
    fill(672, 154, 226, 8, CR(120, 104, 88));
    fill(664, 70, 242, 8, CR(110, 96, 82));

    for (int i = 0; i < 3; i++) {
        int x = 96 + i * 250;
        fillRound(x, 398, 180, 16, 4, CR(168, 132, 92));
        fill(x + 16, 414, 10, 74, CR(112, 90, 70));
        fill(x + 154, 414, 10, 74, CR(112, 90, 70));
        fillRound(x + 10, 430, 160, 8, 4, CR(140, 110, 78));
    }
    fill(700, 286, 130, 126, CR(92, 76, 62));
    fill(706, 292, 118, 12, CR(74, 60, 50));
}

// ------------------------------------------------------------
//  Мини-игра: очередь в столовой
// ------------------------------------------------------------
static float qProg = 0;
static int qPhase = 0;
static double qDoneT = 0, qBubbleT = 0;
static std::wstring qBubble;
static int qShout = 0;

static void startQueue() {
    qProg = 0; qPhase = 0; qDoneT = 0; qBubbleT = 0.6f; qBubble.clear(); qShout = 0;
    gScene = S_QUEUE;
    sfx(300, 0.08, 2, 0.10);
}
static void onQueuePush() {
    if (qPhase != 0) return;
    qProg += (float)rnd(7, 12);
    if (qProg >= 100) { qProg = 100; qPhase = 1; qDoneT = 0; sfx(140, 0.22, 3, 0.18, -60); return; }
    sfx(200 + qProg * 5, 0.05, 0, 0.10, 40);
}
static void updateQueue(double dt) {
    if (qPhase == 0) {
        qProg -= (float)dt * 6.0f;
        if (qProg < 0) qProg = 0;
        qBubbleT -= dt;
        if (qBubbleT <= 0) {
            qBubbleT = 1.7;
            qShout++;
            if (qShout % 2 == 1) qBubble = L"САНЕ, ЗАНИМАЛ?";
            else qBubble = L"ДА ЗАНИМАЛ!!!!!!!!!!";
        }
    } else {
        qDoneT += dt;
        if (qDoneT > 1.3) startDima();
    }
}
static void tri(int x1, int y1, int x2, int y2, int x3, int y3, COLORREF c) {
    POINT p[3] = { { x1, y1 }, { x2, y2 }, { x3, y3 } };
    HGDIOBJ ob = SelectObject(gDC, brushFor(c));
    HGDIOBJ op = SelectObject(gDC, GetStockObject(NULL_PEN));
    Polygon(gDC, p, 3);
    SelectObject(gDC, ob); SelectObject(gDC, op);
}
static void drawQueue() {
    bgCanteen();
    fillA(0, 0, W, H, CR(30, 16, 8), 55);

    for (int i = 0; i < 45; i++) {
        int row = i / 9, col = i % 9;
        int x = 40 + col * 108 + (row % 2) * 54;
        int y = 150 + row * 58 + (int)(sin(gTime * 1.8 + i * 1.7) * 5);
        COLORREF shirt = (i % 5 == 0) ? CR(70, 58, 70) : (i % 3 == 0 ? CR(64, 52, 44) : CR(58, 46, 40));
        fillRound(x - 30, y + 20, 60, 92, 16, shirt);
        fillEllipse(x, y, 24, 24, CR(52, 38, 30));
        if (i % 4 == 0) fillEllipse(x - 6, y - 4, 3, 3, CR(20, 14, 10));
        if (i % 4 == 0) fillEllipse(x + 6, y - 4, 3, 3, CR(20, 14, 10));
    }

    int py = (int)(492 - qProg * 1.25);
    drawChar(480, py, 0.58f, CFG_TIMUR, false, 1.0f, gTime, true);

    // прогресс
    fillRound(280, 22, 400, 22, 11, CR(34, 22, 16));
    if (qProg > 1) fillRound(280, 22, (int)(400 * qProg / 100), 22, 11, CR(255, 190, 80));
    drawText(L"ПРОТИСНУТЬСЯ В ОЧЕРЕДИ", 480, 50, 15, CR(255, 232, 186), 1, true);

    if (qPhase == 0 && !qBubble.empty()) {
        int tw = textW(qBubble, 18, true) + 42;
        int bx = 480 - tw / 2;
        fillRound(bx, 86, tw, 44, 14, CR(250, 246, 236));
        tri(480 - 12, 130, 480 + 12, 130, 480, 144, CR(250, 246, 236));
        drawText(qBubble, 480, 98, 18, CR(50, 40, 36), 1, true);
    }

    if (qPhase == 0) {
        bool blink = (int)(gTime * 3) % 2 == 0;
        drawText(L"ЖМИ ПРОБЕЛ ИЛИ КЛИКАЙ — ПРОТИСНИСЬ", 480, 492, 18,
                 blink ? CR(255, 236, 190) : CR(210, 190, 150), 1, true);
        drawText(L"скоро звонок. ждать некогда", 480, 518, 13, CR(160, 140, 116), 1);
    } else {
        drawText(L"ТЫ ПРОТИСНУЛСЯ... И ЗАДЕЛ ПЛЕЧОМ КОГО-ТО БОЛЬШОГО.", 480, 494, 19, CR(255, 220, 140), 1, true);
    }
}

// ------------------------------------------------------------
//  ДиМа: выбор реплики
// ------------------------------------------------------------
static int dStage = 0;
static int dChoice = -1;
static double dT = 0;
static int dHover = -1;
static const wchar_t* dOpts[3] = {
    L"Извини, брат. Я случайно. Мир?",
    L"Убери руку, очкарик.",
    L"Я тебя не трогал. Ты сам подставился."
};
static void startDima() {
    dStage = 0; dChoice = -1; dT = 0; dHover = -1;
    gScene = S_DIMA;
    sfx(110, 0.18, 3, 0.18, -30);
}
static void onDimaClick() {
    if (dStage == 0) { dStage = 1; sfx(520, 0.05, 2, 0.10); return; }
    if (dStage == 1) {
        if (dHover >= 0) {
            dChoice = dHover;
            dStage = 2;
            sfx(dChoice == 0 ? 660 : 300, 0.10, 2, 0.12);
        }
        return;
    }
    if (dStage == 2) {
        playCS(CS_TABLE(), L"canteen", []() {
            playCS(CS_302(), L"class302", []() {
                playCS(CS_HOME(), L"home", []() { goShop(); });
            });
        });
        dStage = 3;
    }
}
static void updateDima(double dt) { dT += dt; }
static void drawDima() {
    bgCanteen();
    fillA(0, 0, W, H, CR(0, 0, 0), 55);

    if (dStage <= 1) {
        line(636, 306, 336, 294, CR(120, 170, 150), 26);
        line(636, 306, 336, 294, CR(170, 204, 184), 12);
    }
    drawChar(288, 496, 1.0f, CFG_TIMUR, false, 1.0f, gTime, true);
    drawChar(690, 496, 1.16f, CFG_DIMA, true, 1.0f, gTime, true);
    if (dStage <= 1) {
        fillEllipse(322, 288, 24, 20, CR(238, 206, 180));
        fillEllipse(312, 278, 9, 7, CR(238, 206, 180));
    }

    fillRound(46, 386, 868, 144, 18, CR(88, 74, 138));
    fillRound(50, 390, 860, 136, 15, CR(8, 8, 16));

    std::wstring who, text;
    if (dStage == 0) {
        who = L"ДиМа-zxcpipin31";
        text = L"Ты че творишь, сцука!!??";
    } else if (dStage == 1) {
        who = L"ДиМа-zxcpipin31";
        text = L"Ну? Че сказать хочешь?";
    } else if (dStage == 2) {
        if (dChoice == 0) { who = L"Тимур"; text = L"ДиМа отпустил макушку. «Ладно. Бывает. Садись, вместе посидим.»"; }
        else if (dChoice == 1) { who = L"ДиМа-zxcpipin31"; text = L"ДиМа сжал крепче. «Ещё раз так скажешь — и я тебя...» Тимур мягко перевёл тему."; }
        else { who = L"ДиМа-zxcpipin31"; text = L"«Наглый. Мне нравится.» ДиМа отпустил макушку."; }
    } else { who = L""; text = L"Через пару минут они уже сидели за одним столом."; }

    std::wstring nm;
    if (who == L"Тимур") nm = L"ТИМУР";
    else if (who == L"ДиМа-zxcpipin31") nm = who;
    if (!nm.empty()) {
        int tw = textW(nm, 20, true) + 30;
        fillRound(72, 362, tw, 36, 10, speakerColor(who == L"Тимур" ? L"timur" : L"dima"));
        drawText(nm, 72 + 15, 368, 20, CR(10, 10, 18), 0, true);
    }

    auto rows = wrapText(text, 22, 800);
    int yy = 412;
    for (auto& s : rows) { drawText(s, 78, yy, 22, CR(234, 230, 246)); yy += 32; }

    if (dStage == 1) {
        dHover = -1;
        for (int i = 0; i < 3; i++) {
            int bx = 170, bw = 620, bh = 46, by = 56 + i * 58;
            bool hov = ptIn(bx, by, bw, bh);
            if (hov) dHover = i;
            fillRound(bx, by, bw, bh, 10, hov ? CR(70, 96, 70) : CR(22, 34, 30));
            fillRound(bx + 2, by + 2, bw - 4, bh - 4, 8, hov ? CR(46, 72, 48) : CR(16, 24, 22));
            drawText(std::to_wstring(i + 1) + L".", bx + 16, by + 12, 18, CR(150, 200, 150), 0, true);
            drawText(dOpts[i], bx + 48, by + 12, 18, CR(230, 236, 220));
        }
        drawText(L"выбери, что ответить: клик или 1 / 2 / 3", 480, 236, 13, CR(240, 240, 200), 1);
    } else if (dStage == 2) {
        if ((int)(gTime * 2) % 2 == 0) drawText(L"клик / ПРОБЕЛ — дальше", 878, 490, 13, CR(150, 140, 200), 2);
    } else if (dStage == 0) {
        if ((int)(gTime * 2) % 2 == 0) drawText(L"клик / ПРОБЕЛ", 878, 490, 13, CR(150, 140, 200), 2);
    }
}

// ------------------------------------------------------------
//  Реванш: РЕНАТ-ОС
// ------------------------------------------------------------
static std::vector<Card> rHand;
static Card rArtem, rPlayer;
static int rPtsT = 0, rPtsA = 0;
static int rStage = 0;      // 0 выбор, 1 вскрытие, 2 итог, 3 финал, 4 перезапуск
static double rTimer = 0;
static bool rWinLast = false;
static std::wstring rMsg;
static int rHandRects[3][4] = {};
static int rHandCount = 0;
static int rRound = 0;
static int rPicked = -1;
static double rGlitch = 0;

static void beginRematchRound() {
    rRound++;
    rHand.clear();
    int av = rndi(6, 13);
    rArtem = { av, rndi(0, 3), false };
    int winV = rndi(av + 1, 14);
    rHand.push_back({ winV, rndi(0, 3), false });
    rHand.push_back({ rndi(6, 14), rndi(0, 3), false });
    rHand.push_back({ rndi(6, 14), rndi(0, 3), false });
    std::shuffle(rHand.begin(), rHand.end(), rng);
    rStage = 0;
    rPicked = -1;
    rTimer = 0;
    rMsg = L"РЕНАТ-ОС видит его карту. Выбери, чем бить.";
    sfx(720, 0.05, 1, 0.08);
}
static void startRematch() {
    rPtsT = 0; rPtsA = 0; rRound = 0; rGlitch = 0;
    rHand.clear();
    beginRematchRound();
    gScene = S_REMATCH;
}
static void rematchPick(int i) {
    if (rStage != 0 || i < 0 || i >= (int)rHand.size()) return;
    rPlayer = rHand[i];
    rPicked = i;
    rWinLast = rPlayer.v > rArtem.v;
    rStage = 1;
    rTimer = 1.2;
    rGlitch = 0;
    sfx(rWinLast ? 700 : 210, rWinLast ? 0.10 : 0.16, 2, 0.14, rWinLast ? 120 : -70);
}
static void onRematchClick(int x, int y) {
    if (rStage != 0) return;
    for (int i = 0; i < rHandCount; i++) {
        if (x >= rHandRects[i][0] && x <= rHandRects[i][0] + rHandRects[i][2] &&
            y >= rHandRects[i][1] && y <= rHandRects[i][1] + rHandRects[i][3]) {
            rematchPick(i);
            return;
        }
    }
}
static void updateRematch(double dt) {
    if (rStage == 1) {
        rGlitch += dt;
        rTimer -= dt;
        if (rTimer <= 0) {
            if (rWinLast) {
                rPtsT++;
                rMsg = L"РЕНАТ-ОС: ЧИТ ОТКЛОНЁН. АрТеМ в шоке.";
                sfx(880, 0.12, 2, 0.14, 220);
            } else {
                rPtsA++;
                rMsg = L"ГЕНКА-ОС: ЧИТ АКТИВЕН. Этот раунд за АрТеМом.";
                sfx(180, 0.16, 3, 0.16, -60);
            }
            rStage = 2;
            rTimer = 1.5;
        }
    } else if (rStage == 2) {
        rTimer -= dt;
        if (rTimer <= 0) {
            if (rPtsT >= 2) {
                rStage = 3;
                playCS(CS_P2_WIN(), L"canteen", []() { gScene = S_END2; });
            } else if (rPtsA >= 2) {
                rMsg = L"РЕНАТ-ОС: ПЕРЕЗАПУСК...";
                rStage = 4;
                rTimer = 1.6;
                sfx(140, 0.3, 3, 0.18, 90);
            } else beginRematchRound();
        }
    } else if (rStage == 4) {
        rTimer -= dt;
        if (rTimer <= 0) { rPtsT = 0; rPtsA = 0; rRound = 0; beginRematchRound(); }
    }
}
static void drawFighterCard(int x, int y, int w, int h, const CharCfg& cfg, const std::wstring& name,
                            const std::wstring& tag, COLORREF tagCol) {
    fillRound(x, y, w, h, 14, CR(9, 12, 22));
    fillRound(x + 2, y + 2, w - 4, h - 4, 12, CR(15, 21, 36));
    drawAvatar(x + 50, y + 52, 38, cfg, gTime);
    drawText(name, x + 98, y + 20, 19, CR(240, 236, 250), 0, true);
    drawText(tag, x + 98, y + 48, 15, tagCol, 0, true);
    drawText(L"до 2 побед", x + 98, y + 74, 12, CR(110, 116, 140));
}
static void drawRematch() {
    bgCanteen();
    fillA(0, 0, W, H, CR(10, 8, 4), 130);
    fillRound(40, 132, 880, 420, 24, CR(38, 34, 22));
    fillRound(48, 138, 864, 408, 20, CR(34, 48, 42));
    fillRound(48, 138, 864, 10, 8, CR(56, 74, 62));

    drawFighterCard(16, 14, 272, 104, CFG_ARTEM, L"АрТеМ-DUBAI", L"ГЕНКА-ОС: АКТИВЕН", CR(255, 110, 110));
    drawFighterCard(672, 14, 272, 104, CFG_TIMUR, L"ТИМУР", L"РЕНАТ-ОС: АКТИВЕН", CR(255, 212, 90));

    // РЕНАТ-ОС видит карту
    fillRound(672, 126, 272, 52, 12, CR(44, 34, 8));
    drawText(L"РЕНАТ-ОС v1.0", 686, 133, 16, CR(255, 214, 96), 0, true);
    {
        std::wstring cs = L"ВИЖУ: " + rankName(rArtem.v) + std::wstring(1, suitChar(rArtem.s));
        drawText(cs, 686, 153, 15, CR(255, 240, 190));
    }
    fillRound(16, 126, 272, 52, 12, CR(46, 10, 16));
    drawText(L"ГЕНКА-ОС v0.1", 30, 133, 16, CR(255, 96, 96), 0, true);
    drawText(L"статус: " + std::wstring(rPtsT > 0 ? L"в шоке" : L"читер"), 30, 153, 14, CR(255, 176, 176));

    drawText(L"ТИМУР   " + std::to_wstring(rPtsT) + L" : " + std::to_wstring(rPtsA) + L"   АрТеМ", 480, 22, 22,
             CR(240, 236, 250), 1, true);
    drawText(L"РАУНД " + std::to_wstring(rRound), 480, 52, 13, CR(170, 190, 160), 1);

    // карта Артема всегда открыта (РЕНАТ-ОС)
    drawCardShape(500, 150, 106, 148, rArtem, false, false, false);
    drawText(L"КАРТА АрТеМа (видна)", 553, 303, 12, CR(255, 214, 120), 1);
    if (rStage >= 1) {
        drawCardShape(354, 150, 106, 148, rPlayer, false, false, false);
        drawText(L"ТВОЯ КАРТА", 407, 303, 12, CR(150, 220, 150), 1);
    } else {
        fillRound(354, 150, 106, 148, 10, CR(0, 0, 0));
        drawText(L"?", 407, 214, 40, CR(70, 90, 70), 1, true);
        drawText(L"ТВОЯ КАРТА", 407, 303, 12, CR(150, 220, 150), 1);
    }

    {
        COLORREF mc = CR(230, 226, 240);
        if (rStage == 2) mc = rWinLast ? CR(140, 255, 150) : CR(255, 130, 130);
        if (rStage == 4) mc = CR(255, 190, 90);
        auto rows = wrapText(rMsg, 18, 760);
        int yy = 320;
        for (auto& s : rows) { drawText(s, 480, yy, 18, mc, 1); yy += 24; }
    }

    rHandCount = 0;
    int hovered = -1;
    for (int i = 0; i < (int)rHand.size() && i < 3; i++) {
        int w = 106, h = 148, gap = 118;
        int x = 480 + (i - 1) * gap - w / 2;
        int y = 376;
        rHandRects[i][0] = x; rHandRects[i][1] = y - 20; rHandRects[i][2] = w; rHandRects[i][3] = h + 20;
        if (rStage == 0 && ptIn(x, y - 18, w, h + 18)) hovered = i;
        rHandCount++;
    }
    for (int i = 0; i < (int)rHand.size() && i < 3; i++) {
        if (i == hovered && i != rPicked) continue;
        int w = 106, h = 148, gap = 118;
        int x = 480 + (i - 1) * gap - w / 2;
        drawCardShape(x, 376, w, h, rHand[i], false, i == rPicked, false);
    }
    if (hovered >= 0) {
        int w = 106, h = 148, gap = 118;
        int x = 480 + (hovered - 1) * gap - w / 2;
        drawCardShape(x, 358, w, h, rHand[hovered], false, false, false);
    }
    if (rStage == 0) drawText(L"клик по карте или 1 / 2 / 3", 480, 528, 12, CR(160, 180, 140), 1);
}
// ------------------------------------------------------------
//  Переходы главы 2 и финальный экран
// ------------------------------------------------------------
static void goShop() {
    gWalkMode = 2;
    gPlayer.x = 480; gPlayer.y = 486;
    gPlayer.faceLeft = false; gPlayer.moving = false;
    gScene = S_WALK;
}
static void startShopScene() {
    playCS(CS_SHOP(), L"shop", []() {
        playCS(CS_NIGHT(), L"home", []() { startRematch(); });
    });
}
static void startPart2() {
    gWalkMode = 1;
    gBanner2 = 2.6;
    gPlayer.x = 480; gPlayer.y = 486;
    gPlayer.faceLeft = false; gPlayer.moving = false;
    playCS(CS_P2_TOILET(), L"toilet", []() { gWalkMode = 1; gPlayer.x = 480; gPlayer.y = 486; gScene = S_WALK; });
}

static void drawEnd2() {
    bgCity();
    fillA(0, 0, W, H, CR(4, 3, 10), 185);
    fillRound(118, 58, 724, 296, 24, CR(10, 8, 18));
    fillRound(118, 58, 724, 6, 3, CR(90, 76, 40));

    CharCfg timur = CFG_TIMUR;
    timur.eye = 2;
    drawChar(826, 512, 0.96f, timur, true, 0.95f, gTime, true);

    drawText(L"КОНЕЦ ГЛАВЫ 2", 480, 92, 62, CR(255, 198, 61), 1, true, L"Arial Black");
    drawText(L"РЕНАТ-ОС: АКТИВЕН", 480, 168, 24, CR(255, 224, 120), 1, true);
    drawText(L"Тимур выебал АрТеМа-DUBAI'я. В карты, конечно.", 480, 218, 18, CR(240, 236, 250), 1);
    drawText(L"ДиМа-zxcpipin31 больше не заговаривал про две недели.", 480, 250, 16, CR(176, 170, 206), 1);
    drawText(L"Гришка золотой глаз всё видел. Он всё видит.", 480, 278, 16, CR(255, 122, 170), 1);
    drawText(L"Папа Ренат сделал лучший глаз бога на рынке.", 480, 306, 15, CR(150, 146, 180), 1);

    if ((int)(gTime * 2) % 2 == 0)
        drawText(L"R — заново · ENTER — в начало", 480, 420, 22, CR(255, 236, 190), 1, true);
    drawText(L"ТИМУР · АрТеМ-DUBAI · ПЛАТОН-ЛЁХА · ДиМа-zxcpipin31 · ПАПА РЕНАТ · ГРИШКА ЗОЛОТОЙ ГЛАЗ", 480, 508, 11, CR(94, 90, 126), 1);
}
static void onKeyDown(int vk) {
    if (vk == 'M') { gMuted = !gMuted; return; }
    if (vk == VK_F12) { saveBMP(L"screenshot.bmp"); return; }
    switch (gScene) {
    case S_TITLE:
        if (vk == VK_RETURN || vk == VK_SPACE) startIntro();
        break;
    case S_CUTSCENE:
        if (vk == VK_SPACE || vk == VK_RETURN) advanceCS();
        break;
    case S_CARDS:
        if (vk == '1') pickCard(0);
        else if (vk == '2') pickCard(1);
        else if (vk == '3') pickCard(2);
        break;
    case S_FIGHT:
        if (vk == VK_SPACE) fightHit();
        else if (vk == 'R') restartGame();
        break;
    case S_END:
        if (vk == VK_RETURN || vk == VK_SPACE) startPart2();
        else if (vk == 'R') restartGame();
        break;
    case S_QUEUE:
        if (vk == VK_SPACE || vk == VK_RETURN) onQueuePush();
        break;
    case S_DIMA:
        if (vk == VK_SPACE || vk == VK_RETURN) onDimaClick();
        else if ((vk == '1' || vk == '2' || vk == '3') && dStage == 1) {
            dHover = vk - '1';
            onDimaClick();
        }
        break;
    case S_REMATCH:
        if (vk == '1') rematchPick(0);
        else if (vk == '2') rematchPick(1);
        else if (vk == '3') rematchPick(2);
        break;
    case S_END2:
        if (vk == 'R' || vk == VK_RETURN || vk == VK_SPACE) restartGame();
        break;
    default:
        break;
    }
}
static void onClick() {
    audioInit();
    switch (gScene) {
    case S_TITLE: startIntro(); break;
    case S_CUTSCENE: advanceCS(); break;
    case S_CARDS:
        if (gStage == 0) {
            for (int i = 0; i < gHandCount; i++) {
                if (ptIn(gHandRects[i][0], gHandRects[i][1], gHandRects[i][2], gHandRects[i][3])) {
                    pickCard(i);
                    break;
                }
            }
        }
        break;
    case S_FIGHT: fightHit(); break;
    case S_END: startPart2(); break;
    case S_QUEUE: onQueuePush(); break;
    case S_DIMA: onDimaClick(); break;
    case S_REMATCH: onRematchClick(gMouseX, gMouseY); break;
    case S_END2: restartGame(); break;
    default: break;
    }
}

static void startIntro() { playCS(CS_INTRO(), L"street", []() { gScene = S_WALK; }); }
static void meetStart() {
    if (gScene != S_WALK) return;
    gPlayer.y = 392;
    playCS(CS_MEET(), L"school", []() { startCards(); });
}
static void restartGame() {
    gWalkMode = 0;
    gPlayer.x = 480; gPlayer.y = 486; gPlayer.faceLeft = false; gPlayer.moving = false;
    gPlayer.phase = 0;
    csLines.clear();
    initTitle();
    gAuto = wcsstr(GetCommandLineW(), L"--shots") != nullptr;
    initSnow();
    gScene = S_TITLE;
}

static void update(double dt) {
    if (gBanner2 > 0) gBanner2 -= dt;
    switch (gScene) {
    case S_TITLE: updateTitle(dt); break;
    case S_CUTSCENE: updateCS(dt); break;
    case S_WALK: updateWalk(dt); break;
    case S_CARDS: updateCards(dt); break;
    case S_FIGHT: updateFight(dt); break;
    case S_END: updateSnow(dt); break;
    case S_QUEUE: updateQueue(dt); break;
    case S_DIMA: updateDima(dt); break;
    case S_REMATCH: updateRematch(dt); break;
    case S_END2: updateSnow(dt); break;
    default: break;
    }
}
static void render() {
    switch (gScene) {
    case S_TITLE: drawTitle(); break;
    case S_CUTSCENE: drawCS(); break;
    case S_WALK: drawWalk(); break;
    case S_CARDS: drawCards(); break;
    case S_FIGHT: drawFight(); break;
    case S_END: drawEnd(); break;
    case S_QUEUE: drawQueue(); break;
    case S_DIMA: drawDima(); break;
    case S_REMATCH: drawRematch(); break;
    case S_END2: drawEnd2(); break;
    default: break;
    }
    if (gBanner2 > 0.05) {
        float a = (float)std::min(1.0, std::min(gBanner2 / 0.45, (2.6 - gBanner2) / 0.35));
        fillA(0, 0, W, 128, CR(4, 2, 10), (int)(150 * a));
        drawText(L"ГЛАВА 2", 480, 26, 26, CR(200, 190, 160), 1, true);
        drawText(L"РЕНАТ-ОС", 480, 62, 44, CR(255, 224, 120), 1, true, L"Arial Black");
    }
}

// ------------------------------------------------------------
//  Win32
// ------------------------------------------------------------
// ------------------------------------------------------------
//  Скриншоты и авто-режим (--shots)
// ------------------------------------------------------------
static void saveBMP(const wchar_t* path) {
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = W;
    bi.bmiHeader.biHeight = -H;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;
    int sz = W * H * 3;
    std::vector<unsigned char> buf(sz);
    HDC dc = GetDC(gWnd);
    GetDIBits(dc, gBmp, 0, H, buf.data(), &bi, DIB_RGB_COLORS);
    ReleaseDC(gWnd, dc);

    BITMAPFILEHEADER fh = {};
    fh.bfType = 0x4D42;
    fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fh.bfSize = fh.bfOffBits + sz;
    BITMAPINFOHEADER ih = bi.bmiHeader;

    FILE* f = _wfopen(path, L"wb");
    if (!f) return;
    fwrite(&fh, sizeof(fh), 1, f);
    fwrite(&ih, sizeof(ih), 1, f);
    fwrite(buf.data(), 1, sz, f);
    fclose(f);
}

static void autoStep(double dt) {
    gAutoT += dt;
    double t = gAutoT;
    if (t > 0.50 && gAutoStep == 0) { saveBMP(L"shot1.bmp"); gAutoStep = 1; }
    if (t > 0.70 && gAutoStep == 1) { startIntro(); gAutoStep = 2; }
    if (t > 1.60 && gAutoStep == 2) { if (!csLines.empty()) { csIdx = 1; csShown = 9999; } gAutoStep = 3; }
    if (t > 2.20 && gAutoStep == 3) { saveBMP(L"shot2.bmp"); gAutoStep = 4; }
    if (t > 2.40 && gAutoStep == 4) { gScene = S_WALK; gPlayer.x = 480; gPlayer.y = 430; gAutoStep = 5; }
    if (t > 3.00 && gAutoStep == 5) { saveBMP(L"shot3.bmp"); gAutoStep = 6; }
    if (t > 3.20 && gAutoStep == 6) { startCards(); gAutoStep = 7; }
    if (t > 3.90 && gAutoStep == 7) { pickCard(0); gAutoStep = 8; }
    if (t > 4.70 && gAutoStep == 8) { saveBMP(L"shot4.bmp"); gAutoStep = 9; }
    if (t > 4.90 && gAutoStep == 9) { gRound = 2; beginRound(); pickCard(0); gAutoStep = 10; }
    if (t > 5.50 && gAutoStep == 10) { saveBMP(L"shot5.bmp"); gAutoStep = 11; }
    if (t > 5.70 && gAutoStep == 11) { startFight(); for (int i = 0; i < 6; i++) fightHit(); gAutoStep = 12; }
    if (t > 6.40 && gAutoStep == 12) { saveBMP(L"shot6.bmp"); gAutoStep = 13; }
    if (t > 6.60 && gAutoStep == 13) { fHp = 0; fDoneT = 0; gAutoStep = 14; }
    if (t > 7.10 && gAutoStep == 14) { saveBMP(L"shot7.bmp"); gAutoStep = 15; }
    if (t > 7.30 && gAutoStep == 15) { gScene = S_END; gAutoStep = 16; }
    if (t > 7.90 && gAutoStep == 16) { saveBMP(L"shot8.bmp"); gAutoStep = 17; }
    if (t > 8.20 && gAutoStep == 17) { PostQuitMessage(0); gAutoStep = 18; }
}

static void autoStep2(double dt) {
    gAutoT += dt;
    double t = gAutoT;
    if (t > 0.50 && gAutoStep == 0) { startPart2(); gAutoStep = 1; }
    if (t > 1.10 && gAutoStep == 1) { if (!csLines.empty()) { csIdx = 1; csShown = 9999; } gAutoStep = 2; }
    if (t > 1.70 && gAutoStep == 2) { saveBMP(L"c1.bmp"); gAutoStep = 3; }
    if (t > 1.90 && gAutoStep == 3) { gWalkMode = 1; gPlayer.x = 480; gPlayer.y = 424; gScene = S_WALK; gAutoStep = 4; }
    if (t > 2.60 && gAutoStep == 4) { saveBMP(L"c2.bmp"); gAutoStep = 5; }
    if (t > 2.80 && gAutoStep == 5) { startQueue(); qProg = 55; qBubble = L"ДА ЗАНИМАЛ!!!!!!!!!!"; qBubbleT = 5; gAutoStep = 6; }
    if (t > 3.50 && gAutoStep == 6) { saveBMP(L"c3.bmp"); gAutoStep = 7; }
    if (t > 3.70 && gAutoStep == 7) { startDima(); dStage = 1; gAutoStep = 8; }
    if (t > 4.40 && gAutoStep == 8) { saveBMP(L"c4.bmp"); gAutoStep = 9; }
    if (t > 4.60 && gAutoStep == 9) { playCS(CS_TABLE(), L"canteen", []() {}); csIdx = 3; csShown = 9999; gAutoStep = 10; }
    if (t > 5.30 && gAutoStep == 10) { saveBMP(L"c5.bmp"); gAutoStep = 11; }
    if (t > 5.50 && gAutoStep == 11) { playCS(CS_302(), L"class302", []() {}); csIdx = 3; csShown = 9999; gAutoStep = 12; }
    if (t > 6.20 && gAutoStep == 12) { saveBMP(L"c6.bmp"); gAutoStep = 13; }
    if (t > 6.40 && gAutoStep == 13) { playCS(CS_HOME(), L"home", []() {}); csIdx = 5; csShown = 9999; gAutoStep = 14; }
    if (t > 7.10 && gAutoStep == 14) { saveBMP(L"c7.bmp"); gAutoStep = 15; }
    if (t > 7.30 && gAutoStep == 15) { playCS(CS_SHOP(), L"shop", []() {}); csIdx = 2; csShown = 9999; gAutoStep = 16; }
    if (t > 8.00 && gAutoStep == 16) { saveBMP(L"c8.bmp"); gAutoStep = 17; }
    if (t > 8.20 && gAutoStep == 17) { startRematch(); gAutoStep = 18; }
    if (t > 8.90 && gAutoStep == 18) { saveBMP(L"c9.bmp"); gAutoStep = 19; }
    if (t > 9.10 && gAutoStep == 19) {
        int pick = 0;
        for (int i = 0; i < (int)rHand.size(); i++) if (rHand[i].v > rArtem.v) { pick = i; break; }
        rematchPick(pick);
        gAutoStep = 20;
    }
    if (t > 9.90 && gAutoStep == 20) { saveBMP(L"c10.bmp"); gAutoStep = 21; }
    if (t > 10.10 && gAutoStep == 21) { rPtsT = 2; gScene = S_END2; gAutoStep = 22; }
    if (t > 10.80 && gAutoStep == 22) { saveBMP(L"c11.bmp"); gAutoStep = 23; }
    if (t > 11.10 && gAutoStep == 23) { PostQuitMessage(0); gAutoStep = 24; }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (!(lp & 0x40000000)) onKeyDown((int)wp);
        if (wp < 256) gKeys[wp] = true;
        return 0;
    case WM_KEYUP:
        if (wp < 256) gKeys[wp] = false;
        return 0;
    case WM_LBUTTONDOWN:
        gMouseX = (int)(short)LOWORD(lp);
        gMouseY = (int)(short)HIWORD(lp);
        onClick();
        return 0;
    case WM_MOUSEMOVE:
        gMouseX = (int)(short)LOWORD(lp);
        gMouseY = (int)(short)HIWORD(lp);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        if (gDC) BitBlt(dc, 0, 0, W, H, gDC, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    SetProcessDPIAware();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"TimurDubaiGame";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT r = { 0, 0, W, H };
    AdjustWindowRect(&r, style, FALSE);
    int ww = r.right - r.left, wh = r.bottom - r.top;
    int sxw = (GetSystemMetrics(SM_CXSCREEN) - ww) / 2;
    int syw = (GetSystemMetrics(SM_CYSCREEN) - wh) / 2;

    gWnd = CreateWindowExW(0, L"TimurDubaiGame", L"ТИМУР и АрТеМ-DUBAI — карточная драма",
                           style, sxw, syw, ww, wh, nullptr, nullptr, hInst, nullptr);
    if (!gWnd) return 1;
    ShowWindow(gWnd, SW_SHOW);
    UpdateWindow(gWnd);

    HDC dc = GetDC(gWnd);
    gDC = CreateCompatibleDC(dc);
    gBmp = CreateCompatibleBitmap(dc, W, H);
    SelectObject(gDC, gBmp);
    SetBkMode(gDC, TRANSPARENT);
    ReleaseDC(gWnd, dc);

    initChars();
    initSnow();
    initTitle();
    gAuto2 = wcsstr(GetCommandLineW(), L"--shots2") != nullptr;
    gAuto = gAuto2 || wcsstr(GetCommandLineW(), L"--shots") != nullptr;

    LARGE_INTEGER freq, prev;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);

    MSG msg;
    bool running = true;
    while (running) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { running = false; break; }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!running) break;

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double dt = (double)(now.QuadPart - prev.QuadPart) / (double)freq.QuadPart;
        prev = now;
        if (dt > 0.05) dt = 0.05;
        gTime += dt;
        if (gAuto) { if (gAuto2) autoStep2(dt); else autoStep(dt); }

        update(dt);
        render();

        HDC wdc = GetDC(gWnd);
        BitBlt(wdc, 0, 0, W, H, gDC, 0, 0, SRCCOPY);
        ReleaseDC(gWnd, wdc);
        Sleep(1);
    }

    audioShutdown();
    return 0;
}
