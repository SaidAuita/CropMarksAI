#include "MakeCropMarksAIPlugin.h"
#include <cstdio>
#include <cmath>
#include <commctrl.h>
#include <windowsx.h>

#define PLUGIN_VERSION_STR "v1.31"

#pragma comment(lib, "comctl32.lib")

// Control IDs
#define IDC_HEADER_TITLE        2001
#define IDC_HEADER_VER          2002
#define IDC_BTN_SETTINGS        2003

// Tab Navigation
#define IDC_TAB_MARKS           2020
#define IDC_TAB_PANTONES        2021
#define IDC_TAB_SETTINGS        2022

// Pantones Tab Controls
#define IDC_LBL_PROCESS_INKS    2601
#define IDC_CHK_INK_C           2602
#define IDC_CHK_INK_M           2603
#define IDC_CHK_INK_Y           2604
#define IDC_CHK_INK_K           2605

#define IDC_LBL_SPOTS_HEADER    2610
#define IDC_BTN_SCAN_SPOTS      2611
#define IDC_BTN_SPOTS_ALL       2612
#define IDC_BTN_SPOTS_NONE      2613
#define IDC_BTN_SPOTS_FILTER    2614
#define IDC_STATIC_SPOTS_COUNT  2615
#define IDC_LIST_SPOTS          2616
#define IDC_BTN_PANTONES_APPLY  2617

// Settings Tab Controls
#define IDC_LBL_SET_BAR_HEADER    2701
#define IDC_LBL_SET_BAR_MODE      2702
#define IDC_CMB_BAR_MODE          2703
#define IDC_LBL_SET_PATCH_SIZE    2704
#define IDC_ED_SET_PATCH_W        2705
#define IDC_ED_SET_PATCH_H        2706
#define IDC_LBL_SET_BAR_OFFSET    2707
#define IDC_ED_SET_BAR_OFFSET     2708
#define IDC_LBL_SET_BAR_LEN       2709
#define IDC_CMB_BAR_LEN           2710
#define IDC_LBL_SET_BAR_ORDER     2711
#define IDC_CMB_BAR_ORDER         2712

#define IDC_LBL_SET_REG_HEADER    2720
#define IDC_LBL_SET_REG_DIAM      2721
#define IDC_ED_SET_REG_DIAM       2722
#define IDC_ED_SET_REG_LEN        2723
#define IDC_LBL_SET_REG_STROKE    2724
#define IDC_ED_SET_REG_STROKE     2725
#define IDC_CMB_SET_REG_STROKE_UN 2726
#define IDC_LBL_SET_REG_OFFSET    2727
#define IDC_ED_SET_REG_OFF_H      2728
#define IDC_ED_SET_REG_OFF_V      2729
#define IDC_LBL_SET_REG_POS       2730
#define IDC_CMB_SET_REG_POS       2731
#define IDC_CHK_SET_REG_WHITE     2732
#define IDC_LBL_SET_REG_ORDER     2733
#define IDC_CMB_SET_REG_ORDER     2734

#define IDC_LBL_SET_DOC_HEADER    2740
#define IDC_CHK_SET_DOC_INFO      2741
#define IDC_CHK_SET_DOC_TRANSLIT  2742
#define IDC_LBL_SET_HK_HEADER     2743
#define IDC_STATIC_SET_HK_INFO    2744
#define IDC_BTN_SET_HOTKEYS       2745
#define IDC_BTN_SET_SAVE_DEFAULTS 2746
#define IDC_BTN_SETTINGS_APPLY    2747

#define IDC_LBL_LENGTH          2010
#define IDC_LBL_OFFSET          2011
#define IDC_LBL_BLEED           2012
#define IDC_LBL_STROKE          2013
#define IDC_LBL_TARGET          2014
#define IDC_LBL_BOUNDS          2015
#define IDC_LBL_GROUP           2016

#define IDC_PRESET_BTN_BASE     2100 // 2100..2107 for presets 1..8
#define IDC_ED_PRESET_NAME      2110
#define IDC_BTN_PRESET_SAVE     2111

// Direction checkboxes
#define IDC_CHK_TL              2201
#define IDC_CHK_TC              2202
#define IDC_CHK_TR              2203
#define IDC_CHK_BL              2204
#define IDC_CHK_BC              2205
#define IDC_CHK_BR              2206
#define IDC_CHK_LT              2207
#define IDC_CHK_LC              2208
#define IDC_CHK_LB              2209
#define IDC_CHK_RT              2210
#define IDC_CHK_RC              2211
#define IDC_CHK_RB              2212

#define IDC_BTN_DIR_ALL         2220
#define IDC_BTN_DIR_CORNERS     2221
#define IDC_BTN_DIR_NONE        2222

// Parameters inputs
#define IDC_ED_LEN_VER          2301
#define IDC_ED_LEN_HOR          2302
#define IDC_CMB_UNITS           2303

#define IDC_ED_OFF_VER          2304
#define IDC_ED_OFF_HOR          2305

#define IDC_ED_BLEED_VER        2306
#define IDC_ED_BLEED_HOR        2307

#define IDC_ED_STROKE           2308
#define IDC_CMB_STROKE_UNITS    2309

// Options
#define IDC_CHK_WHITE_CONTOUR   2401
#define IDC_CMB_BOUNDS          2402
#define IDC_CMB_TARGET          2403
#define IDC_CMB_GROUP           2404
#define IDC_CHK_PERIMETER_ONLY  2405
#define IDC_CHK_REG_MARKS       2406
#define IDC_CHK_BARS_LONG       2407
#define IDC_CHK_BARS_SHORT      2408

// Actions & Links
#define IDC_BTN_CREATE          2501
#define IDC_STATIC_STATUS       2502
#define IDC_STATIC_LINK_AUTHOR  2503
#define IDC_STATIC_LINK_TOOLS   2504

static HHOOK        g_hKeyHook = NULL;
static WNDPROC      g_oldPanelProc = NULL;

static HFONT        g_hUIFont = NULL;
static HFONT        g_hHeaderFont = NULL;
static HFONT        g_hBoldFont = NULL;
static HFONT        g_hSmallFont = NULL;
static HBRUSH       g_hPanelBgBrush = NULL;
static HBRUSH       g_hInputBgBrush = NULL;
static float        g_appliedFontScale = 0.0f;

static float GetPanelScale(HWND hwnd) {
    float aiScale = 1.0f;
    if (sAIUser && sAIUser->GetAppScaleFactor) {
        AIReal s = sAIUser->GetAppScaleFactor();
        if (s >= 0.5f && s <= 5.0f) {
            aiScale = (float)s;
        }
    }

    float winDpiScale = 1.0f;
    if (hwnd) {
        HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
        if (hUser32) {
            typedef UINT (WINAPI *GetDpiForWindowProc)(HWND);
            GetDpiForWindowProc pGetDpi = (GetDpiForWindowProc)GetProcAddress(hUser32, "GetDpiForWindow");
            if (pGetDpi) {
                UINT dpi = pGetDpi(hwnd);
                if (dpi > 0) winDpiScale = (float)dpi / 96.0f;
            }
        }
        if (winDpiScale <= 0.1f) {
            HDC hdc = GetDC(hwnd);
            if (hdc) {
                int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
                ReleaseDC(hwnd, hdc);
                if (dpi > 0) winDpiScale = (float)dpi / 96.0f;
            }
        }
    }

    float finalScale = (std::max)(aiScale, winDpiScale);
    if (finalScale < 0.75f) finalScale = 0.75f;
    if (finalScale > 4.0f)  finalScale = 4.0f;
    return finalScale;
}

static void EnsureFontsForScale(HWND hwnd, float scale) {
    if (std::abs(g_appliedFontScale - scale) < 0.01f && g_hUIFont) return;
    g_appliedFontScale = scale;

    if (g_hUIFont)     { DeleteObject(g_hUIFont);     g_hUIFont = NULL; }
    if (g_hHeaderFont) { DeleteObject(g_hHeaderFont); g_hHeaderFont = NULL; }
    if (g_hBoldFont)   { DeleteObject(g_hBoldFont);   g_hBoldFont = NULL; }
    if (g_hSmallFont)  { DeleteObject(g_hSmallFont);  g_hSmallFont = NULL; }

    int szUI     = -(int)(11.0f * scale + 0.5f);
    int szHeader = -(int)(13.0f * scale + 0.5f);
    int szSmall  = -(int)(10.0f * scale + 0.5f);

    g_hUIFont = CreateFontA(szUI, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_hHeaderFont = CreateFontA(szHeader, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_hBoldFont = CreateFontA(szUI, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_hSmallFont = CreateFontA(szSmall, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    if (hwnd && IsWindow(hwnd)) {
        HWND hTitle = GetDlgItem(hwnd, IDC_HEADER_TITLE);
        if (hTitle) SendMessage(hTitle, WM_SETFONT, (WPARAM)g_hHeaderFont, TRUE);

        HWND hVer = GetDlgItem(hwnd, IDC_HEADER_VER);
        if (hVer) SendMessage(hVer, WM_SETFONT, (WPARAM)g_hSmallFont, TRUE);

        HWND hEdPName = GetDlgItem(hwnd, IDC_ED_PRESET_NAME);
        if (hEdPName) SendMessage(hEdPName, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);

        HWND hLinkAuthor = GetDlgItem(hwnd, IDC_STATIC_LINK_AUTHOR);
        if (hLinkAuthor) SendMessage(hLinkAuthor, WM_SETFONT, (WPARAM)g_hSmallFont, TRUE);

        HWND hLinkTools = GetDlgItem(hwnd, IDC_STATIC_LINK_TOOLS);
        if (hLinkTools) SendMessage(hLinkTools, WM_SETFONT, (WPARAM)g_hSmallFont, TRUE);

        for (int cid = IDC_CHK_TL; cid <= IDC_CHK_RB; ++cid) {
            HWND hLbl = GetDlgItem(hwnd, cid + 1000);
            if (hLbl) SendMessage(hLbl, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
        }

        HWND hLblPerim = GetDlgItem(hwnd, IDC_CHK_PERIMETER_ONLY + 1000);
        if (hLblPerim) SendMessage(hLblPerim, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        HWND hLblWhite = GetDlgItem(hwnd, IDC_CHK_WHITE_CONTOUR + 1000);
        if (hLblWhite) SendMessage(hLblWhite, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        HWND hLblReg = GetDlgItem(hwnd, IDC_CHK_REG_MARKS + 1000);
        if (hLblReg) SendMessage(hLblReg, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        HWND hLblBarL = GetDlgItem(hwnd, IDC_CHK_BARS_LONG + 1000);
        if (hLblBarL) SendMessage(hLblBarL, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        HWND hLblBarS = GetDlgItem(hwnd, IDC_CHK_BARS_SHORT + 1000);
        if (hLblBarS) SendMessage(hLblBarS, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);

        const int allGeneralIds[] = {
            IDC_BTN_SETTINGS, IDC_BTN_PRESET_SAVE, IDC_BTN_DIR_ALL, IDC_BTN_DIR_CORNERS, IDC_BTN_DIR_NONE,
            IDC_LBL_LENGTH, IDC_ED_LEN_VER, IDC_ED_LEN_HOR, IDC_CMB_UNITS,
            IDC_LBL_OFFSET, IDC_ED_OFF_VER, IDC_ED_OFF_HOR,
            IDC_LBL_BLEED,  IDC_ED_BLEED_VER, IDC_ED_BLEED_HOR,
            IDC_LBL_STROKE, IDC_ED_STROKE, IDC_CMB_STROKE_UNITS,
            IDC_LBL_TARGET, IDC_CMB_TARGET,
            IDC_LBL_BOUNDS, IDC_CMB_BOUNDS,
            IDC_LBL_GROUP,  IDC_CMB_GROUP,
            IDC_BTN_CREATE, IDC_STATIC_STATUS
        };
        for (int cid : allGeneralIds) {
            HWND h = GetDlgItem(hwnd, cid);
            if (h) SendMessage(h, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        }
        for (int i = 0; i < 8; ++i) {
            HWND h = GetDlgItem(hwnd, IDC_PRESET_BTN_BASE + i);
            if (h) SendMessage(h, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        }

        // Tab Navigation Buttons
        HWND hTab0 = GetDlgItem(hwnd, IDC_TAB_MARKS);
        HWND hTab1 = GetDlgItem(hwnd, IDC_TAB_PANTONES);
        HWND hTab2 = GetDlgItem(hwnd, IDC_TAB_SETTINGS);
        if (hTab0) SendMessage(hTab0, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
        if (hTab1) SendMessage(hTab1, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
        if (hTab2) SendMessage(hTab2, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

        // Pantones Tab
        HWND hLblProc = GetDlgItem(hwnd, IDC_LBL_PROCESS_INKS);
        if (hLblProc) SendMessage(hLblProc, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
        HWND hLblSpotsH = GetDlgItem(hwnd, IDC_LBL_SPOTS_HEADER);
        if (hLblSpotsH) SendMessage(hLblSpotsH, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
        HWND hSpotsCnt = GetDlgItem(hwnd, IDC_STATIC_SPOTS_COUNT);
        if (hSpotsCnt) SendMessage(hSpotsCnt, WM_SETFONT, (WPARAM)g_hSmallFont, TRUE);
        HWND hSpotsList = GetDlgItem(hwnd, IDC_LIST_SPOTS);
        if (hSpotsList) SendMessage(hSpotsList, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);

        for (int chk = IDC_CHK_INK_C; chk <= IDC_CHK_INK_K; ++chk) {
            HWND hLbl = GetDlgItem(hwnd, chk + 1000);
            if (hLbl) SendMessage(hLbl, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        }
        const int pantoneBtnIds[] = {
            IDC_BTN_SCAN_SPOTS, IDC_BTN_SPOTS_ALL, IDC_BTN_SPOTS_NONE, IDC_BTN_SPOTS_FILTER, IDC_BTN_PANTONES_APPLY
        };
        for (int cid : pantoneBtnIds) {
            HWND h = GetDlgItem(hwnd, cid);
            if (h) SendMessage(h, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        }

        // Settings Tab Headers
        const int settingHeaderIds[] = {
            IDC_LBL_SET_BAR_HEADER, IDC_LBL_SET_REG_HEADER, IDC_LBL_SET_DOC_HEADER, IDC_LBL_SET_HK_HEADER
        };
        for (int cid : settingHeaderIds) {
            HWND h = GetDlgItem(hwnd, cid);
            if (h) SendMessage(h, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
        }
        HWND hHkInfo = GetDlgItem(hwnd, IDC_STATIC_SET_HK_INFO);
        if (hHkInfo) SendMessage(hHkInfo, WM_SETFONT, (WPARAM)g_hSmallFont, TRUE);

        HWND hLblRegWhite = GetDlgItem(hwnd, IDC_CHK_SET_REG_WHITE + 1000);
        if (hLblRegWhite) SendMessage(hLblRegWhite, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        HWND hLblDocInfo = GetDlgItem(hwnd, IDC_CHK_SET_DOC_INFO + 1000);
        if (hLblDocInfo) SendMessage(hLblDocInfo, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        HWND hLblDocTrans = GetDlgItem(hwnd, IDC_CHK_SET_DOC_TRANSLIT + 1000);
        if (hLblDocTrans) SendMessage(hLblDocTrans, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);

        const int settingGeneralIds[] = {
            IDC_LBL_SET_BAR_MODE, IDC_CMB_BAR_MODE,
            IDC_LBL_SET_PATCH_SIZE, IDC_ED_SET_PATCH_W, IDC_ED_SET_PATCH_H,
            IDC_LBL_SET_BAR_OFFSET, IDC_ED_SET_BAR_OFFSET,
            IDC_LBL_SET_BAR_LEN, IDC_CMB_BAR_LEN,
            IDC_LBL_SET_BAR_ORDER, IDC_CMB_BAR_ORDER,
            IDC_LBL_SET_REG_DIAM, IDC_ED_SET_REG_DIAM, IDC_ED_SET_REG_LEN,
            IDC_LBL_SET_REG_STROKE, IDC_ED_SET_REG_STROKE, IDC_CMB_SET_REG_STROKE_UN,
            IDC_LBL_SET_REG_OFFSET, IDC_ED_SET_REG_OFF_H, IDC_ED_SET_REG_OFF_V,
            IDC_LBL_SET_REG_POS, IDC_CMB_SET_REG_POS,
            IDC_LBL_SET_REG_ORDER, IDC_CMB_SET_REG_ORDER,
            IDC_BTN_SET_HOTKEYS, IDC_BTN_SET_SAVE_DEFAULTS, IDC_BTN_SETTINGS_APPLY
        };
        for (int cid : settingGeneralIds) {
            HWND h = GetDlgItem(hwnd, cid);
            if (h) SendMessage(h, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        }
    }
}

struct UIThemeColors {
    bool isDark;
    COLORREF panelBg;
    COLORREF textPrimary;
    COLORREF textSecondary;
    COLORREF btnBg;
    COLORREF btnBgHover;
    COLORREF btnBgPressed;
    COLORREF btnBorder;
    COLORREF btnText;
    COLORREF accentBg;
    COLORREF accentText;
    COLORREF inputBg;
    COLORREF inputBorder;
    COLORREF statusBg;
    COLORREF statusBorder;
    COLORREF statusText;
    COLORREF linkColor;
};

static UIThemeColors GetCurrentThemeColors() {
    UIThemeColors c;
    bool isDark = true;
    if (sAIUITheme) {
        isDark = sAIUITheme->IsUIThemeDark() != 0;
    }
    c.isDark = isDark;

    if (isDark) {
        // Dark / Medium Dark theme (Illustrator native)
        c.panelBg       = RGB(53, 53, 53);      // #353535
        c.textPrimary   = RGB(235, 235, 235);   // #ebebeb
        c.textSecondary = RGB(160, 160, 160);   // #a0a0a0
        c.btnBg         = RGB(68, 68, 68);      // #444444
        c.btnBgHover    = RGB(84, 84, 84);      // #545454
        c.btnBgPressed  = RGB(50, 50, 50);      // #323232
        c.btnBorder     = RGB(92, 92, 92);      // #5c5c5c
        c.btnText       = RGB(235, 235, 235);   // #ebebeb
        c.accentBg      = RGB(20, 115, 230);    // #1473e6 Adobe Blue
        c.accentText    = RGB(255, 255, 255);
        c.inputBg       = RGB(38, 38, 38);      // #262626
        c.inputBorder   = RGB(75, 75, 75);      // #4b4b4b
        c.statusBg      = RGB(42, 42, 42);      // #2a2a2a
        c.statusBorder  = RGB(70, 70, 70);      // #464646
        c.statusText    = RGB(215, 215, 215);   // #d7d7d7
        c.linkColor     = RGB(140, 185, 235);   // #8cb9eb
    } else {
        // Light / Medium Light theme (Illustrator native)
        c.panelBg       = RGB(240, 240, 240);   // #f0f0f0
        c.textPrimary   = RGB(32, 32, 32);      // #202020
        c.textSecondary = RGB(115, 115, 115);   // #737373
        c.btnBg         = RGB(255, 255, 255);   // #ffffff
        c.btnBgHover    = RGB(240, 240, 240);   // #f0f0f0
        c.btnBgPressed  = RGB(220, 220, 220);   // #dcdcdc
        c.btnBorder     = RGB(175, 175, 175);   // #afafaf
        c.btnText       = RGB(32, 32, 32);      // #202020
        c.accentBg      = RGB(14, 100, 215);    // #0e64d7
        c.accentText    = RGB(255, 255, 255);
        c.inputBg       = RGB(255, 255, 255);
        c.inputBorder   = RGB(180, 180, 180);
        c.statusBg      = RGB(230, 230, 230);   // #e6e6e6
        c.statusBorder  = RGB(195, 195, 195);   // #c3c3c3
        c.statusText    = RGB(35, 35, 35);      // #232323
        c.linkColor     = RGB(30, 100, 200);    // #1e64c8
    }

    if (sAIUITheme) {
        AIUIThemeColor colBg;
        if (sAIUITheme->GetUIThemeColor(kAIUIThemeSelectorPanel, kAIUIComponentColorBackground, colBg) == kNoErr) {
            c.panelBg = RGB((int)(colBg.red * 255.0f + 0.5f),
                            (int)(colBg.green * 255.0f + 0.5f),
                            (int)(colBg.blue * 255.0f + 0.5f));
        }
        AIUIThemeColor colText;
        if (sAIUITheme->GetUIThemeColor(kAIUIThemeSelectorPanel, kAIUIComponentColorText, colText) == kNoErr) {
            c.textPrimary = RGB((int)(colText.red * 255.0f + 0.5f),
                                (int)(colText.green * 255.0f + 0.5f),
                                (int)(colText.blue * 255.0f + 0.5f));
        }
    }

    return c;
}

static LRESULT CALLBACK CustomButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_MOUSEMOVE: {
            bool isHover = GetPropA(hwnd, "MK_Hover") != NULL;
            if (!isHover) {
                SetPropA(hwnd, "MK_Hover", (HANDLE)1);
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
                TrackMouseEvent(&tme);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
        case WM_MOUSELEAVE: {
            RemovePropA(hwnd, "MK_Hover");
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);

            bool isHover = GetPropA(hwnd, "MK_Hover") != NULL;
            bool isDown = (SendMessage(hwnd, BM_GETSTATE, 0, 0) & BST_PUSHED) != 0;
            bool isPrimary = (GetPropA(hwnd, "MK_Primary") != NULL);
            bool isSelectedPreset = (GetPropA(hwnd, "MK_Selected") != NULL);

            UIThemeColors tc = GetCurrentThemeColors();

            COLORREF bgCol = tc.btnBg;
            COLORREF borderCol = tc.btnBorder;
            COLORREF txtCol = tc.btnText;

            if (isPrimary || isSelectedPreset) {
                bgCol = isDown ? RGB(10, 85, 180) : (isHover ? RGB(25, 130, 245) : tc.accentBg);
                borderCol = bgCol;
                txtCol = tc.accentText;
            } else {
                if (isDown) {
                    bgCol = tc.btnBgPressed;
                } else if (isHover) {
                    bgCol = tc.btnBgHover;
                    borderCol = tc.textSecondary;
                }
            }

            HBRUSH hBr = CreateSolidBrush(bgCol);
            HPEN hPen = CreatePen(PS_SOLID, 1, borderCol);
            HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, hBr);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

            float scale = GetPanelScale(hwnd);
            int corner = (std::max)(2, (int)(4.0f * scale + 0.5f));
            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, corner, corner);

            SelectObject(hdc, hOldBr);
            SelectObject(hdc, hOldPen);
            DeleteObject(hBr);
            DeleteObject(hPen);

            char text[128] = { 0 };
            GetWindowTextA(hwnd, text, sizeof(text));

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, txtCol);
            HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
            HFONT hOldFont = NULL;
            if (hFont) hOldFont = (HFONT)SelectObject(hdc, hFont);

            DrawTextA(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            if (hOldFont) SelectObject(hdc, hOldFont);
            EndPaint(hwnd, &ps);
            return 0;
        }
    }

    WNDPROC oldProc = (WNDPROC)GetPropA(hwnd, "MK_OldProc");
    if (oldProc) {
        return CallWindowProcA(oldProc, hwnd, msg, wParam, lParam);
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void SubclassButton(HWND hBtn, bool isPrimary = false) {
    if (!hBtn) return;
    WNDPROC old = (WNDPROC)SetWindowLongPtrA(hBtn, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
    SetPropA(hBtn, "MK_OldProc", (HANDLE)old);
    if (isPrimary) SetPropA(hBtn, "MK_Primary", (HANDLE)1);
}

typedef HRESULT (WINAPI *PFN_SetWindowTheme)(HWND, LPCWSTR, LPCWSTR);
static void DisableUxTheme(HWND hWnd) {
    if (!hWnd) return;
    static HMODULE hUxTheme = LoadLibraryA("uxtheme.dll");
    if (hUxTheme) {
        static PFN_SetWindowTheme pfn = (PFN_SetWindowTheme)GetProcAddress(hUxTheme, "SetWindowTheme");
        if (pfn) pfn(hWnd, L"", L"");
    }
}

static LRESULT CALLBACK CustomEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WNDPROC oldProc = (WNDPROC)GetPropA(hwnd, "MK_OldEditProc");
    if (msg == WM_NCPAINT) {
        HDC hdc = GetWindowDC(hwnd);
        if (hdc) {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            OffsetRect(&rc, -rc.left, -rc.top);
            UIThemeColors tc = GetCurrentThemeColors();
            HPEN hPen = CreatePen(PS_SOLID, 1, tc.inputBorder);
            HPEN oldPen = (HPEN)SelectObject(hdc, hPen);
            HBRUSH oldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBr);
            DeleteObject(hPen);
            ReleaseDC(hwnd, hdc);
        }
        return 0;
    }
    if (msg == WM_ERASEBKGND) {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        UIThemeColors tc = GetCurrentThemeColors();
        HBRUSH hBr = CreateSolidBrush(tc.inputBg);
        FillRect(hdc, &rc, hBr);
        DeleteObject(hBr);
        return 1;
    }
    if (oldProc) return CallWindowProcA(oldProc, hwnd, msg, wParam, lParam);
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void SubclassEdit(HWND hEdit) {
    if (!hEdit) return;
    DisableUxTheme(hEdit);
    WNDPROC old = (WNDPROC)SetWindowLongPtrA(hEdit, GWLP_WNDPROC, (LONG_PTR)CustomEditProc);
    SetPropA(hEdit, "MK_OldEditProc", (HANDLE)old);
}

static LRESULT CALLBACK CustomComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WNDPROC oldProc = (WNDPROC)GetPropA(hwnd, "MK_OldComboProc");
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (hdc) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            UIThemeColors tc = GetCurrentThemeColors();
            float scale = GetPanelScale(hwnd);
            auto S = [scale](int v) -> int { return (int)(v * scale + 0.5f); };

            // Background
            HBRUSH hBr = CreateSolidBrush(tc.inputBg);
            FillRect(hdc, &rc, hBr);
            DeleteObject(hBr);

            // 1px Border
            HPEN hPen = CreatePen(PS_SOLID, 1, tc.inputBorder);
            HPEN oldPen = (HPEN)SelectObject(hdc, hPen);
            HBRUSH oldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            int corner = (std::max)(1, S(2));
            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, corner, corner);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBr);
            DeleteObject(hPen);

            // Selected item text
            int curSel = (int)SendMessageA(hwnd, CB_GETCURSEL, 0, 0);
            if (curSel >= 0) {
                char text[256] = { 0 };
                SendMessageA(hwnd, CB_GETLBTEXT, curSel, (LPARAM)text);
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, tc.textPrimary);
                RECT textRc = rc;
                textRc.left += S(5);
                textRc.right -= S(14);
                HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
                HFONT oldFont = NULL;
                if (hFont) oldFont = (HFONT)SelectObject(hdc, hFont);
                DrawTextA(hdc, text, -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                if (oldFont) SelectObject(hdc, oldFont);
            }

            // Vector arrow
            int arrowWing = (std::max)(2, S(3));
            int arrowPenW = (std::max)(1, S(1));
            int ax = rc.right - S(8);
            int ay = rc.top + (rc.bottom - rc.top) / 2 - S(1);
            HPEN hArrowPen = CreatePen(PS_SOLID, arrowPenW, tc.textSecondary);
            oldPen = (HPEN)SelectObject(hdc, hArrowPen);
            MoveToEx(hdc, ax - arrowWing, ay, NULL);
            LineTo(hdc, ax, ay + arrowWing);
            LineTo(hdc, ax + arrowWing, ay);
            SelectObject(hdc, oldPen);
            DeleteObject(hArrowPen);

            EndPaint(hwnd, &ps);
        }
        return 0;
    }
    if (msg == WM_ERASEBKGND) {
        return 1;
    }
    if (oldProc) return CallWindowProcA(oldProc, hwnd, msg, wParam, lParam);
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void SubclassCombo(HWND hCombo) {
    if (!hCombo) return;
    DisableUxTheme(hCombo);
    WNDPROC old = (WNDPROC)SetWindowLongPtrA(hCombo, GWLP_WNDPROC, (LONG_PTR)CustomComboProc);
    SetPropA(hCombo, "MK_OldComboProc", (HANDLE)old);
}

static void RelayoutPanel(HWND hwnd) {
    if (!hwnd) return;
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    if (width <= 0) return;

    float scale = GetPanelScale(hwnd);
    EnsureFontsForScale(hwnd, scale);

    auto S = [scale](int v) -> int {
        return (int)(v * scale + 0.5f);
    };

    int pad = S(8);
    int contentW = (std::max)(S(100), width - pad * 2);

    // Row 1: Header
    HWND hTitle = GetDlgItem(hwnd, IDC_HEADER_TITLE);
    HWND hVer = GetDlgItem(hwnd, IDC_HEADER_VER);
    int verW = S(38);
    int titleW = (std::max)(S(50), contentW - verW);
    if (hTitle) SetWindowPos(hTitle, NULL, pad, S(6), titleW, S(18), SWP_NOZORDER);
    if (hVer)   SetWindowPos(hVer, NULL, pad + contentW - verW, S(7), verW, S(16), SWP_NOZORDER);

    // Row 2: Tab Navigation Bar (Marks, Pantones, Settings)
    HWND hTab0 = GetDlgItem(hwnd, IDC_TAB_MARKS);
    HWND hTab1 = GetDlgItem(hwnd, IDC_TAB_PANTONES);
    HWND hTab2 = GetDlgItem(hwnd, IDC_TAB_SETTINGS);
    int tabH = S(22);
    int tabW = (contentW - 2 * S(2)) / 3;
    if (hTab0) SetWindowPos(hTab0, NULL, pad, S(26), tabW, tabH, SWP_NOZORDER);
    if (hTab1) SetWindowPos(hTab1, NULL, pad + tabW + S(2), S(26), tabW, tabH, SWP_NOZORDER);
    if (hTab2) SetWindowPos(hTab2, NULL, pad + (tabW + S(2)) * 2, S(26), contentW - (tabW + S(2)) * 2, tabH, SWP_NOZORDER);

    // ==========================================
    // TAB 0: MARKS
    // ==========================================
    int btnH = S(20);
    int btnW = (contentW - 7 * S(2)) / 8;
    if (btnW < S(10)) btnW = S(10);
    for (int i = 0; i < 8; ++i) {
        HWND hBtn = GetDlgItem(hwnd, IDC_PRESET_BTN_BASE + i);
        if (hBtn) SetWindowPos(hBtn, NULL, pad + i * (btnW + S(2)), S(52), btnW, btnH, SWP_NOZORDER);
    }

    HWND hEdPName = GetDlgItem(hwnd, IDC_ED_PRESET_NAME);
    HWND hSave = GetDlgItem(hwnd, IDC_BTN_PRESET_SAVE);
    int saveW = S(44);
    int pNameW = (std::max)(S(35), contentW - saveW - S(2));
    if (hEdPName) SetWindowPos(hEdPName, NULL, pad, S(74), pNameW, btnH, SWP_NOZORDER);
    if (hSave)    SetWindowPos(hSave, NULL, pad + pNameW + S(2), S(74), saveW, btnH, SWP_NOZORDER);

    // Directions Grid
    int cx = width / 2;
    int boxSize = S(12);
    int chkRowH = S(18);
    auto moveCheck = [&](int cid, int kx, int ky) {
        HWND hChk = GetDlgItem(hwnd, cid);
        HWND hLbl = GetDlgItem(hwnd, cid + 1000);
        if (hChk) SetWindowPos(hChk, NULL, kx, ky + (chkRowH - boxSize) / 2, boxSize, boxSize, SWP_NOZORDER);
        if (hLbl) SetWindowPos(hLbl, NULL, kx + boxSize + S(3), ky, S(22), chkRowH, SWP_NOZORDER);
    };

    int colL = cx - S(48);
    int colC = cx - S(14);
    int colR = cx + S(20);
    int colSideL = cx - S(68);
    int colSideR = cx + S(40);

    moveCheck(IDC_CHK_TL, colL, S(98));
    moveCheck(IDC_CHK_TC, colC, S(98));
    moveCheck(IDC_CHK_TR, colR, S(98));

    moveCheck(IDC_CHK_LT, colSideL, S(118));
    moveCheck(IDC_CHK_RT, colSideR, S(118));

    moveCheck(IDC_CHK_LC, colSideL, S(136));
    moveCheck(IDC_CHK_RC, colSideR, S(136));

    moveCheck(IDC_CHK_LB, colSideL, S(154));
    moveCheck(IDC_CHK_RB, colSideR, S(154));

    moveCheck(IDC_CHK_BL, colL, S(174));
    moveCheck(IDC_CHK_BC, colC, S(174));
    moveCheck(IDC_CHK_BR, colR, S(174));

    // Direction Buttons (All, Corners, Clear)
    HWND hDirAll = GetDlgItem(hwnd, IDC_BTN_DIR_ALL);
    HWND hDirCor = GetDlgItem(hwnd, IDC_BTN_DIR_CORNERS);
    HWND hDirClr = GetDlgItem(hwnd, IDC_BTN_DIR_NONE);
    int dirBtnW = (contentW - S(4)) / 3;
    if (hDirAll) SetWindowPos(hDirAll, NULL, pad, S(196), dirBtnW, btnH, SWP_NOZORDER);
    if (hDirCor) SetWindowPos(hDirCor, NULL, pad + dirBtnW + S(2), S(196), dirBtnW, btnH, SWP_NOZORDER);
    if (hDirClr) SetWindowPos(hDirClr, NULL, pad + (dirBtnW + S(2)) * 2, S(196), contentW - (dirBtnW + S(2)) * 2, btnH, SWP_NOZORDER);

    // Inputs (Length, Offset, Bleed, Stroke)
    int editH = S(20);
    int dropH = S(140);
    int lblW = (std::min)(S(78), (std::max)(S(52), contentW / 3));
    int remW = contentW - lblW - S(4);
    int comboUnitW = S(46);
    int edW = (remW - comboUnitW - S(4)) / 2;
    if (edW < S(18)) edW = S(18);

    int edX1 = pad + lblW + S(2);
    int edX2 = edX1 + edW + S(2);
    int cmbX = edX2 + edW + S(2);

    auto updateComboHeight = [&](HWND hCmb) {
        if (hCmb) {
            SendMessageA(hCmb, CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)editH);
            SendMessageA(hCmb, CB_SETITEMHEIGHT, (WPARAM)0,  (LPARAM)S(18));
        }
    };

    HWND hLblLen = GetDlgItem(hwnd, IDC_LBL_LENGTH);
    HWND hEdLenV = GetDlgItem(hwnd, IDC_ED_LEN_VER);
    HWND hEdLenH = GetDlgItem(hwnd, IDC_ED_LEN_HOR);
    HWND hCmbUnits = GetDlgItem(hwnd, IDC_CMB_UNITS);
    if (hLblLen) SetWindowPos(hLblLen, NULL, pad, S(223), lblW, S(18), SWP_NOZORDER);
    if (hEdLenV) SetWindowPos(hEdLenV, NULL, edX1, S(222), edW, editH, SWP_NOZORDER);
    if (hEdLenH) SetWindowPos(hEdLenH, NULL, edX2, S(222), edW, editH, SWP_NOZORDER);
    if (hCmbUnits) {
        SetWindowPos(hCmbUnits, NULL, cmbX, S(222), comboUnitW, dropH, SWP_NOZORDER);
        updateComboHeight(hCmbUnits);
    }

    HWND hLblOff = GetDlgItem(hwnd, IDC_LBL_OFFSET);
    HWND hEdOffV = GetDlgItem(hwnd, IDC_ED_OFF_VER);
    HWND hEdOffH = GetDlgItem(hwnd, IDC_ED_OFF_HOR);
    if (hLblOff) SetWindowPos(hLblOff, NULL, pad, S(247), lblW, S(18), SWP_NOZORDER);
    if (hEdOffV) SetWindowPos(hEdOffV, NULL, edX1, S(246), edW, editH, SWP_NOZORDER);
    if (hEdOffH) SetWindowPos(hEdOffH, NULL, edX2, S(246), edW, editH, SWP_NOZORDER);

    HWND hLblBld = GetDlgItem(hwnd, IDC_LBL_BLEED);
    HWND hEdBldV = GetDlgItem(hwnd, IDC_ED_BLEED_VER);
    HWND hEdBldH = GetDlgItem(hwnd, IDC_ED_BLEED_HOR);
    if (hLblBld) SetWindowPos(hLblBld, NULL, pad, S(271), lblW, S(18), SWP_NOZORDER);
    if (hEdBldV) SetWindowPos(hEdBldV, NULL, edX1, S(270), edW, editH, SWP_NOZORDER);
    if (hEdBldH) SetWindowPos(hEdBldH, NULL, edX2, S(270), edW, editH, SWP_NOZORDER);

    HWND hLblStr = GetDlgItem(hwnd, IDC_LBL_STROKE);
    HWND hEdStroke = GetDlgItem(hwnd, IDC_ED_STROKE);
    HWND hCmbStrokeUnits = GetDlgItem(hwnd, IDC_CMB_STROKE_UNITS);
    if (hLblStr) SetWindowPos(hLblStr, NULL, pad, S(295), lblW, S(18), SWP_NOZORDER);
    if (hEdStroke) SetWindowPos(hEdStroke, NULL, edX1, S(294), edW, editH, SWP_NOZORDER);
    if (hCmbStrokeUnits) {
        SetWindowPos(hCmbStrokeUnits, NULL, edX2, S(294), comboUnitW + edW + S(2), dropH, SWP_NOZORDER);
        updateComboHeight(hCmbStrokeUnits);
    }

    // Checkboxes (Marks Tab)
    auto moveCheckFull = [&](int cid, int ky) {
        HWND hChk = GetDlgItem(hwnd, cid);
        HWND hLbl = GetDlgItem(hwnd, cid + 1000);
        int chkH = S(18);
        if (hChk) SetWindowPos(hChk, NULL, pad, ky + (chkH - boxSize) / 2, boxSize, boxSize, SWP_NOZORDER);
        if (hLbl) SetWindowPos(hLbl, NULL, pad + boxSize + S(5), ky, (std::max)(S(50), contentW - boxSize - S(5)), chkH, SWP_NOZORDER);
    };
    moveCheckFull(IDC_CHK_PERIMETER_ONLY, S(320));
    moveCheckFull(IDC_CHK_WHITE_CONTOUR, S(342));
    moveCheckFull(IDC_CHK_REG_MARKS, S(364));
    moveCheckFull(IDC_CHK_BARS_LONG, S(386));
    moveCheckFull(IDC_CHK_BARS_SHORT, S(408));

    // Combos (Target, Bounds, Grouping)
    int comboLblW = S(54);
    int comboBodyW = (std::max)(S(50), contentW - comboLblW - S(3));
    HWND hLblTarget = GetDlgItem(hwnd, IDC_LBL_TARGET);
    HWND hCmbTarget = GetDlgItem(hwnd, IDC_CMB_TARGET);
    HWND hLblBounds = GetDlgItem(hwnd, IDC_LBL_BOUNDS);
    HWND hCmbBounds = GetDlgItem(hwnd, IDC_CMB_BOUNDS);
    HWND hLblGroup  = GetDlgItem(hwnd, IDC_LBL_GROUP);
    HWND hCmbGroup  = GetDlgItem(hwnd, IDC_CMB_GROUP);

    if (hLblTarget) SetWindowPos(hLblTarget, NULL, pad, S(433), comboLblW, S(18), SWP_NOZORDER);
    if (hCmbTarget) {
        SetWindowPos(hCmbTarget, NULL, pad + comboLblW + S(2), S(432), comboBodyW, dropH, SWP_NOZORDER);
        updateComboHeight(hCmbTarget);
    }
    if (hLblBounds) SetWindowPos(hLblBounds, NULL, pad, S(457), comboLblW, S(18), SWP_NOZORDER);
    if (hCmbBounds) {
        SetWindowPos(hCmbBounds, NULL, pad + comboLblW + S(2), S(456), comboBodyW, dropH, SWP_NOZORDER);
        updateComboHeight(hCmbBounds);
    }
    if (hLblGroup)  SetWindowPos(hLblGroup,  NULL, pad, S(481), comboLblW, S(18), SWP_NOZORDER);
    if (hCmbGroup) {
        SetWindowPos(hCmbGroup, NULL, pad + comboLblW + S(2), S(480), comboBodyW, dropH, SWP_NOZORDER);
        updateComboHeight(hCmbGroup);
    }

    HWND hBtnCreate = GetDlgItem(hwnd, IDC_BTN_CREATE);
    int createH = S(28);
    if (hBtnCreate) SetWindowPos(hBtnCreate, NULL, pad, S(508), contentW, createH, SWP_NOZORDER);

    // ==========================================
    // TAB 1: PANTONES
    // ==========================================
    HWND hLblProc = GetDlgItem(hwnd, IDC_LBL_PROCESS_INKS);
    if (hLblProc) SetWindowPos(hLblProc, NULL, pad, S(52), contentW, S(18), SWP_NOZORDER);

    int inkW = (contentW - 3 * S(2)) / 4;
    auto moveInkCheck = [&](int cid, int kx, int ky) {
        HWND hChk = GetDlgItem(hwnd, cid);
        HWND hLbl = GetDlgItem(hwnd, cid + 1000);
        int chkH = S(18);
        if (hChk) SetWindowPos(hChk, NULL, kx, ky + (chkH - boxSize) / 2, boxSize, boxSize, SWP_NOZORDER);
        if (hLbl) SetWindowPos(hLbl, NULL, kx + boxSize + S(3), ky, (std::max)(S(20), inkW - boxSize - S(3)), chkH, SWP_NOZORDER);
    };
    moveInkCheck(IDC_CHK_INK_C, pad + 0 * (inkW + S(2)), S(74));
    moveInkCheck(IDC_CHK_INK_M, pad + 1 * (inkW + S(2)), S(74));
    moveInkCheck(IDC_CHK_INK_Y, pad + 2 * (inkW + S(2)), S(74));
    moveInkCheck(IDC_CHK_INK_K, pad + 3 * (inkW + S(2)), S(74));

    HWND hLblSpots = GetDlgItem(hwnd, IDC_LBL_SPOTS_HEADER);
    if (hLblSpots) SetWindowPos(hLblSpots, NULL, pad, S(98), contentW, S(18), SWP_NOZORDER);

    int actW = (contentW - 3 * S(2)) / 4;
    HWND hBtnScan   = GetDlgItem(hwnd, IDC_BTN_SCAN_SPOTS);
    HWND hBtnAll    = GetDlgItem(hwnd, IDC_BTN_SPOTS_ALL);
    HWND hBtnNone   = GetDlgItem(hwnd, IDC_BTN_SPOTS_NONE);
    HWND hBtnFilter = GetDlgItem(hwnd, IDC_BTN_SPOTS_FILTER);
    if (hBtnScan)   SetWindowPos(hBtnScan,   NULL, pad + 0 * (actW + S(2)), S(118), actW, S(22), SWP_NOZORDER);
    if (hBtnAll)    SetWindowPos(hBtnAll,    NULL, pad + 1 * (actW + S(2)), S(118), actW, S(22), SWP_NOZORDER);
    if (hBtnNone)   SetWindowPos(hBtnNone,   NULL, pad + 2 * (actW + S(2)), S(118), actW, S(22), SWP_NOZORDER);
    if (hBtnFilter) SetWindowPos(hBtnFilter, NULL, pad + 3 * (actW + S(2)), S(118), contentW - (actW + S(2)) * 3, S(22), SWP_NOZORDER);

    HWND hSpotsCount = GetDlgItem(hwnd, IDC_STATIC_SPOTS_COUNT);
    if (hSpotsCount) SetWindowPos(hSpotsCount, NULL, pad, S(144), contentW, S(16), SWP_NOZORDER);

    HWND hSpotsList = GetDlgItem(hwnd, IDC_LIST_SPOTS);
    int listTop = S(164);
    int listH = (std::max)(S(100), S(504) - listTop);
    if (hSpotsList) {
        SetWindowPos(hSpotsList, NULL, pad, listTop, contentW, listH, SWP_NOZORDER);
        ListView_SetColumnWidth(hSpotsList, 0, (std::max)(S(120), contentW - S(20)));
    }

    HWND hBtnPantonesApply = GetDlgItem(hwnd, IDC_BTN_PANTONES_APPLY);
    if (hBtnPantonesApply) SetWindowPos(hBtnPantonesApply, NULL, pad, S(508), contentW, createH, SWP_NOZORDER);

    // ==========================================
    // TAB 2: SETTINGS
    // ==========================================
    HWND hLblSetBar = GetDlgItem(hwnd, IDC_LBL_SET_BAR_HEADER);
    if (hLblSetBar) SetWindowPos(hLblSetBar, NULL, pad, S(52), contentW, S(18), SWP_NOZORDER);

    int sLblW = (std::min)(S(88), (std::max)(S(60), contentW / 3));
    int sBodyW = (std::max)(S(40), contentW - sLblW - S(2));

    HWND hLblSetBMode = GetDlgItem(hwnd, IDC_LBL_SET_BAR_MODE);
    HWND hCmbSetBMode = GetDlgItem(hwnd, IDC_CMB_BAR_MODE);
    if (hLblSetBMode) SetWindowPos(hLblSetBMode, NULL, pad, S(72), sLblW, S(18), SWP_NOZORDER);
    if (hCmbSetBMode) {
        SetWindowPos(hCmbSetBMode, NULL, pad + sLblW + S(2), S(71), sBodyW, dropH, SWP_NOZORDER);
        updateComboHeight(hCmbSetBMode);
    }

    HWND hLblSetPatch = GetDlgItem(hwnd, IDC_LBL_SET_PATCH_SIZE);
    HWND hEdSetPatchW = GetDlgItem(hwnd, IDC_ED_SET_PATCH_W);
    HWND hEdSetPatchH = GetDlgItem(hwnd, IDC_ED_SET_PATCH_H);
    int sEdW = (sBodyW - S(4)) / 2;
    if (hLblSetPatch) SetWindowPos(hLblSetPatch, NULL, pad, S(96), sLblW, S(18), SWP_NOZORDER);
    if (hEdSetPatchW) SetWindowPos(hEdSetPatchW, NULL, pad + sLblW + S(2), S(95), sEdW, editH, SWP_NOZORDER);
    if (hEdSetPatchH) SetWindowPos(hEdSetPatchH, NULL, pad + sLblW + S(2) + sEdW + S(4), S(95), sBodyW - sEdW - S(4), editH, SWP_NOZORDER);

    HWND hLblSetBOff = GetDlgItem(hwnd, IDC_LBL_SET_BAR_OFFSET);
    HWND hEdSetBOff  = GetDlgItem(hwnd, IDC_ED_SET_BAR_OFFSET);
    if (hLblSetBOff) SetWindowPos(hLblSetBOff, NULL, pad, S(120), sLblW, S(18), SWP_NOZORDER);
    if (hEdSetBOff)  SetWindowPos(hEdSetBOff,  NULL, pad + sLblW + S(2), S(119), sBodyW, editH, SWP_NOZORDER);

    HWND hLblSetBLen = GetDlgItem(hwnd, IDC_LBL_SET_BAR_LEN);
    HWND hCmbSetBLen = GetDlgItem(hwnd, IDC_CMB_BAR_LEN);
    if (hLblSetBLen) SetWindowPos(hLblSetBLen, NULL, pad, S(144), sLblW, S(18), SWP_NOZORDER);
    if (hCmbSetBLen) {
        SetWindowPos(hCmbSetBLen, NULL, pad + sLblW + S(2), S(143), sBodyW, dropH, SWP_NOZORDER);
        updateComboHeight(hCmbSetBLen);
    }

    HWND hLblSetBOrd = GetDlgItem(hwnd, IDC_LBL_SET_BAR_ORDER);
    HWND hCmbSetBOrd = GetDlgItem(hwnd, IDC_CMB_BAR_ORDER);
    if (hLblSetBOrd) SetWindowPos(hLblSetBOrd, NULL, pad, S(168), sLblW, S(18), SWP_NOZORDER);
    if (hCmbSetBOrd) {
        SetWindowPos(hCmbSetBOrd, NULL, pad + sLblW + S(2), S(167), sBodyW, dropH, SWP_NOZORDER);
        updateComboHeight(hCmbSetBOrd);
    }

    // Reg Marks Section
    HWND hLblSetReg = GetDlgItem(hwnd, IDC_LBL_SET_REG_HEADER);
    if (hLblSetReg) SetWindowPos(hLblSetReg, NULL, pad, S(196), contentW, S(18), SWP_NOZORDER);

    HWND hLblSetRDim = GetDlgItem(hwnd, IDC_LBL_SET_REG_DIAM);
    HWND hEdSetRDim  = GetDlgItem(hwnd, IDC_ED_SET_REG_DIAM);
    HWND hEdSetRLen  = GetDlgItem(hwnd, IDC_ED_SET_REG_LEN);
    if (hLblSetRDim) SetWindowPos(hLblSetRDim, NULL, pad, S(216), sLblW, S(18), SWP_NOZORDER);
    if (hEdSetRDim)  SetWindowPos(hEdSetRDim,  NULL, pad + sLblW + S(2), S(215), sEdW, editH, SWP_NOZORDER);
    if (hEdSetRLen)  SetWindowPos(hEdSetRLen,  NULL, pad + sLblW + S(2) + sEdW + S(4), S(215), sBodyW - sEdW - S(4), editH, SWP_NOZORDER);

    HWND hLblSetRStr = GetDlgItem(hwnd, IDC_LBL_SET_REG_STROKE);
    HWND hEdSetRStr  = GetDlgItem(hwnd, IDC_ED_SET_REG_STROKE);
    HWND hCmbSetRStrUn = GetDlgItem(hwnd, IDC_CMB_SET_REG_STROKE_UN);
    if (hLblSetRStr) SetWindowPos(hLblSetRStr, NULL, pad, S(240), sLblW, S(18), SWP_NOZORDER);
    if (hEdSetRStr)  SetWindowPos(hEdSetRStr,  NULL, pad + sLblW + S(2), S(239), sEdW, editH, SWP_NOZORDER);
    if (hCmbSetRStrUn) {
        SetWindowPos(hCmbSetRStrUn, NULL, pad + sLblW + S(2) + sEdW + S(4), S(239), sBodyW - sEdW - S(4), dropH, SWP_NOZORDER);
        updateComboHeight(hCmbSetRStrUn);
    }

    HWND hLblSetROff = GetDlgItem(hwnd, IDC_LBL_SET_REG_OFFSET);
    HWND hEdSetROffH = GetDlgItem(hwnd, IDC_ED_SET_REG_OFF_H);
    HWND hEdSetROffV = GetDlgItem(hwnd, IDC_ED_SET_REG_OFF_V);
    if (hLblSetROff) SetWindowPos(hLblSetROff, NULL, pad, S(264), sLblW, S(18), SWP_NOZORDER);
    if (hEdSetROffH) SetWindowPos(hEdSetROffH, NULL, pad + sLblW + S(2), S(263), sEdW, editH, SWP_NOZORDER);
    if (hEdSetROffV) SetWindowPos(hEdSetROffV, NULL, pad + sLblW + S(2) + sEdW + S(4), S(263), sBodyW - sEdW - S(4), editH, SWP_NOZORDER);

    HWND hLblSetRPos = GetDlgItem(hwnd, IDC_LBL_SET_REG_POS);
    HWND hCmbSetRPos = GetDlgItem(hwnd, IDC_CMB_SET_REG_POS);
    if (hLblSetRPos) SetWindowPos(hLblSetRPos, NULL, pad, S(288), sLblW, S(18), SWP_NOZORDER);
    if (hCmbSetRPos) {
        SetWindowPos(hCmbSetRPos, NULL, pad + sLblW + S(2), S(287), sBodyW, dropH, SWP_NOZORDER);
        updateComboHeight(hCmbSetRPos);
    }

    moveCheckFull(IDC_CHK_SET_REG_WHITE, S(312));

    HWND hLblSetROrd = GetDlgItem(hwnd, IDC_LBL_SET_REG_ORDER);
    HWND hCmbSetROrd = GetDlgItem(hwnd, IDC_CMB_SET_REG_ORDER);
    if (hLblSetROrd) SetWindowPos(hLblSetROrd, NULL, pad, S(334), sLblW, S(18), SWP_NOZORDER);
    if (hCmbSetROrd) {
        SetWindowPos(hCmbSetROrd, NULL, pad + sLblW + S(2), S(333), sBodyW, dropH, SWP_NOZORDER);
        updateComboHeight(hCmbSetROrd);
    }

    // Doc Info Section
    HWND hLblSetDoc = GetDlgItem(hwnd, IDC_LBL_SET_DOC_HEADER);
    if (hLblSetDoc) SetWindowPos(hLblSetDoc, NULL, pad, S(362), contentW, S(18), SWP_NOZORDER);
    moveCheckFull(IDC_CHK_SET_DOC_INFO, S(380));
    moveCheckFull(IDC_CHK_SET_DOC_TRANSLIT, S(400));

    // Hotkeys Section
    HWND hLblSetHk = GetDlgItem(hwnd, IDC_LBL_SET_HK_HEADER);
    if (hLblSetHk) SetWindowPos(hLblSetHk, NULL, pad, S(424), contentW, S(18), SWP_NOZORDER);
    HWND hSetHkInfo = GetDlgItem(hwnd, IDC_STATIC_SET_HK_INFO);
    if (hSetHkInfo) SetWindowPos(hSetHkInfo, NULL, pad, S(442), contentW, S(16), SWP_NOZORDER);

    int halfBtnW = (contentW - S(4)) / 2;
    HWND hBtnHotkeys = GetDlgItem(hwnd, IDC_BTN_SET_HOTKEYS);
    HWND hBtnSaveDef = GetDlgItem(hwnd, IDC_BTN_SET_SAVE_DEFAULTS);
    if (hBtnHotkeys) SetWindowPos(hBtnHotkeys, NULL, pad, S(464), halfBtnW, S(24), SWP_NOZORDER);
    if (hBtnSaveDef) SetWindowPos(hBtnSaveDef, NULL, pad + halfBtnW + S(4), S(464), contentW - halfBtnW - S(4), S(24), SWP_NOZORDER);

    HWND hBtnSettingsApply = GetDlgItem(hwnd, IDC_BTN_SETTINGS_APPLY);
    if (hBtnSettingsApply) SetWindowPos(hBtnSettingsApply, NULL, pad, S(508), contentW, createH, SWP_NOZORDER);

    // ==========================================
    // COMMON FOOTER
    // ==========================================
    HWND hStatus = GetDlgItem(hwnd, IDC_STATIC_STATUS);
    int statusH = S(18);
    int yStatus = S(508) + createH + S(4);
    if (hStatus) SetWindowPos(hStatus, NULL, pad, yStatus, contentW, statusH, SWP_NOZORDER);

    HWND hLinkAuthor = GetDlgItem(hwnd, IDC_STATIC_LINK_AUTHOR);
    HWND hLinkTools  = GetDlgItem(hwnd, IDC_STATIC_LINK_TOOLS);
    int linkH = S(15);
    int yLink1 = yStatus + statusH + S(4);
    int yLink2 = yLink1 + linkH + S(2);
    if (hLinkAuthor) SetWindowPos(hLinkAuthor, NULL, pad, yLink1, contentW, linkH, SWP_NOZORDER);
    if (hLinkTools)  SetWindowPos(hLinkTools,  NULL, pad, yLink2, contentW, linkH, SWP_NOZORDER);
}

static void AIAPI PanelVisibilityChangedNotifyProc(AIPanelRef inPanel, AIBoolean isVisible) {
    MakeCropMarksAIPlugin::GetInstance().OnVisibilityChanged(isVisible != 0);
}

static void AIAPI PanelSizeChangedNotifyProc(AIPanelRef inPanel) {
    HWND hDlg = MakeCropMarksAIPlugin::GetInstance().GetPanelHWND();
    if (hDlg) {
        RelayoutPanel(hDlg);
        InvalidateRect(hDlg, NULL, TRUE);
    }
}

static void AIAPI PanelClosedNotifyProc(AIPanelRef inPanel) {
    MakeCropMarksAIPlugin::GetInstance().OnVisibilityChanged(false);
}

static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == PM_REMOVE) {
        if (ConfigManager::GetInstance().IsDialogActive() || ConfigManager::GetInstance().IsPluginDisabled()) {
            return CallNextHookEx(g_hKeyHook, nCode, wParam, lParam);
        }
        MSG* pMsg = reinterpret_cast<MSG*>(lParam);
        if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) {
            HWND hPanel = MakeCropMarksAIPlugin::GetInstance().GetPanelHWND();
            if (hPanel && (pMsg->hwnd == hPanel || IsChild(hPanel, pMsg->hwnd))) {
                if (pMsg->wParam == VK_TAB) {
                    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                    HWND hCurrFocus = GetFocus();
                    HWND hNext = GetNextDlgTabItem(hPanel, hCurrFocus, shift ? TRUE : FALSE);
                    if (hNext && hNext != hCurrFocus) {
                        SetFocus(hNext);
                        char clsName[32] = { 0 };
                        GetClassNameA(hNext, clsName, sizeof(clsName));
                        if (_stricmp(clsName, "Edit") == 0) {
                            SendMessageA(hNext, EM_SETSEL, 0, -1);
                        }
                    }
                    pMsg->message = WM_NULL;
                    return 1;
                } else if (pMsg->wParam == VK_RETURN) {
                    char clsName[32] = { 0 };
                    GetClassNameA(pMsg->hwnd, clsName, sizeof(clsName));
                    if (_stricmp(clsName, "Edit") == 0) {
                        if (GetDlgCtrlID(pMsg->hwnd) == IDC_ED_PRESET_NAME) {
                            SendMessage(hPanel, WM_COMMAND, MAKEWPARAM(IDC_BTN_PRESET_SAVE, BN_CLICKED), (LPARAM)GetDlgItem(hPanel, IDC_BTN_PRESET_SAVE));
                            pMsg->message = WM_NULL;
                            return 1;
                        } else {
                            MakeCropMarksAIPlugin::GetInstance().ExecuteCreateMarks();
                            pMsg->message = WM_NULL;
                            return 1;
                        }
                    }
                }
            }

            char cls[32] = { 0 };
            GetClassNameA(pMsg->hwnd, cls, sizeof(cls));
            bool isEdit = (_stricmp(cls, "Edit") == 0 || _stricmp(cls, "RichEdit") == 0);

            if (!isEdit) {
                UINT vk = (UINT)pMsg->wParam;
                UINT mod = 0;
                if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) mod |= MOD_CONTROL;
                if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0)    mod |= MOD_ALT;
                if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)   mod |= MOD_SHIFT;

                auto& cfg = ConfigManager::GetInstance();
                if (cfg.MatchHotkey(cfg.GetOpenUiHotkey(), vk, mod)) {
                    SPPluginRef pluginRef = MakeCropMarksAIPlugin::GetInstance().GetPluginRef();
                    AppContext appContext(pluginRef);
                    PluginLog("[HotKey Hook] Open UI Hotkey triggered -> TogglePanel()");
                    MakeCropMarksAIPlugin::GetInstance().TogglePanel();
                    pMsg->message = WM_NULL;
                    return 1;
                } else if (cfg.MatchHotkey(cfg.GetQuickApplyHotkey(), vk, mod)) {
                    SPPluginRef pluginRef = MakeCropMarksAIPlugin::GetInstance().GetPluginRef();
                    AppContext appContext(pluginRef);
                    PluginLog("[HotKey Hook] Quick Apply Hotkey triggered -> ExecuteQuickApply()");
                    MakeCropMarksAIPlugin::GetInstance().ExecuteQuickApply();
                    pMsg->message = WM_NULL;
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(g_hKeyHook, nCode, wParam, lParam);
}

static LRESULT CALLBACK MakeCropMarksAIPanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE: {
            RelayoutPanel(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }

        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hwnd, &rc);
            UIThemeColors tc = GetCurrentThemeColors();
            HBRUSH hBr = CreateSolidBrush(tc.panelBg);
            FillRect(hdc, &rc, hBr);
            DeleteObject(hBr);
            return 1;
        }

        case WM_MEASUREITEM: {
            LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
            if (lpmis && lpmis->CtlType == ODT_COMBOBOX) {
                float scale = GetPanelScale(hwnd);
                if (lpmis->itemID == (UINT)-1) {
                    lpmis->itemHeight = (UINT)(20.0f * scale + 0.5f);
                } else {
                    lpmis->itemHeight = (UINT)(18.0f * scale + 0.5f);
                }
                return TRUE;
            }
            break;
        }

        case WM_DPICHANGED: {
            RelayoutPanel(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }

        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
            if (lpdis && lpdis->CtlType == ODT_COMBOBOX) {
                UIThemeColors tc = GetCurrentThemeColors();
                HDC hdc = lpdis->hDC;
                RECT rc = lpdis->rcItem;
                float scale = GetPanelScale(hwnd);
                auto S = [scale](int v) -> int { return (int)(v * scale + 0.5f); };

                bool isSelected = (lpdis->itemState & ODS_SELECTED) != 0;
                bool isClosed = (lpdis->itemID == (UINT)-1 || (lpdis->itemState & ODS_COMBOBOXEDIT));

                COLORREF bg = tc.inputBg;
                COLORREF txtCol = tc.textPrimary;
                if (isSelected && !isClosed) {
                    bg = tc.accentBg;
                    txtCol = tc.accentText;
                }

                HBRUSH hBr = CreateSolidBrush(bg);
                FillRect(hdc, &rc, hBr);
                DeleteObject(hBr);

                int itemIdx = lpdis->itemID;
                if (itemIdx < 0 && isClosed) {
                    itemIdx = (int)SendMessageA(lpdis->hwndItem, CB_GETCURSEL, 0, 0);
                }

                if (itemIdx >= 0) {
                    char text[256] = { 0 };
                    SendMessageA(lpdis->hwndItem, CB_GETLBTEXT, itemIdx, (LPARAM)text);
                    SetBkMode(hdc, TRANSPARENT);
                    SetTextColor(hdc, txtCol);
                    RECT textRc = rc;
                    textRc.left += S(5);
                    textRc.right -= S(14);
                    HFONT hFont = (HFONT)SendMessage(lpdis->hwndItem, WM_GETFONT, 0, 0);
                    HFONT oldFont = NULL;
                    if (hFont) oldFont = (HFONT)SelectObject(hdc, hFont);
                    DrawTextA(hdc, text, -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                    if (oldFont) SelectObject(hdc, oldFont);
                }

                if (isClosed) {
                    HPEN hBorderPen = CreatePen(PS_SOLID, 1, tc.inputBorder);
                    HPEN oldPen = (HPEN)SelectObject(hdc, hBorderPen);
                    HBRUSH oldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                    int corner = (std::max)(1, S(2));
                    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, corner, corner);
                    SelectObject(hdc, oldPen);
                    SelectObject(hdc, oldBr);
                    DeleteObject(hBorderPen);

                    int arrowWing = (std::max)(2, S(3));
                    int arrowPenW = (std::max)(1, S(1));
                    int ax = rc.right - S(8);
                    int ay = rc.top + (rc.bottom - rc.top) / 2 - S(1);
                    HPEN hArrowPen = CreatePen(PS_SOLID, arrowPenW, tc.textSecondary);
                    oldPen = (HPEN)SelectObject(hdc, hArrowPen);
                    MoveToEx(hdc, ax - arrowWing, ay, NULL);
                    LineTo(hdc, ax, ay + arrowWing);
                    LineTo(hdc, ax + arrowWing, ay);
                    SelectObject(hdc, oldPen);
                    DeleteObject(hArrowPen);
                }

                return TRUE;
            }
            break;
        }

        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                HWND hChild = ChildWindowFromPoint(hwnd, pt);
                if (hChild) {
                    int cid = GetDlgCtrlID(hChild);
                    if (cid == IDC_STATIC_LINK_AUTHOR || cid == IDC_STATIC_LINK_TOOLS) {
                        SetCursor(LoadCursor(NULL, IDC_HAND));
                        return TRUE;
                    }
                }
            }
            break;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            HWND hChild = (HWND)lParam;
            UIThemeColors tc = GetCurrentThemeColors();

            char cls[32] = { 0 };
            GetClassNameA(hChild, cls, sizeof(cls));
            if (_stricmp(cls, "Edit") == 0) {
                SetBkColor(hdcStatic, tc.inputBg);
                SetTextColor(hdcStatic, tc.textPrimary);
                if (g_hInputBgBrush) DeleteObject(g_hInputBgBrush);
                g_hInputBgBrush = CreateSolidBrush(tc.inputBg);
                return (LRESULT)g_hInputBgBrush;
            }

            SetBkMode(hdcStatic, TRANSPARENT);
            int id = GetDlgCtrlID(hChild);
            if (id == IDC_STATIC_STATUS) {
                SetTextColor(hdcStatic, tc.textSecondary);
            } else if (id == IDC_HEADER_TITLE) {
                SetTextColor(hdcStatic, tc.textPrimary);
            } else if (id == IDC_STATIC_LINK_AUTHOR || id == IDC_STATIC_LINK_TOOLS) {
                SetTextColor(hdcStatic, tc.linkColor);
            } else {
                SetTextColor(hdcStatic, tc.textPrimary);
            }

            if (!g_hPanelBgBrush) {
                g_hPanelBgBrush = CreateSolidBrush(tc.panelBg);
            }
            return (LRESULT)g_hPanelBgBrush;
        }

        case WM_CTLCOLOREDIT: {
            HDC hdcEdit = (HDC)wParam;
            UIThemeColors tc = GetCurrentThemeColors();
            SetBkMode(hdcEdit, OPAQUE);
            SetBkColor(hdcEdit, tc.inputBg);
            SetTextColor(hdcEdit, tc.textPrimary);
            if (g_hInputBgBrush) DeleteObject(g_hInputBgBrush);
            g_hInputBgBrush = CreateSolidBrush(tc.inputBg);
            return (LRESULT)g_hInputBgBrush;
        }

        case WM_CTLCOLORLISTBOX: {
            HDC hdcList = (HDC)wParam;
            UIThemeColors tc = GetCurrentThemeColors();
            SetBkColor(hdcList, tc.inputBg);
            SetTextColor(hdcList, tc.textPrimary);
            if (g_hInputBgBrush) DeleteObject(g_hInputBgBrush);
            g_hInputBgBrush = CreateSolidBrush(tc.inputBg);
            return (LRESULT)g_hInputBgBrush;
        }

        case WM_NOTIFY: {
            if (MakeCropMarksAIPlugin::IsPopulatingSpots()) break;
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh && pnmh->idFrom == IDC_LIST_SPOTS) {
                if (pnmh->code == LVN_ITEMCHANGED) {
                    NMLISTVIEW* pnmv = (NMLISTVIEW*)lParam;
                    if (pnmv->uChanged & LVIF_STATE) {
                        int item = pnmv->iItem;
                        if (item >= 0) {
                            BOOL checked = ListView_GetCheckState(pnmh->hwndFrom, item);
                            auto& spots = ConfigManager::GetInstance().GetDocumentSpots();
                            if (item < (int)spots.size()) {
                                spots[item].isSelected = (checked != 0);
                            }
                        }
                    }
                }
            }
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            // Forward click on static label (id = cid + 1000) to corresponding checkbox
            if (code == STN_CLICKED && id >= 3200 && id < 4000) {
                int chkId = id - 1000;
                HWND hChk = GetDlgItem(hwnd, chkId);
                if (hChk) {
                    LRESULT state = SendMessage(hChk, BM_GETCHECK, 0, 0);
                    SendMessage(hChk, BM_SETCHECK, state == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED, 0);
                    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(chkId, BN_CLICKED), (LPARAM)hChk);
                    return 0;
                }
            }

            if (id >= IDC_PRESET_BTN_BASE && id < IDC_PRESET_BTN_BASE + 8) {
                int presetIdx = id - IDC_PRESET_BTN_BASE;
                ConfigManager::GetInstance().SetActivePresetIndex(presetIdx);
                auto p = ConfigManager::GetInstance().GetPreset(presetIdx);
                MakeCropMarksAIPlugin::GetInstance().PopulateUIFromParams(p.params);

                // Update preset name edit box
                SetDlgItemTextA(hwnd, IDC_ED_PRESET_NAME, p.name.c_str());

                // Update selected button highlight
                for (int i = 0; i < 8; ++i) {
                    HWND hB = GetDlgItem(hwnd, IDC_PRESET_BTN_BASE + i);
                    if (i == presetIdx) SetPropA(hB, "MK_Selected", (HANDLE)1);
                    else RemovePropA(hB, "MK_Selected");
                    InvalidateRect(hB, NULL, FALSE);
                }
                return 0;
            }

            switch (id) {
                // Tab Navigation
                case IDC_TAB_MARKS: {
                    MakeCropMarksAIPlugin::GetInstance().SwitchTab(kTabMarks);
                    return 0;
                }
                case IDC_TAB_PANTONES: {
                    MakeCropMarksAIPlugin::GetInstance().SwitchTab(kTabPantones);
                    return 0;
                }
                case IDC_TAB_SETTINGS: {
                    MakeCropMarksAIPlugin::GetInstance().SwitchTab(kTabSettings);
                    return 0;
                }

                // Process Inks Checkboxes
                case IDC_CHK_INK_C:
                case IDC_CHK_INK_M:
                case IDC_CHK_INK_Y:
                case IDC_CHK_INK_K: {
                    ConfigManager::GetInstance().SetBarCyan(IsDlgButtonChecked(hwnd, IDC_CHK_INK_C) == BST_CHECKED);
                    ConfigManager::GetInstance().SetBarMagenta(IsDlgButtonChecked(hwnd, IDC_CHK_INK_M) == BST_CHECKED);
                    ConfigManager::GetInstance().SetBarYellow(IsDlgButtonChecked(hwnd, IDC_CHK_INK_Y) == BST_CHECKED);
                    ConfigManager::GetInstance().SetBarBlack(IsDlgButtonChecked(hwnd, IDC_CHK_INK_K) == BST_CHECKED);
                    return 0;
                }

                // Pantones Tab Actions
                case IDC_BTN_SCAN_SPOTS: {
                    AppContext appContext;
                    auto spots = CropMarkEngine::GetInstance().ScanDocumentPantones();
                    ConfigManager::GetInstance().SetDocumentSpots(spots);
                    MakeCropMarksAIPlugin::GetInstance().PopulatePantonesList();
                    char sbuf[128];
                    snprintf(sbuf, sizeof(sbuf), "Scanned: %zu spot color(s) found.", spots.size());
                    SetDlgItemTextA(hwnd, IDC_STATIC_STATUS, sbuf);
                    return 0;
                }
                case IDC_BTN_SPOTS_ALL: {
                    auto& spots = ConfigManager::GetInstance().GetDocumentSpots();
                    HWND hList = GetDlgItem(hwnd, IDC_LIST_SPOTS);
                    for (size_t i = 0; i < spots.size(); ++i) {
                        spots[i].isSelected = true;
                        if (hList) ListView_SetCheckState(hList, (int)i, TRUE);
                    }
                    return 0;
                }
                case IDC_BTN_SPOTS_NONE: {
                    auto& spots = ConfigManager::GetInstance().GetDocumentSpots();
                    HWND hList = GetDlgItem(hwnd, IDC_LIST_SPOTS);
                    for (size_t i = 0; i < spots.size(); ++i) {
                        spots[i].isSelected = false;
                        if (hList) ListView_SetCheckState(hList, (int)i, FALSE);
                    }
                    return 0;
                }
                case IDC_BTN_SPOTS_FILTER: {
                    auto& spots = ConfigManager::GetInstance().GetDocumentSpots();
                    HWND hList = GetDlgItem(hwnd, IDC_LIST_SPOTS);
                    for (size_t i = 0; i < spots.size(); ++i) {
                        spots[i].isSelected = !spots[i].isTechnical;
                        if (hList) ListView_SetCheckState(hList, (int)i, spots[i].isSelected ? TRUE : FALSE);
                    }
                    SetDlgItemTextA(hwnd, IDC_STATIC_STATUS, "Technical colors excluded.");
                    return 0;
                }
                case IDC_BTN_PANTONES_APPLY: {
                    MakeCropMarksAIPlugin::GetInstance().UpdateSpotsFromList();
                    ConfigManager::GetInstance().Save();
                    SetDlgItemTextA(hwnd, IDC_STATIC_STATUS, "Pantone color settings applied.");
                    return 0;
                }

                // Settings Tab Actions
                case IDC_BTN_SET_HOTKEYS: {
                    float scale = GetPanelScale(hwnd);
                    ConfigManager::GetInstance().ShowSettingsDialog(hwnd, scale);
                    return 0;
                }
                case IDC_BTN_SET_SAVE_DEFAULTS: {
                    MakeCropMarksAIPlugin::GetInstance().ReadSettingsFromUI();
                    if (hwnd) {
                        CropMarkParams p;
                        MakeCropMarksAIPlugin::GetInstance().ReadParamsFromUI(p);
                        ConfigManager::GetInstance().SetActiveParams(p);
                    }
                    ConfigManager::GetInstance().Save();
                    SetDlgItemTextA(hwnd, IDC_STATIC_STATUS, "Settings saved as defaults.");
                    return 0;
                }
                case IDC_BTN_SETTINGS_APPLY: {
                    MakeCropMarksAIPlugin::GetInstance().ReadSettingsFromUI();
                    if (hwnd) {
                        CropMarkParams p;
                        MakeCropMarksAIPlugin::GetInstance().ReadParamsFromUI(p);
                        ConfigManager::GetInstance().SetActiveParams(p);
                    }
                    ConfigManager::GetInstance().Save();
                    SetDlgItemTextA(hwnd, IDC_STATIC_STATUS, "Settings applied successfully.");
                    return 0;
                }

                case IDC_BTN_SETTINGS: {
                    float scale = GetPanelScale(hwnd);
                    ConfigManager::GetInstance().ShowSettingsDialog(hwnd, scale);
                    return 0;
                }
                case IDC_BTN_PRESET_SAVE: {
                    int idx = ConfigManager::GetInstance().GetActivePresetIndex();
                    PresetItem p = ConfigManager::GetInstance().GetPreset(idx);
                    MakeCropMarksAIPlugin::GetInstance().ReadParamsFromUI(p.params);
                    char nameBuf[128] = { 0 };
                    GetDlgItemTextA(hwnd, IDC_ED_PRESET_NAME, nameBuf, sizeof(nameBuf));
                    if (strlen(nameBuf) > 0) {
                        p.name = nameBuf;
                    }
                    ConfigManager::GetInstance().SetPreset(idx, p);
                    SetDlgItemTextA(hwnd, IDC_STATIC_STATUS, "Preset saved successfully.");
                    return 0;
                }
                case IDC_STATIC_LINK_AUTHOR: {
                    ShellExecuteA(NULL, "open", "https://github.com/SaidAuita/CropMarksAI", NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                }
                case IDC_STATIC_LINK_TOOLS: {
                    ShellExecuteA(NULL, "open", "https://ph-cu-s.com/tools", NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                }
                case IDC_BTN_DIR_ALL: {
                    for (int chkId = IDC_CHK_TL; chkId <= IDC_CHK_RB; ++chkId) {
                        CheckDlgButton(hwnd, chkId, BST_CHECKED);
                    }
                    return 0;
                }
                case IDC_BTN_DIR_CORNERS: {
                    for (int chkId = IDC_CHK_TL; chkId <= IDC_CHK_RB; ++chkId) {
                        bool isCorner = (chkId != IDC_CHK_TC && chkId != IDC_CHK_BC &&
                                         chkId != IDC_CHK_LC && chkId != IDC_CHK_RC);
                        CheckDlgButton(hwnd, chkId, isCorner ? BST_CHECKED : BST_UNCHECKED);
                    }
                    return 0;
                }
                case IDC_BTN_DIR_NONE: {
                    for (int chkId = IDC_CHK_TL; chkId <= IDC_CHK_RB; ++chkId) {
                        CheckDlgButton(hwnd, chkId, BST_UNCHECKED);
                    }
                    return 0;
                }
                case IDC_BTN_CREATE: {
                    MakeCropMarksAIPlugin::GetInstance().ExecuteCreateMarks();
                    return 0;
                }
            }
            break;
        }

        case WM_DESTROY:
            g_oldPanelProc = NULL;
            break;
    }

    if (g_oldPanelProc) {
        return CallWindowProc(g_oldPanelProc, hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

MakeCropMarksAIPlugin& MakeCropMarksAIPlugin::GetInstance() {
    static MakeCropMarksAIPlugin instance;
    return instance;
}

MakeCropMarksAIPlugin::MakeCropMarksAIPlugin()
    : m_pluginRef(nullptr),
      m_panelRef(nullptr),
      m_hPanelDlg(NULL),
      m_hBrightnessNotifier(nullptr),
      m_hMenuWindow(nullptr),
      m_hMenuCreate(nullptr),
      m_hMenuQuickApply(nullptr),
      m_hMenuSettings(nullptr),
      m_hMenuAbout(nullptr) {}

MakeCropMarksAIPlugin::~MakeCropMarksAIPlugin() {}

SPErr MakeCropMarksAIPlugin::StartupPlugin(void* message) {
    PluginLog("[MakeCropMarksAIPlugin] StartupPlugin starting...");

    // 1. Create official dockable panel
    CreateDockablePanel();

    // 2. Add menu commands
    AddMenuItems();

    // 3. Register theme change notifier
    if (sAINotifier) {
        sAINotifier->AddNotifier(m_pluginRef, "CropMarksAIPlugin",
                                 kAIUIBrightnessChangedNotifier, &m_hBrightnessNotifier);
    }

    // 4. Install thread-level keyboard hook
    if (!g_hKeyHook) {
        g_hKeyHook = SetWindowsHookExW(WH_GETMESSAGE, KeyboardHookProc, NULL, GetCurrentThreadId());
        PluginLog("  [SetWindowsHookExW] Keyboard hook installed: %p (threadId=%u)",
                  g_hKeyHook, GetCurrentThreadId());
    }

    UpdateUI();
    return kSPNoError;
}

SPErr MakeCropMarksAIPlugin::Notify(void* message) {
    if (!message) return 1;
    AINotifierMessage* notifMsg = static_cast<AINotifierMessage*>(message);
    if (notifMsg->notifier == m_hBrightnessNotifier) {
        PluginLog("[MakeCropMarksAIPlugin::Notify] UI Brightness changed!");
        if (g_hPanelBgBrush) {
            DeleteObject(g_hPanelBgBrush);
            g_hPanelBgBrush = NULL;
        }
        if (m_hPanelDlg) {
            InvalidateRect(m_hPanelDlg, NULL, TRUE);
            EnumChildWindows(m_hPanelDlg, [](HWND hChild, LPARAM) -> BOOL {
                InvalidateRect(hChild, NULL, TRUE);
                return TRUE;
            }, 0);
        }
    }
    return kNoErr;
}

SPErr MakeCropMarksAIPlugin::ShutdownPlugin(void* message) {
    PluginLog("[MakeCropMarksAIPlugin] ShutdownPlugin starting...");

    if (g_hKeyHook) {
        UnhookWindowsHookEx(g_hKeyHook);
        g_hKeyHook = NULL;
    }

    if (m_panelRef && sAIPanel) {
        sAIPanel->Destroy(m_panelRef);
        m_panelRef = nullptr;
    }

    if (g_hUIFont) { DeleteObject(g_hUIFont); g_hUIFont = NULL; }
    if (g_hHeaderFont) { DeleteObject(g_hHeaderFont); g_hHeaderFont = NULL; }
    if (g_hBoldFont) { DeleteObject(g_hBoldFont); g_hBoldFont = NULL; }
    if (g_hSmallFont) { DeleteObject(g_hSmallFont); g_hSmallFont = NULL; }
    if (g_hPanelBgBrush) { DeleteObject(g_hPanelBgBrush); g_hPanelBgBrush = NULL; }

    return kSPNoError;
}

SPErr MakeCropMarksAIPlugin::CreateDockablePanel() {
    if (!sAIPanel) {
        PluginLog("[CreateDockablePanel FAIL] sAIPanel is NULL!");
        return 1;
    }

    AISize minSize = { 100, 100 };
    AIErr err = sAIPanel->Create(
        m_pluginRef,
        ai::UnicodeString("CropMarksAIPanel"),
        ai::UnicodeString("CropMarksAI"),
        1,
        minSize,
        true,
        nullptr,
        this,
        m_panelRef
    );

    if (err != kNoErr || !m_panelRef) {
        PluginLog("[CreateDockablePanel FAIL] sAIPanel->Create error: %d", err);
        return 1;
    }

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    AISize minS = { 160, 300 };
    AISize maxS = { 1200, 1200 };
    AISize prefConstS = { 240, 580 };
    AISize prefUnconstS = { 240, 580 };
    sAIPanel->SetSizes(m_panelRef, minS, prefUnconstS, prefConstS, maxS);
    sAIPanel->SetSVGIconResourceID(m_panelRef, 16300, 16301);

    sAIPanel->SetVisibilityChangedNotifyProc(m_panelRef, PanelVisibilityChangedNotifyProc);
    sAIPanel->SetSizeChangedNotifyProc(m_panelRef, PanelSizeChangedNotifyProc);
    sAIPanel->SetClosedNotifyProc(m_panelRef, PanelClosedNotifyProc);

    AIPanelPlatformWindow platWin = nullptr;
    err = sAIPanel->GetPlatformWindow(m_panelRef, platWin);
    m_hPanelDlg = (HWND)platWin;

    if (!m_hPanelDlg) {
        PluginLog("[CreateDockablePanel FAIL] Platform window is NULL!");
        return 1;
    }

    SetWindowLongPtr(m_hPanelDlg, GWL_STYLE, GetWindowLongPtr(m_hPanelDlg, GWL_STYLE) | WS_CLIPCHILDREN);
    PluginLog("[CreateDockablePanel OK] Panel HWND = %p", m_hPanelDlg);

    float initScale = GetPanelScale(m_hPanelDlg);
    EnsureFontsForScale(m_hPanelDlg, initScale);

    AIReal prefW = (AIReal)(int)(240.0f * initScale + 0.5f);
    AIReal prefH = (AIReal)(int)(580.0f * initScale + 0.5f);
    AISize scaledPref = { prefW, prefH };
    AISize scaledMax = { (std::max)(1200.0, prefW * 2.0), (std::max)(1200.0, prefH * 2.0) };
    sAIPanel->SetSizes(m_panelRef, minS, scaledPref, scaledPref, scaledMax);

    HINSTANCE hInst = GetModuleHandle(NULL);

    auto makeStatic = [&](const char* txt, int sx, int sy, int sw, int sh, int cid, HFONT f = g_hUIFont) {
        HWND h = CreateWindowExA(0, "STATIC", txt, WS_CHILD | WS_VISIBLE, sx, sy, sw, sh, m_hPanelDlg, (HMENU)(INT_PTR)cid, hInst, NULL);
        SendMessage(h, WM_SETFONT, (WPARAM)f, TRUE);
        return h;
    };

    auto makeEdit = [&](int ex, int ey, int ew, int eh, int cid) {
        HWND h = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | WS_BORDER,
                                 ex, ey, ew, eh, m_hPanelDlg, (HMENU)(INT_PTR)cid, hInst, NULL);
        SendMessage(h, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        SubclassEdit(h);
        return h;
    };

    auto makeCombo = [&](int cx, int cy, int cw, int ch, int cid) {
        HWND h = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP,
                                 cx, cy, cw, ch, m_hPanelDlg, (HMENU)(INT_PTR)cid, hInst, NULL);
        SendMessage(h, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        SubclassCombo(h);
        SendMessageA(h, CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)24);
        SendMessageA(h, CB_SETITEMHEIGHT, (WPARAM)0,  (LPARAM)22);
        return h;
    };

    auto makeCheckWithLabel = [&](const char* txt, int kx, int ky, int kw, int kh, int cid, HFONT hFont = g_hBoldFont) {
        int boxSize = 15;
        HWND hChk = CreateWindowExA(0, "BUTTON", "", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                                    kx, ky + (kh - boxSize) / 2, boxSize, boxSize, m_hPanelDlg, (HMENU)(INT_PTR)cid, hInst, NULL);
        HWND hLbl = CreateWindowExA(0, "STATIC", txt, WS_CHILD | WS_VISIBLE | SS_NOTIFY,
                                    kx + boxSize + 6, ky, kw - boxSize - 6, kh, m_hPanelDlg, (HMENU)(INT_PTR)(cid + 1000), hInst, NULL);
        SendMessage(hLbl, WM_SETFONT, (WPARAM)hFont, TRUE);
        return hChk;
    };

    auto makeBtn = [&](const char* txt, int bx, int by, int bw, int bh, int cid, bool primary = false) {
        HWND h = CreateWindowExA(0, "BUTTON", txt, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                 bx, by, bw, bh, m_hPanelDlg, (HMENU)(INT_PTR)cid, hInst, NULL);
        SendMessage(h, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        SubclassButton(h, primary);
        return h;
    };

    // Row 1: Header + Version
    makeStatic("CROPMARKS AI", 10, 8, 160, 20, IDC_HEADER_TITLE, g_hHeaderFont);
    makeStatic(PLUGIN_VERSION_STR, 174, 10, 48, 16, IDC_HEADER_VER, g_hSmallFont);

    // Tab Navigation Bar (Marks, Pantones, Settings)
    makeBtn("Marks", 10, 26, 60, 22, IDC_TAB_MARKS);
    makeBtn("Pantones", 72, 26, 60, 22, IDC_TAB_PANTONES);
    makeBtn("Settings", 134, 26, 60, 22, IDC_TAB_SETTINGS);

    // ==========================================
    // TAB 0: MARKS CONTROLS
    // ==========================================
    // Row 2: Presets (1..8)
    int btnW = 26;
    int btnH = 24;
    for (int i = 0; i < 8; ++i) {
        char num[8];
        snprintf(num, sizeof(num), "%d", i + 1);
        makeBtn(num, 10 + i * 27, 34, btnW, btnH, IDC_PRESET_BTN_BASE + i);
    }

    // Row 3: Preset Name Edit + Save
    makeEdit(10, 63, 115, 24, IDC_ED_PRESET_NAME);
    makeBtn("Save", 130, 63, 44, 24, IDC_BTN_PRESET_SAVE);

    // Row 4: Directions Grid (TL, TC, TR, LT, RT, LC, RC, LB, RB, BL, BC, BR)
    // Top Row
    makeCheckWithLabel("TL", 28, 96, 46, 22, IDC_CHK_TL, g_hBoldFont);
    makeCheckWithLabel("TC", 92, 96, 46, 22, IDC_CHK_TC, g_hBoldFont);
    makeCheckWithLabel("TR", 155, 96, 46, 22, IDC_CHK_TR, g_hBoldFont);

    // Middle Row 1
    makeCheckWithLabel("LT", 8, 120, 46, 22, IDC_CHK_LT, g_hBoldFont);
    makeCheckWithLabel("RT", 175, 120, 46, 22, IDC_CHK_RT, g_hBoldFont);

    // Middle Row 2
    makeCheckWithLabel("LC", 8, 144, 46, 22, IDC_CHK_LC, g_hBoldFont);
    makeCheckWithLabel("RC", 175, 144, 46, 22, IDC_CHK_RC, g_hBoldFont);

    // Middle Row 3
    makeCheckWithLabel("LB", 8, 168, 46, 22, IDC_CHK_LB, g_hBoldFont);
    makeCheckWithLabel("RB", 175, 168, 46, 22, IDC_CHK_RB, g_hBoldFont);

    // Bottom Row
    makeCheckWithLabel("BL", 28, 194, 46, 22, IDC_CHK_BL, g_hBoldFont);
    makeCheckWithLabel("BC", 92, 194, 46, 22, IDC_CHK_BC, g_hBoldFont);
    makeCheckWithLabel("BR", 155, 194, 46, 22, IDC_CHK_BR, g_hBoldFont);

    // Direction Presets (All, Corners, Clear)
    makeBtn("All", 28, 222, 50, 24, IDC_BTN_DIR_ALL);
    makeBtn("Corners", 84, 222, 66, 24, IDC_BTN_DIR_CORNERS);
    makeBtn("Clear", 155, 222, 50, 24, IDC_BTN_DIR_NONE);

    // Row 5: Inputs (Length, Offset, Bleed, Stroke)
    makeStatic("Length (V / H):", 10, 254, 90, 22, IDC_LBL_LENGTH);
    makeEdit(102, 252, 35, 24, IDC_ED_LEN_VER);
    makeEdit(140, 252, 35, 24, IDC_ED_LEN_HOR);
    HWND hCmbUnits = makeCombo(178, 252, 56, 140, IDC_CMB_UNITS);

    makeStatic("Offset (V / H):", 10, 282, 90, 22, IDC_LBL_OFFSET);
    makeEdit(102, 280, 35, 24, IDC_ED_OFF_VER);
    makeEdit(140, 280, 35, 24, IDC_ED_OFF_HOR);

    makeStatic("Bleed (V / H):", 10, 310, 90, 22, IDC_LBL_BLEED);
    makeEdit(102, 308, 35, 24, IDC_ED_BLEED_VER);
    makeEdit(140, 308, 35, 24, IDC_ED_BLEED_HOR);

    makeStatic("Stroke Width:", 10, 338, 90, 22, IDC_LBL_STROKE);
    makeEdit(102, 336, 35, 24, IDC_ED_STROKE);
    HWND hCmbStrokeUnits = makeCombo(140, 336, 56, 140, IDC_CMB_STROKE_UNITS);

    // Populate units
    for (int u = 0; u < 4; ++u) {
        SendMessageA(hCmbUnits, CB_ADDSTRING, 0, (LPARAM)CropMarkParams::GetUnitName(u));
        SendMessageA(hCmbStrokeUnits, CB_ADDSTRING, 0, (LPARAM)CropMarkParams::GetUnitName(u));
    }

    // Row 6: Perimeter Only Checkbox (no inner marks inside block)
    makeCheckWithLabel("Only perimeter (no inner marks)", 10, 368, 226, 20, IDC_CHK_PERIMETER_ONLY, g_hUIFont);

    // Row 7: White Knockout Contour Checkbox
    makeCheckWithLabel("White knockout contour underlay (+1 pt)", 10, 392, 226, 20, IDC_CHK_WHITE_CONTOUR, g_hUIFont);

    // Registration Marks & Color Bars
    makeCheckWithLabel("Registration marks (crosshairs)", 10, 416, 226, 20, IDC_CHK_REG_MARKS, g_hUIFont);
    makeCheckWithLabel("Color bars (long side)", 10, 440, 226, 20, IDC_CHK_BARS_LONG, g_hUIFont);
    makeCheckWithLabel("Color bars (short side)", 10, 464, 226, 20, IDC_CHK_BARS_SHORT, g_hUIFont);

    // Row 8: Target, Bounds, Grouping
    makeStatic("Target:", 10, 420, 58, 20, IDC_LBL_TARGET);
    HWND hCmbTarget = makeCombo(70, 418, 164, 140, IDC_CMB_TARGET);
    SendMessageA(hCmbTarget, CB_ADDSTRING, 0, (LPARAM)"Each object in selection");
    SendMessageA(hCmbTarget, CB_ADDSTRING, 0, (LPARAM)"Entire selection (1 box)");
    SendMessageA(hCmbTarget, CB_ADDSTRING, 0, (LPARAM)"Artboard bounds");

    makeStatic("Bounds:", 10, 446, 58, 20, IDC_LBL_BOUNDS);
    HWND hCmbBounds = makeCombo(70, 444, 164, 140, IDC_CMB_BOUNDS);
    SendMessageA(hCmbBounds, CB_ADDSTRING, 0, (LPARAM)"Geometric Bounds");
    SendMessageA(hCmbBounds, CB_ADDSTRING, 0, (LPARAM)"Visible (with stroke)");

    makeStatic("Grouping:", 10, 472, 58, 20, IDC_LBL_GROUP);
    HWND hCmbGroup = makeCombo(70, 470, 164, 140, IDC_CMB_GROUP);
    SendMessageA(hCmbGroup, CB_ADDSTRING, 0, (LPARAM)"No grouping");
    SendMessageA(hCmbGroup, CB_ADDSTRING, 0, (LPARAM)"Marks only");
    SendMessageA(hCmbGroup, CB_ADDSTRING, 0, (LPARAM)"Marks and selection");

    // Main Action Button (Marks)
    makeBtn("CREATE CROP MARKS", 10, 502, 224, 34, IDC_BTN_CREATE, true);

    // ==========================================
    // TAB 1: PANTONES CONTROLS
    // ==========================================
    makeStatic("PROCESS INKS (CMYK)", 10, 52, 220, 18, IDC_LBL_PROCESS_INKS, g_hBoldFont);
    makeCheckWithLabel("C", 10, 74, 45, 18, IDC_CHK_INK_C, g_hUIFont);
    makeCheckWithLabel("M", 60, 74, 45, 18, IDC_CHK_INK_M, g_hUIFont);
    makeCheckWithLabel("Y", 110, 74, 45, 18, IDC_CHK_INK_Y, g_hUIFont);
    makeCheckWithLabel("K", 160, 74, 45, 18, IDC_CHK_INK_K, g_hUIFont);

    makeStatic("SPOT / PANTONE COLORS", 10, 98, 220, 18, IDC_LBL_SPOTS_HEADER, g_hBoldFont);
    makeBtn("Rescan", 10, 118, 50, 22, IDC_BTN_SCAN_SPOTS);
    makeBtn("All", 64, 118, 50, 22, IDC_BTN_SPOTS_ALL);
    makeBtn("None", 118, 118, 50, 22, IDC_BTN_SPOTS_NONE);
    makeBtn("Filter", 172, 118, 50, 22, IDC_BTN_SPOTS_FILTER);

    makeStatic("0 spot colors in document", 10, 144, 220, 16, IDC_STATIC_SPOTS_COUNT, g_hSmallFont);

    HWND hSpotsList = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_SINGLESEL,
        10, 164, 220, 330, m_hPanelDlg, (HMENU)(INT_PTR)IDC_LIST_SPOTS, hInst, NULL);
    ListView_SetExtendedListViewStyle(hSpotsList, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    SendMessage(hSpotsList, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
    LVCOLUMNA lvc = { 0 };
    lvc.mask = LVCF_WIDTH | LVCF_TEXT;
    lvc.cx = 200;
    lvc.pszText = (LPSTR)"Spot Color";
    ListView_InsertColumn(hSpotsList, 0, &lvc);

    makeBtn("APPLY PANTONES", 10, 508, 220, 28, IDC_BTN_PANTONES_APPLY, true);

    // ==========================================
    // TAB 2: SETTINGS CONTROLS
    // ==========================================
    makeStatic("COLOR BARS", 10, 52, 220, 18, IDC_LBL_SET_BAR_HEADER, g_hBoldFont);
    makeStatic("Mode:", 10, 72, 70, 18, IDC_LBL_SET_BAR_MODE);
    HWND hCmbBarMode = makeCombo(82, 71, 140, 140, IDC_CMB_BAR_MODE);
    SendMessageA(hCmbBarMode, CB_ADDSTRING, 0, (LPARAM)"100% Solids (CMYK+Pantones)");
    SendMessageA(hCmbBarMode, CB_ADDSTRING, 0, (LPARAM)"ISO 12647-2 (Halftones)");
    SendMessageA(hCmbBarMode, CB_ADDSTRING, 0, (LPARAM)"ISO 12647-7 (Proofing)");

    makeStatic("Patch (WxH):", 10, 96, 70, 18, IDC_LBL_SET_PATCH_SIZE);
    makeEdit(82, 95, 65, 20, IDC_ED_SET_PATCH_W);
    makeEdit(152, 95, 65, 20, IDC_ED_SET_PATCH_H);

    makeStatic("Offset (mm):", 10, 120, 70, 18, IDC_LBL_SET_BAR_OFFSET);
    makeEdit(82, 119, 140, 20, IDC_ED_SET_BAR_OFFSET);

    makeStatic("Length:", 10, 144, 70, 18, IDC_LBL_SET_BAR_LEN);
    HWND hCmbBarLen = makeCombo(82, 143, 140, 140, IDC_CMB_BAR_LEN);
    SendMessageA(hCmbBarLen, CB_ADDSTRING, 0, (LPARAM)"Short centered strip");
    SendMessageA(hCmbBarLen, CB_ADDSTRING, 0, (LPARAM)"Full side length");

    makeStatic("Order:", 10, 168, 70, 18, IDC_LBL_SET_BAR_ORDER);
    HWND hCmbBarOrd = makeCombo(82, 167, 140, 140, IDC_CMB_BAR_ORDER);
    SendMessageA(hCmbBarOrd, CB_ADDSTRING, 0, (LPARAM)"Above crop marks");
    SendMessageA(hCmbBarOrd, CB_ADDSTRING, 0, (LPARAM)"Below crop marks");

    makeStatic("REGISTRATION MARKS", 10, 196, 220, 18, IDC_LBL_SET_REG_HEADER, g_hBoldFont);
    makeStatic("Diam / Arm:", 10, 216, 70, 18, IDC_LBL_SET_REG_DIAM);
    makeEdit(82, 215, 65, 20, IDC_ED_SET_REG_DIAM);
    makeEdit(152, 215, 65, 20, IDC_ED_SET_REG_LEN);

    makeStatic("Stroke Width:", 10, 240, 70, 18, IDC_LBL_SET_REG_STROKE);
    makeEdit(82, 239, 65, 20, IDC_ED_SET_REG_STROKE);
    HWND hCmbRegStrokeUn = makeCombo(152, 239, 65, 140, IDC_CMB_SET_REG_STROKE_UN);
    for (int u = 0; u < 4; ++u) {
        SendMessageA(hCmbRegStrokeUn, CB_ADDSTRING, 0, (LPARAM)CropMarkParams::GetUnitName(u));
    }

    makeStatic("Offset (H / V):", 10, 264, 70, 18, IDC_LBL_SET_REG_OFFSET);
    makeEdit(82, 263, 65, 20, IDC_ED_SET_REG_OFF_H);
    makeEdit(152, 263, 65, 20, IDC_ED_SET_REG_OFF_V);

    makeStatic("Placement:", 10, 288, 70, 18, IDC_LBL_SET_REG_POS);
    HWND hCmbRegPos = makeCombo(82, 287, 140, 140, IDC_CMB_SET_REG_POS);
    SendMessageA(hCmbRegPos, CB_ADDSTRING, 0, (LPARAM)"Centers (4)");
    SendMessageA(hCmbRegPos, CB_ADDSTRING, 0, (LPARAM)"Corners (4)");
    SendMessageA(hCmbRegPos, CB_ADDSTRING, 0, (LPARAM)"Centers + Corners (8)");
    SendMessageA(hCmbRegPos, CB_ADDSTRING, 0, (LPARAM)"All 16 Directions (16)");

    makeCheckWithLabel("White knockout contour underlay (+1.2 pt)", 10, 312, 220, 20, IDC_CHK_SET_REG_WHITE, g_hUIFont);

    makeStatic("Order:", 10, 334, 70, 18, IDC_LBL_SET_REG_ORDER);
    HWND hCmbRegOrd = makeCombo(82, 333, 140, 140, IDC_CMB_SET_REG_ORDER);
    SendMessageA(hCmbRegOrd, CB_ADDSTRING, 0, (LPARAM)"Above crop marks");
    SendMessageA(hCmbRegOrd, CB_ADDSTRING, 0, (LPARAM)"Below crop marks");

    makeStatic("DOCUMENT INFO", 10, 362, 220, 18, IDC_LBL_SET_DOC_HEADER, g_hBoldFont);
    makeCheckWithLabel("Add document name, date & time stamp", 10, 380, 220, 20, IDC_CHK_SET_DOC_INFO, g_hUIFont);
    makeCheckWithLabel("Transliterate Cyrillic to Latin", 10, 400, 220, 20, IDC_CHK_SET_DOC_TRANSLIT, g_hUIFont);

    makeStatic("KEYBOARD SHORTCUTS", 10, 424, 220, 18, IDC_LBL_SET_HK_HEADER, g_hBoldFont);
    makeStatic("Default: Ctrl+Shift+F4", 10, 442, 220, 16, IDC_STATIC_SET_HK_INFO, g_hSmallFont);

    makeBtn("Hotkeys...", 10, 464, 105, 24, IDC_BTN_SET_HOTKEYS);
    makeBtn("Save Defaults", 120, 464, 105, 24, IDC_BTN_SET_SAVE_DEFAULTS);

    makeBtn("APPLY SETTINGS", 10, 508, 220, 28, IDC_BTN_SETTINGS_APPLY, true);

    // ==========================================
    // COMMON FOOTER
    // ==========================================
    makeStatic("Ready", 10, 542, 224, 30, IDC_STATIC_STATUS, g_hSmallFont);

    // Footer links (clickable)
    CreateWindowExA(0, "STATIC", "github.com/SaidAuita/CropMarksAI",
                    WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP | SS_ENDELLIPSIS | SS_NOTIFY,
                    10, 574, 220, 16, m_hPanelDlg, (HMENU)(INT_PTR)IDC_STATIC_LINK_AUTHOR, hInst, NULL);
    CreateWindowExA(0, "STATIC", "More scripts: ph-cu-s.com/tools",
                    WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP | SS_ENDELLIPSIS | SS_NOTIFY,
                    10, 592, 220, 16, m_hPanelDlg, (HMENU)(INT_PTR)IDC_STATIC_LINK_TOOLS, hInst, NULL);

    // Subclass panel window for custom theme painting
    g_oldPanelProc = (WNDPROC)SetWindowLongPtrA(m_hPanelDlg, GWLP_WNDPROC, (LONG_PTR)MakeCropMarksAIPanelProc);

    // Initial population from active preset
    int activeIdx = ConfigManager::GetInstance().GetActivePresetIndex();
    PresetItem curPreset = ConfigManager::GetInstance().GetPreset(activeIdx);
    PopulateUIFromParams(curPreset.params);
    SetDlgItemTextA(m_hPanelDlg, IDC_ED_PRESET_NAME, curPreset.name.c_str());
    HWND hActivePresetBtn = GetDlgItem(m_hPanelDlg, IDC_PRESET_BTN_BASE + activeIdx);
    if (hActivePresetBtn) SetPropA(hActivePresetBtn, "MK_Selected", (HANDLE)1);

    // Default to Tab 0: Marks
    SwitchTab(kTabMarks);

    // Initial layout calculation for current dimensions
    RelayoutPanel(m_hPanelDlg);

    return kSPNoError;
}

void MakeCropMarksAIPlugin::SwitchTab(int tabIndex) {
    if (tabIndex < kTabMarks || tabIndex > kTabSettings) return;
    m_currentTab = tabIndex;

    HWND hTab0 = GetDlgItem(m_hPanelDlg, IDC_TAB_MARKS);
    HWND hTab1 = GetDlgItem(m_hPanelDlg, IDC_TAB_PANTONES);
    HWND hTab2 = GetDlgItem(m_hPanelDlg, IDC_TAB_SETTINGS);
    if (hTab0) {
        if (tabIndex == kTabMarks) SetPropA(hTab0, "MK_Selected", (HANDLE)1);
        else RemovePropA(hTab0, "MK_Selected");
        InvalidateRect(hTab0, NULL, FALSE);
    }
    if (hTab1) {
        if (tabIndex == kTabPantones) SetPropA(hTab1, "MK_Selected", (HANDLE)1);
        else RemovePropA(hTab1, "MK_Selected");
        InvalidateRect(hTab1, NULL, FALSE);
    }
    if (hTab2) {
        if (tabIndex == kTabSettings) SetPropA(hTab2, "MK_Selected", (HANDLE)1);
        else RemovePropA(hTab2, "MK_Selected");
        InvalidateRect(hTab2, NULL, FALSE);
    }

    UpdateTabVisibility();

    if (tabIndex == kTabPantones) {
        AppContext appContext;
        if (ConfigManager::GetInstance().GetDocumentSpots().empty()) {
            auto spots = CropMarkEngine::GetInstance().ScanDocumentPantones();
            ConfigManager::GetInstance().SetDocumentSpots(spots);
        }
        PopulatePantonesList();
    } else if (tabIndex == kTabSettings) {
        PopulateSettingsToUI();
    }

    RelayoutPanel(m_hPanelDlg);
    InvalidateRect(m_hPanelDlg, NULL, TRUE);
}

void MakeCropMarksAIPlugin::UpdateTabVisibility() {
    if (!m_hPanelDlg) return;

    const int tab0Controls[] = {
        IDC_ED_PRESET_NAME, IDC_BTN_PRESET_SAVE,
        IDC_CHK_TL, IDC_CHK_TC, IDC_CHK_TR,
        IDC_CHK_BL, IDC_CHK_BC, IDC_CHK_BR,
        IDC_CHK_LT, IDC_CHK_LC, IDC_CHK_LB,
        IDC_CHK_RT, IDC_CHK_RC, IDC_CHK_RB,
        IDC_BTN_DIR_ALL, IDC_BTN_DIR_CORNERS, IDC_BTN_DIR_NONE,
        IDC_LBL_LENGTH, IDC_ED_LEN_VER, IDC_ED_LEN_HOR, IDC_CMB_UNITS,
        IDC_LBL_OFFSET, IDC_ED_OFF_VER, IDC_ED_OFF_HOR,
        IDC_LBL_BLEED, IDC_ED_BLEED_VER, IDC_ED_BLEED_HOR,
        IDC_LBL_STROKE, IDC_ED_STROKE, IDC_CMB_STROKE_UNITS,
        IDC_CHK_PERIMETER_ONLY, IDC_CHK_WHITE_CONTOUR,
        IDC_CHK_REG_MARKS, IDC_CHK_BARS_LONG, IDC_CHK_BARS_SHORT,
        IDC_LBL_TARGET, IDC_CMB_TARGET,
        IDC_LBL_BOUNDS, IDC_CMB_BOUNDS,
        IDC_LBL_GROUP, IDC_CMB_GROUP,
        IDC_BTN_CREATE
    };

    int show0 = (m_currentTab == kTabMarks) ? SW_SHOW : SW_HIDE;
    for (int i = 0; i < 8; ++i) {
        HWND h = GetDlgItem(m_hPanelDlg, IDC_PRESET_BTN_BASE + i);
        if (h) ShowWindow(h, show0);
    }
    for (int cid : tab0Controls) {
        HWND h = GetDlgItem(m_hPanelDlg, cid);
        if (h) ShowWindow(h, show0);
        HWND hLbl = GetDlgItem(m_hPanelDlg, cid + 1000);
        if (hLbl) ShowWindow(hLbl, show0);
    }

    const int tab1Controls[] = {
        IDC_LBL_PROCESS_INKS,
        IDC_CHK_INK_C, IDC_CHK_INK_M, IDC_CHK_INK_Y, IDC_CHK_INK_K,
        IDC_LBL_SPOTS_HEADER,
        IDC_BTN_SCAN_SPOTS, IDC_BTN_SPOTS_ALL, IDC_BTN_SPOTS_NONE, IDC_BTN_SPOTS_FILTER,
        IDC_STATIC_SPOTS_COUNT,
        IDC_LIST_SPOTS,
        IDC_BTN_PANTONES_APPLY
    };

    int show1 = (m_currentTab == kTabPantones) ? SW_SHOW : SW_HIDE;
    for (int cid : tab1Controls) {
        HWND h = GetDlgItem(m_hPanelDlg, cid);
        if (h) ShowWindow(h, show1);
        HWND hLbl = GetDlgItem(m_hPanelDlg, cid + 1000);
        if (hLbl) ShowWindow(hLbl, show1);
    }

    const int tab2Controls[] = {
        IDC_LBL_SET_BAR_HEADER, IDC_LBL_SET_BAR_MODE, IDC_CMB_BAR_MODE,
        IDC_LBL_SET_PATCH_SIZE, IDC_ED_SET_PATCH_W, IDC_ED_SET_PATCH_H,
        IDC_LBL_SET_BAR_OFFSET, IDC_ED_SET_BAR_OFFSET,
        IDC_LBL_SET_BAR_LEN, IDC_CMB_BAR_LEN,
        IDC_LBL_SET_BAR_ORDER, IDC_CMB_BAR_ORDER,
        IDC_LBL_SET_REG_HEADER, IDC_LBL_SET_REG_DIAM, IDC_ED_SET_REG_DIAM, IDC_ED_SET_REG_LEN,
        IDC_LBL_SET_REG_STROKE, IDC_ED_SET_REG_STROKE, IDC_CMB_SET_REG_STROKE_UN,
        IDC_LBL_SET_REG_OFFSET, IDC_ED_SET_REG_OFF_H, IDC_ED_SET_REG_OFF_V,
        IDC_LBL_SET_REG_POS, IDC_CMB_SET_REG_POS,
        IDC_CHK_SET_REG_WHITE,
        IDC_LBL_SET_REG_ORDER, IDC_CMB_SET_REG_ORDER,
        IDC_LBL_SET_DOC_HEADER, IDC_CHK_SET_DOC_INFO, IDC_CHK_SET_DOC_TRANSLIT,
        IDC_LBL_SET_HK_HEADER, IDC_STATIC_SET_HK_INFO,
        IDC_BTN_SET_HOTKEYS, IDC_BTN_SET_SAVE_DEFAULTS,
        IDC_BTN_SETTINGS_APPLY
    };

    int show2 = (m_currentTab == kTabSettings) ? SW_SHOW : SW_HIDE;
    for (int cid : tab2Controls) {
        HWND h = GetDlgItem(m_hPanelDlg, cid);
        if (h) ShowWindow(h, show2);
        HWND hLbl = GetDlgItem(m_hPanelDlg, cid + 1000);
        if (hLbl) ShowWindow(hLbl, show2);
    }
}

bool MakeCropMarksAIPlugin::s_isPopulatingSpots = false;

void MakeCropMarksAIPlugin::PopulatePantonesList() {
    if (!m_hPanelDlg) return;
    s_isPopulatingSpots = true;

    auto& cfg = ConfigManager::GetInstance();
    CheckDlgButton(m_hPanelDlg, IDC_CHK_INK_C, cfg.GetBarCyan() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_INK_M, cfg.GetBarMagenta() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_INK_Y, cfg.GetBarYellow() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_INK_K, cfg.GetBarBlack() ? BST_CHECKED : BST_UNCHECKED);

    HWND hList = GetDlgItem(m_hPanelDlg, IDC_LIST_SPOTS);
    if (!hList) {
        s_isPopulatingSpots = false;
        return;
    }

    UIThemeColors tc = GetCurrentThemeColors();
    ListView_SetBkColor(hList, tc.inputBg);
    ListView_SetTextBkColor(hList, tc.inputBg);
    ListView_SetTextColor(hList, tc.textPrimary);

    ListView_DeleteAllItems(hList);

    auto& spots = cfg.GetDocumentSpots();
    char countBuf[64];
    snprintf(countBuf, sizeof(countBuf), "%zu spot color(s) found in document", spots.size());
    SetDlgItemTextA(m_hPanelDlg, IDC_STATIC_SPOTS_COUNT, countBuf);

    for (size_t i = 0; i < spots.size(); ++i) {
        const auto& sp = spots[i];
        std::string itemText = sp.name;
        if (sp.isTechnical) {
            itemText += " [Technical / Skip]";
        }
        LVITEMA lvi = { 0 };
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = (int)i;
        lvi.pszText = (LPSTR)itemText.c_str();
        lvi.lParam = (LPARAM)i;
        ListView_InsertItem(hList, &lvi);
        ListView_SetCheckState(hList, (int)i, sp.isSelected ? TRUE : FALSE);
    }

    s_isPopulatingSpots = false;
}

void MakeCropMarksAIPlugin::UpdateSpotsFromList() {
    if (!m_hPanelDlg) return;
    auto& cfg = ConfigManager::GetInstance();
    cfg.SetBarCyan(IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_INK_C) == BST_CHECKED);
    cfg.SetBarMagenta(IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_INK_M) == BST_CHECKED);
    cfg.SetBarYellow(IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_INK_Y) == BST_CHECKED);
    cfg.SetBarBlack(IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_INK_K) == BST_CHECKED);

    HWND hList = GetDlgItem(m_hPanelDlg, IDC_LIST_SPOTS);
    if (hList) {
        auto& spots = cfg.GetDocumentSpots();
        int count = ListView_GetItemCount(hList);
        for (int i = 0; i < count && i < (int)spots.size(); ++i) {
            spots[i].isSelected = (ListView_GetCheckState(hList, i) != 0);
        }
    }
}

void MakeCropMarksAIPlugin::PopulateSettingsToUI() {
    if (!m_hPanelDlg) return;
    auto& cfg = ConfigManager::GetInstance();

    char buf[32];
    auto setFloat = [&](int id, double val) {
        snprintf(buf, sizeof(buf), "%.2f", val);
        char* dot = strchr(buf, '.');
        if (dot) {
            char* end = buf + strlen(buf) - 1;
            while (end > dot && *end == '0') *end-- = '\0';
            if (end == dot) *end = '\0';
        }
        SetDlgItemTextA(m_hPanelDlg, id, buf);
    };

    // Color bars
    SendDlgItemMessage(m_hPanelDlg, IDC_CMB_BAR_MODE, CB_SETCURSEL, (WPARAM)cfg.GetBarMode(), 0);
    setFloat(IDC_ED_SET_PATCH_W, cfg.GetPatchWidth());
    setFloat(IDC_ED_SET_PATCH_H, cfg.GetPatchHeight());
    setFloat(IDC_ED_SET_BAR_OFFSET, cfg.GetBarsOffset());
    SendDlgItemMessage(m_hPanelDlg, IDC_CMB_BAR_LEN, CB_SETCURSEL, (WPARAM)cfg.GetBarLength(), 0);
    SendDlgItemMessage(m_hPanelDlg, IDC_CMB_BAR_ORDER, CB_SETCURSEL, (WPARAM)cfg.GetCropMarksLayerOrder(), 0);

    // Reg marks
    setFloat(IDC_ED_SET_REG_DIAM, cfg.GetRegCircleDiam());
    setFloat(IDC_ED_SET_REG_LEN, cfg.GetRegCrossLength());
    setFloat(IDC_ED_SET_REG_STROKE, cfg.GetRegStrokeWidth());
    SendDlgItemMessage(m_hPanelDlg, IDC_CMB_SET_REG_STROKE_UN, CB_SETCURSEL, (WPARAM)cfg.GetRegStrokeUnits(), 0);
    setFloat(IDC_ED_SET_REG_OFF_H, cfg.GetRegOffsetHor());
    setFloat(IDC_ED_SET_REG_OFF_V, cfg.GetRegOffsetVer());
    SendDlgItemMessage(m_hPanelDlg, IDC_CMB_SET_REG_POS, CB_SETCURSEL, (WPARAM)cfg.GetRegPositions(), 0);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_SET_REG_WHITE, cfg.GetRegWhiteUnderlay() ? BST_CHECKED : BST_UNCHECKED);
    SendDlgItemMessage(m_hPanelDlg, IDC_CMB_SET_REG_ORDER, CB_SETCURSEL, (WPARAM)cfg.GetCrossMarksLayerOrder(), 0);

    // Doc info
    CheckDlgButton(m_hPanelDlg, IDC_CHK_SET_DOC_INFO, cfg.GetDocInfo() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_SET_DOC_TRANSLIT, cfg.GetDocInfoTranslit() ? BST_CHECKED : BST_UNCHECKED);
}

void MakeCropMarksAIPlugin::ReadSettingsFromUI() {
    if (!m_hPanelDlg) return;
    auto& cfg = ConfigManager::GetInstance();

    char buf[64];
    auto getFloat = [&](int id, double defVal) -> double {
        GetDlgItemTextA(m_hPanelDlg, id, buf, sizeof(buf));
        for (char* c = buf; *c; ++c) if (*c == ',') *c = '.';
        try { return std::stod(buf); } catch (...) { return defVal; }
    };

    // Color bars
    int bMode = (int)SendDlgItemMessage(m_hPanelDlg, IDC_CMB_BAR_MODE, CB_GETCURSEL, 0, 0);
    if (bMode >= 0) {
        cfg.SetBarMode(bMode);
        if (bMode == kColorBarISO12647_7Mode) {
            cfg.SetBarStandard(kStandardISO12647_7);
        } else if (bMode == kColorBarISO12647_2Mode) {
            cfg.SetBarStandard(kStandardISO12647_2);
        }
    }
    cfg.SetPatchWidth(getFloat(IDC_ED_SET_PATCH_W, 5.0));
    cfg.SetPatchHeight(getFloat(IDC_ED_SET_PATCH_H, 5.0));
    cfg.SetBarsOffset(getFloat(IDC_ED_SET_BAR_OFFSET, 2.0));
    int bLen = (int)SendDlgItemMessage(m_hPanelDlg, IDC_CMB_BAR_LEN, CB_GETCURSEL, 0, 0);
    if (bLen >= 0) cfg.SetBarLength(bLen);
    int bOrd = (int)SendDlgItemMessage(m_hPanelDlg, IDC_CMB_BAR_ORDER, CB_GETCURSEL, 0, 0);
    if (bOrd >= 0) cfg.SetCropMarksLayerOrder(bOrd);

    // Reg marks
    cfg.SetRegCircleDiam(getFloat(IDC_ED_SET_REG_DIAM, 4.0));
    cfg.SetRegCrossLength(getFloat(IDC_ED_SET_REG_LEN, 6.0));
    cfg.SetRegStrokeWidth(getFloat(IDC_ED_SET_REG_STROKE, 0.10));
    int rStrUn = (int)SendDlgItemMessage(m_hPanelDlg, IDC_CMB_SET_REG_STROKE_UN, CB_GETCURSEL, 0, 0);
    if (rStrUn >= 0) cfg.SetRegStrokeUnits(rStrUn);
    cfg.SetRegOffsetHor(getFloat(IDC_ED_SET_REG_OFF_H, 2.0));
    cfg.SetRegOffsetVer(getFloat(IDC_ED_SET_REG_OFF_V, 2.0));
    int rPos = (int)SendDlgItemMessage(m_hPanelDlg, IDC_CMB_SET_REG_POS, CB_GETCURSEL, 0, 0);
    if (rPos >= 0) cfg.SetRegPositions(rPos);
    cfg.SetRegWhiteUnderlay(IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_SET_REG_WHITE) == BST_CHECKED);
    int rOrd = (int)SendDlgItemMessage(m_hPanelDlg, IDC_CMB_SET_REG_ORDER, CB_GETCURSEL, 0, 0);
    if (rOrd >= 0) cfg.SetCrossMarksLayerOrder(rOrd);

    // Doc info
    cfg.SetDocInfo(IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_SET_DOC_INFO) == BST_CHECKED);
    cfg.SetDocInfoTranslit(IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_SET_DOC_TRANSLIT) == BST_CHECKED);
}

void MakeCropMarksAIPlugin::PopulateUIFromParams(const CropMarkParams& p) {
    if (!m_hPanelDlg) return;

    CheckDlgButton(m_hPanelDlg, IDC_CHK_TL, p.dirs.topLeft ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_TC, p.dirs.topCenter ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_TR, p.dirs.topRight ? BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(m_hPanelDlg, IDC_CHK_BL, p.dirs.bottomLeft ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_BC, p.dirs.bottomCenter ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_BR, p.dirs.bottomRight ? BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(m_hPanelDlg, IDC_CHK_LT, p.dirs.leftTop ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_LC, p.dirs.leftCenter ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_LB, p.dirs.leftBottom ? BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(m_hPanelDlg, IDC_CHK_RT, p.dirs.rightTop ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_RC, p.dirs.rightCenter ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_RB, p.dirs.rightBottom ? BST_CHECKED : BST_UNCHECKED);

    char buf[32];
    auto setFloat = [&](int id, double val) {
        snprintf(buf, sizeof(buf), "%.2f", val);
        // Trim trailing zeros
        char* dot = strchr(buf, '.');
        if (dot) {
            char* end = buf + strlen(buf) - 1;
            while (end > dot && *end == '0') *end-- = '\0';
            if (end == dot) *end = '\0';
        }
        SetDlgItemTextA(m_hPanelDlg, id, buf);
    };

    setFloat(IDC_ED_LEN_VER, p.lengthVer);
    setFloat(IDC_ED_LEN_HOR, p.lengthHor);
    setFloat(IDC_ED_OFF_VER, p.offsetVer);
    setFloat(IDC_ED_OFF_HOR, p.offsetHor);
    setFloat(IDC_ED_BLEED_VER, p.bleedVer);
    setFloat(IDC_ED_BLEED_HOR, p.bleedHor);
    setFloat(IDC_ED_STROKE, p.strokeWidth);

    SendDlgItemMessage(m_hPanelDlg, IDC_CMB_UNITS, CB_SETCURSEL, (WPARAM)p.units, 0);
    SendDlgItemMessage(m_hPanelDlg, IDC_CMB_STROKE_UNITS, CB_SETCURSEL, (WPARAM)p.strokeUnits, 0);

    CheckDlgButton(m_hPanelDlg, IDC_CHK_PERIMETER_ONLY, p.perimeterOnly ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_WHITE_CONTOUR, p.whiteContour ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_REG_MARKS, p.regMarks ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_BARS_LONG, p.colorBarsLong ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hPanelDlg, IDC_CHK_BARS_SHORT, p.colorBarsShort ? BST_CHECKED : BST_UNCHECKED);
    SendDlgItemMessage(m_hPanelDlg, IDC_CMB_TARGET, CB_SETCURSEL, (WPARAM)p.targetMode, 0);
    SendDlgItemMessage(m_hPanelDlg, IDC_CMB_BOUNDS, CB_SETCURSEL, (WPARAM)p.boundsMode, 0);
    SendDlgItemMessage(m_hPanelDlg, IDC_CMB_GROUP, CB_SETCURSEL, (WPARAM)p.groupMode, 0);
}

void MakeCropMarksAIPlugin::ReadParamsFromUI(CropMarkParams& p) {
    if (!m_hPanelDlg) return;

    p.dirs.topLeft = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_TL) == BST_CHECKED);
    p.dirs.topCenter = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_TC) == BST_CHECKED);
    p.dirs.topRight = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_TR) == BST_CHECKED);

    p.dirs.bottomLeft = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_BL) == BST_CHECKED);
    p.dirs.bottomCenter = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_BC) == BST_CHECKED);
    p.dirs.bottomRight = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_BR) == BST_CHECKED);

    p.dirs.leftTop = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_LT) == BST_CHECKED);
    p.dirs.leftCenter = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_LC) == BST_CHECKED);
    p.dirs.leftBottom = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_LB) == BST_CHECKED);

    p.dirs.rightTop = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_RT) == BST_CHECKED);
    p.dirs.rightCenter = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_RC) == BST_CHECKED);
    p.dirs.rightBottom = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_RB) == BST_CHECKED);

    char buf[64];
    auto getFloat = [&](int id, double defVal) -> double {
        GetDlgItemTextA(m_hPanelDlg, id, buf, sizeof(buf));
        for (char* c = buf; *c; ++c) if (*c == ',') *c = '.';
        try { return std::stod(buf); } catch (...) { return defVal; }
    };

    p.lengthVer = getFloat(IDC_ED_LEN_VER, 5.0);
    p.lengthHor = getFloat(IDC_ED_LEN_HOR, 5.0);
    p.offsetVer = getFloat(IDC_ED_OFF_VER, 2.0);
    p.offsetHor = getFloat(IDC_ED_OFF_HOR, 2.0);
    p.bleedVer = getFloat(IDC_ED_BLEED_VER, 0.0);
    p.bleedHor = getFloat(IDC_ED_BLEED_HOR, 0.0);
    p.strokeWidth = getFloat(IDC_ED_STROKE, 0.25);

    p.units = (int)SendDlgItemMessage(m_hPanelDlg, IDC_CMB_UNITS, CB_GETCURSEL, 0, 0);
    if (p.units < 0) p.units = kUnitsMM;

    p.strokeUnits = (int)SendDlgItemMessage(m_hPanelDlg, IDC_CMB_STROKE_UNITS, CB_GETCURSEL, 0, 0);
    if (p.strokeUnits < 0) p.strokeUnits = kUnitsPT;

    p.perimeterOnly = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_PERIMETER_ONLY) == BST_CHECKED);
    p.whiteContour = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_WHITE_CONTOUR) == BST_CHECKED);
    p.regMarks = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_REG_MARKS) == BST_CHECKED);
    p.colorBarsLong = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_BARS_LONG) == BST_CHECKED);
    p.colorBarsShort = (IsDlgButtonChecked(m_hPanelDlg, IDC_CHK_BARS_SHORT) == BST_CHECKED);
    p.targetMode = (int)SendDlgItemMessage(m_hPanelDlg, IDC_CMB_TARGET, CB_GETCURSEL, 0, 0);
    p.boundsMode = (int)SendDlgItemMessage(m_hPanelDlg, IDC_CMB_BOUNDS, CB_GETCURSEL, 0, 0);
    p.groupMode = (int)SendDlgItemMessage(m_hPanelDlg, IDC_CMB_GROUP, CB_GETCURSEL, 0, 0);

    p.barsOffset = ConfigManager::GetInstance().GetBarsOffset();
    p.crossOffset = ConfigManager::GetInstance().GetCrossOffset();
    p.cropMarksLayerOrder = ConfigManager::GetInstance().GetCropMarksLayerOrder();
    p.crossMarksLayerOrder = ConfigManager::GetInstance().GetCrossMarksLayerOrder();
    p.barLength = ConfigManager::GetInstance().GetBarLength();
    p.barStandard = ConfigManager::GetInstance().GetBarStandard();
    p.docInfo = ConfigManager::GetInstance().GetDocInfo();
    p.docInfoTranslit = ConfigManager::GetInstance().GetDocInfoTranslit();

    p.barMode = ConfigManager::GetInstance().GetBarMode();
    p.patchWidth = ConfigManager::GetInstance().GetPatchWidth();
    p.patchHeight = ConfigManager::GetInstance().GetPatchHeight();
    p.barCyan = ConfigManager::GetInstance().GetBarCyan();
    p.barMagenta = ConfigManager::GetInstance().GetBarMagenta();
    p.barYellow = ConfigManager::GetInstance().GetBarYellow();
    p.barBlack = ConfigManager::GetInstance().GetBarBlack();
    p.selectedSpots = ConfigManager::GetInstance().GetDocumentSpots();

    p.regCircleDiam = ConfigManager::GetInstance().GetRegCircleDiam();
    p.regCrossLength = ConfigManager::GetInstance().GetRegCrossLength();
    p.regStrokeWidth = ConfigManager::GetInstance().GetRegStrokeWidth();
    p.regStrokeUnits = ConfigManager::GetInstance().GetRegStrokeUnits();
    p.regOffsetHor = ConfigManager::GetInstance().GetRegOffsetHor();
    p.regOffsetVer = ConfigManager::GetInstance().GetRegOffsetVer();
    p.regPositions = ConfigManager::GetInstance().GetRegPositions();
    p.regWhiteUnderlay = ConfigManager::GetInstance().GetRegWhiteUnderlay();
}

void MakeCropMarksAIPlugin::ExecuteCreateMarks() {
    CropMarkParams p;
    ReadParamsFromUI(p);
    ConfigManager::GetInstance().SetActiveParams(p);

    AppContext appContext(m_pluginRef);
    if (ConfigManager::GetInstance().GetDocumentSpots().empty()) {
        auto spots = CropMarkEngine::GetInstance().ScanDocumentPantones();
        ConfigManager::GetInstance().SetDocumentSpots(spots);
        p.selectedSpots = spots;
    }
    CropMarkResult res = CropMarkEngine::GetInstance().CreateCropMarks(p);

    if (m_hPanelDlg) {
        SetDlgItemTextA(m_hPanelDlg, IDC_STATIC_STATUS, res.message.c_str());
    }
}

void MakeCropMarksAIPlugin::ExecuteQuickApply() {
    if (m_hPanelDlg && IsWindow(m_hPanelDlg)) {
        ExecuteCreateMarks();
        return;
    }

    AppContext appContext(m_pluginRef);
    if (ConfigManager::GetInstance().GetDocumentSpots().empty()) {
        auto spots = CropMarkEngine::GetInstance().ScanDocumentPantones();
        ConfigManager::GetInstance().SetDocumentSpots(spots);
    }
    auto p = ConfigManager::GetInstance().GetActiveParams();
    p.selectedSpots = ConfigManager::GetInstance().GetDocumentSpots();
    CropMarkResult res = CropMarkEngine::GetInstance().CreateCropMarks(p);

    if (m_hPanelDlg) {
        SetDlgItemTextA(m_hPanelDlg, IDC_STATIC_STATUS, res.message.c_str());
    }
}

void MakeCropMarksAIPlugin::TogglePanel() {
    if (!m_panelRef || !sAIPanel) return;
    AIBoolean isShown = false;
    sAIPanel->IsShown(m_panelRef, isShown);
    sAIPanel->Show(m_panelRef, !isShown);
}

void MakeCropMarksAIPlugin::UpdateUI() {
    if (m_hPanelDlg) {
        InvalidateRect(m_hPanelDlg, NULL, TRUE);
    }
}

SPErr MakeCropMarksAIPlugin::AddMenuItems() {
    if (!sAIMenu) return 1;

    // 1. Window Menu item (toggles panel)
    AIPlatformAddMenuItemDataUS windowData;
    windowData.groupName = kToolPalettesMenuGroup;
    windowData.itemText = ai::UnicodeString("CropMarksAI");
    sAIMenu->AddMenuItem(m_pluginRef, "CropMarksAI_Window", &windowData, kMenuItemWantsUpdateOption, &m_hMenuWindow);

    // 2. Object Menu items
    AIPlatformAddMenuItemDataUS objCreateData;
    objCreateData.groupName = kObjectUtilsMenuGroup;
    objCreateData.itemText = ai::UnicodeString("Create Crop Marks...");
    sAIMenu->AddMenuItem(m_pluginRef, "CropMarksAI_Create", &objCreateData, kMenuItemNoOptions, &m_hMenuCreate);

    AIPlatformAddMenuItemDataUS objQuickData;
    objQuickData.groupName = kObjectUtilsMenuGroup;
    objQuickData.itemText = ai::UnicodeString("Quick Apply Crop Marks\tCtrl+Shift+F4");
    sAIMenu->AddMenuItem(m_pluginRef, "CropMarksAI_Quick", &objQuickData, kMenuItemNoOptions, &m_hMenuQuickApply);

    AIPlatformAddMenuItemDataUS objSettingsData;
    objSettingsData.groupName = kObjectUtilsMenuGroup;
    objSettingsData.itemText = ai::UnicodeString("CropMarksAI Settings...");
    sAIMenu->AddMenuItem(m_pluginRef, "CropMarksAI_Settings", &objSettingsData, kMenuItemNoOptions, &m_hMenuSettings);

    return kSPNoError;
}

SPErr MakeCropMarksAIPlugin::GoMenuItem(void* message) {
    AIMenuMessage* menuMsg = static_cast<AIMenuMessage*>(message);
    if (!menuMsg) return 1;

    AppContext appContext(m_pluginRef);

    if (menuMsg->menuItem == m_hMenuWindow) {
        TogglePanel();
    } else if (menuMsg->menuItem == m_hMenuCreate) {
        if (m_panelRef && sAIPanel) sAIPanel->Show(m_panelRef, true);
    } else if (menuMsg->menuItem == m_hMenuQuickApply) {
        ExecuteQuickApply();
    } else if (menuMsg->menuItem == m_hMenuSettings) {
        float scale = GetPanelScale(m_hPanelDlg);
        ConfigManager::GetInstance().ShowSettingsDialog(m_hPanelDlg, scale);
    }
    return kSPNoError;
}

SPErr MakeCropMarksAIPlugin::UpdateMenuItem(void* message) {
    AIMenuMessage* menuMsg = static_cast<AIMenuMessage*>(message);
    if (!menuMsg || !sAIMenu) return 1;

    if (menuMsg->menuItem == m_hMenuWindow && m_panelRef && sAIPanel) {
        AIBoolean isShown = false;
        sAIPanel->IsShown(m_panelRef, isShown);
        sAIMenu->CheckItem(m_hMenuWindow, isShown != 0);
    }
    return kSPNoError;
}

void MakeCropMarksAIPlugin::OnVisibilityChanged(bool isVisible) {
    if (m_hMenuWindow && sAIMenu) {
        sAIMenu->CheckItem(m_hMenuWindow, isVisible);
    }
}

