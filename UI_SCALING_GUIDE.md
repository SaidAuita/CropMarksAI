# UI_SCALING_GUIDE: Стандарт масштабирования и цветовых схем нативных C++ плагинов Adobe Illustrator

Настоящее руководство фиксирует проверенные в продакшне стандарты создания адаптивного, масштабируемого графического интерфейса (GUI) для нативных C++ плагинов Adobe Illustrator (.aip) под Windows x64.

Данный стандарт гарантирует, что плагин выглядит на 100% органично как на стандартных FHD-мониторах (100% / 1.0x), так и на 2K/4K/HiDPI экранах (125%, 150%, 175%, 200%), а пропорции и цвета контролов в точности соответствуют нативным палитрам Illustrator (Stroke, Properties, Paragraph, Transform).

---

## 1. Двухуровневое определение коэффициента масштаба (Scale Factor)

В Illustrator масштабирование может задаваться как на уровне Windows (системный DPI: 125%, 150%), так и внутри самого приложения (`Preferences -> User Interface -> UI Scaling`: Small, Medium, Large).

Для корректного отображения итоговый масштаб берется как максимум из двух значений:

```cpp
#include "AIUser.h"

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
```

---

## 2. Типографика и калибровка шрифтов

Шрифты Win32 задаются в отрицательных пикселях для точной привязки к высоте символов (Character Height):

| Элемент UI | Базовый размер при 1.0x (FHD) | Размер при 1.5x (4K) | Начертание | Применение |
|---|---|---|---|---|
| **Интерфейсный (`szUI`)** | **-11.0 px** (~8.25 pt) | **-16.5 px** | Segoe UI Regular / Bold | Кнопки, поля ввода, выпадающие списки, метки, карточки статуса |
| **Заголовок (`szHeader`)** | **-13.0 px** (~9.75 pt) | **-19.5 px** | Segoe UI Bold | Название плагина в шапке панели |
| **Второстепенный (`szSmall`)** | **-10.0 px** (~7.5 pt) | **-15.0 px** | Segoe UI Regular | Бейдж версии, ссылки в футере панели |

### Функция обновления шрифтов:
```cpp
static HFONT g_hUIFont = NULL;
static HFONT g_hHeaderFont = NULL;
static HFONT g_hSmallFont = NULL;
static float g_appliedFontScale = 0.0f;

static void EnsureFontsForScale(HWND hwnd, float scale) {
    if (std::abs(g_appliedFontScale - scale) < 0.01f && g_hUIFont) return;
    g_appliedFontScale = scale;

    if (g_hUIFont)     { DeleteObject(g_hUIFont);     g_hUIFont = NULL; }
    if (g_hHeaderFont) { DeleteObject(g_hHeaderFont); g_hHeaderFont = NULL; }
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
    g_hSmallFont = CreateFontA(szSmall, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    if (hwnd && IsWindow(hwnd)) {
        // Передача актуального шрифта всем зарегистрированным контролам через SendMessage(h, WM_SETFONT, ...)
    }
}
```

---

## 3. Габариты и размеры элементов управления

Все координаты и размеры вычисляются через макрос-лямбду:
```cpp
auto S = [scale](int v) -> int {
    return (int)(v * scale + 0.5f);
};
```

### Стандарты высот и отступов:
- **Поле ввода (`Edit`) и выпадающий список (`ComboBox`)**:
  - Высота: `S(20)` (в точности соответствует 20–21 px полей `Weight:` в палитре Stroke).
  - Внутренний отступ текста: `S(4)` слева/справа.
  - Высота элементов ComboBox: `CB_SETITEMHEIGHT`: закрытый `-1` -> `S(22)`, пункт списка `0` -> `S(20)`.
- **Кнопки (`Button`)**:
  - Основные кнопки действий: `S(22)` (компактные) или `S(24)` / `S(26)` (большие акцентные).
  - Вспомогательные кнопки (Settings, Presets, Clear): `S(20)`.
  - Радиус скругления кнопок: `corner = S(3)` (диаметр `corner * 2`), скругление точно такое же, как у кнопки `Edit Artboards`.
- **Чекбоксы (`CheckBox`)**:
  - Размер квадрата: `S(12)` (соответствует нативным чекбоксам 11–12 px).
- **Карточка статуса / информационная плашка**:
  - Высота: `S(32)` – `S(34)`.
  - Внутренний отступ: по горизонтали `S(6)`, по вертикали `S(2)`.
  - Стиль текста: `DT_LEFT | DT_WORDBREAK`.
- **Отступы и границы панели**:
  - Внешний отступ от края панели (`pad`): `S(8)`.
  - Межстрочный интервал между группами: `S(3)` – `S(5)`.

---

## 4. Палитра цветов Illustrator (Адаптация к Dark и Light темам)

Плагин подписывается на смену яркости интерфейса через `kAIUIBrightnessChangedNotifier`:

```cpp
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
        // Dark / Medium Dark (Illustrator native)
        c.panelBg       = RGB(53, 53, 53);      // #353535
        c.textPrimary   = RGB(235, 235, 235);   // #ebebeb
        c.textSecondary = RGB(160, 160, 160);   // #a0a0a0
        c.btnBg         = RGB(68, 68, 68);      // #444444 (как Edit Artboards)
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
        // Light / Medium Light (Illustrator native)
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

    // Точная синхронизация цвета фона с ядром Illustrator
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
            c.btnText = c.textPrimary;
        }
    }

    return c;
}
```

---

## 5. Докирование панели (Dock Constraints)

В Adobe Illustrator SDK размеры палитр при докинге (`sAIPanel->SetSizes`) должны иметь ультра-компактный минимальный порог (`100x100`), чтобы панель свободно уменьшалась и сжималась пользователем влево:

```cpp
AISize minS = { 100, 100 };
AISize maxS = { 1200, 1200 };
AISize prefConstS = { 190, 235 };
AISize prefUnconstS = { 210, 240 };
sAIPanel->SetSizes(m_panelRef, minS, prefUnconstS, prefConstS, maxS);
```

### Обработка событий в `PanelWndProc`:
```cpp
case WM_DPICHANGED:
    RelayoutPanel(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
    return 0;

case WM_SIZE:
    RelayoutPanel(hwnd);
    break;
```

---

## 6. Масштабирование модальных Win32-диалогов (Settings / Preferences)

Для модальных окон настроек плагина критически важно соблюдать два правила:
1. **Синхронизация масштаба с вызывающей панелью**: передавать `currentScale` из панели (где уже учтены `sAIUser->GetAppScaleFactor()` и Per-Monitor DPI), чтобы диалог не жил собственной жизнью относительно Illustrator.
2. **Компактные базовые пропорции**: не задавать избыточные размеры. При 1.0x (FHD) диалог настроек должен быть компактным (~340×225 px), со шрифтом `-11px` (Segoe UI), высотой выпадающих списков `S(20)` (закрытый) и `S(18)` (пункт списка).

```cpp
bool ShowSettingsDialog(HWND parentHwnd, float currentScale = 0.0f) {
    // 1. Определение масштаба: приоритет имеет масштаб вызывающей панели
    float scale = currentScale;
    if (scale < 0.75f || scale > 4.0f) {
        scale = GetDialogScale(parentHwnd);
    }
    auto S = [scale](int v) -> int { return (int)(v * scale + 0.5f); };

    // 2. Компактные базовые габариты клиентской области
    int clientW = S(340);
    int clientH = S(224);
    RECT rc = { 0, 0, clientW, clientH };
    DWORD dwStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
    DWORD dwExStyle = WS_EX_DLGMODALFRAME | WS_EX_TOPMOST;
    AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);
    int dlgW = rc.right - rc.left;
    int dlgH = rc.bottom - rc.top;

    // 3. Центрирование относительно родительского окна Illustrator
    RECT rcParent;
    if (hOwner && IsWindow(hOwner)) GetWindowRect(hOwner, &rcParent);
    else GetWindowRect(GetDesktopWindow(), &rcParent);

    int x = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int y = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;

    // 4. Нативный шрифт интерфейса (-11px при 1.0x)
    int fontH = -(int)(11.0f * scale + 0.5f);
    HFONT hFont = CreateFontA(fontH, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
```

---

## 7. Чеклист проверки при создании нового плагина

1. [ ] В `Suites.h` подключены `#include "AIUser.h"`, `#include "AIUITheme.h"`, `#include "AINotifier.h"`.
2. [ ] В `Suites.cpp` выполнен захват `kAIUserSuite`, `kAIUIThemeSuite`, `kAINotifierSuite`.
3. [ ] Зарегистрирован нотификатор `kAIUIBrightnessChangedNotifier` в `StartupPlugin`.
4. [ ] Добавлена функция `GetPanelScale(hwnd)` с проверкой `sAIUser->GetAppScaleFactor()` и `GetDpiForWindow`.
5. [ ] Шрифты формируются динамически в `EnsureFontsForScale`: `-11px` (UI), `-13px` (Header bold), `-10px` (Small).
6. [ ] Кнопки имеют нативное скругление `S(3)` и высоту `S(20)` – `S(24)`.
7. [ ] В `RelayoutPanel` отсутствуют «голые» пиксели — все размеры обернуты в `S(...)`.
8. [ ] Обработаны `WM_DPICHANGED` и `WM_SIZE`.
9. [ ] Окна диалогов масштабируются через `AdjustWindowRectEx`.
10. [ ] Проверено переключение режимов Dark <-> Light в `Preferences -> User Interface`.
