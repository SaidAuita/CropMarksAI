# CropMarksAI 📐🚀

[![Adobe Illustrator](https://img.shields.io/badge/Adobe%20Illustrator-CC%202020%20--%202026%2B-FF9A00?style=for-the-badge&logo=adobeillustrator&logoColor=white)](https://www.adobe.com/products/illustrator.html)
[![Language](https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-blue?style=for-the-badge&logo=windows&logoColor=white)](https://microsoft.com)
[![Version](https://img.shields.io/badge/Version-v1.07-success?style=for-the-badge)](https://github.com/SaidAuita/CropMarksAI/releases)
[![License](https://img.shields.io/badge/License-Proprietary%20%2F%20Freeware-informational?style=for-the-badge)](LICENSE)

> **High-performance native C++ prepress plugin (`.aip`) for Adobe Illustrator (CC 2020 – 2026+ x64).**  
> Generates precision vector crop marks, fold marks, bleed offsets, and knockout underlays in sub-millisecond time (<1 ms) directly in Illustrator process memory.

---

[English](#english) • [Русский](#русский)

---

<a name="english"></a>
## English

### ⚡ Key Features

1. **Ultra-Fast Native Performance (< 1 ms)**:
   - Direct geometry calculation and vector path construction inside `Illustrator.exe` address space using native Illustrator SDK suites (`sAIArt`, `sAIPath`, `sAIPathStyle`).
   - Up to 100× faster than traditional ExtendScript / JSX scripts on complex layouts with hundreds of objects.

2. **12 Independent Crop Mark Directions**:
   - Corner marks: Top-Left (`TL`), Top-Right (`TR`), Bottom-Left (`BL`), Bottom-Right (`BR`).
   - Side marks: Left-Top (`LT`), Left-Bottom (`LB`), Right-Top (`RT`), Right-Bottom (`RB`).
   - Center / fold marks: Top-Center (`TC`), Bottom-Center (`BC`), Left-Center (`LC`), Right-Center (`RC`).
   - One-click direction presets: `All`, `Corners`, `Clear`.

3. **Prepress Precision & Print Standards**:
   - **Registration Color**: Strokes are assigned true 100% Registration color (`sAICustomColor->GetCurrentRegistrationColor`), rendering on all separation plates.
   - **White Knockout Contour Underlay (`+1 pt`)**: Automatic white background stroke placed underneath the registration line for maximum visibility on rich black and dark backgrounds.
   - **Only Perimeter Mode (No Inner Marks)**: Creates marks strictly along the outer bounding perimeter of grouped layouts (cards, stickers, impositions), preventing unwanted internal cut lines.
   - **Line Deduplication**: Positional cache eliminates duplicate overlapping lines with a precision tolerance of `0.002 pt`.
   - **Clipping Mask & Guide Awareness**: Accurately detects clipping masks (`kArtIsClipMask`) to bound crop marks to the visible artwork, completely ignoring non-printing guides.
   - **Send to Back**: Automatically reorders generated marks to the very bottom of the artwork stack (`kPlaceInsideOnBottom`).
   - **One-Step Undo**: Fully integrated into Illustrator's native history stack (`sAIUndo->SetUndoTextUS`).

4. **Adaptive UI & HiDPI / 4K Calibration**:
   - Matches native Illustrator palette scale (1:1 proportions with Stroke, Paragraph, and Character panels).
   - Reads Illustrator UI scale via `sAIUser->GetAppScaleFactor()` combined with Win32 Per-Monitor DPI (`WM_DPICHANGED`).
   - Supports all 4 Illustrator color themes (**Dark**, **Medium Dark**, **Medium Light**, **Light**) with instantaneous redraw on brightness changes (`kAIUIBrightnessChangedNotifier`).

5. **8 Quick Presets with Custom Naming**:
   - Fast switching between 8 custom preset slots (`[1]` .. `[8]`).
   - Editable preset name text field (`IDC_ED_PRESET_NAME`): name presets according to your shop standards (e.g., `Card 90x50`, `Flyer A4 Bleed 2mm`, `Packaging 6/4`).
   - Persistent storage in `%APPDATA%\MakeCropMarksAI\presets.json`.

6. **Customizable Hotkeys & Keyboard Hook**:
   - **F4** (default): Toggle dockable panel open/close.
   - **Shift + F4** (default): Quick Apply active preset to selection without opening the dialog.
   - **Tab** / **Shift + Tab**: Keyboard navigation between controls with automatic text selection.
   - **Enter**: When inside parameter edit fields, instantly generates crop marks. When inside the preset name field, instantly saves the preset.
   - Optional `Disable plugin` toggle in settings (`⚙`) to suspend keyboard hooks.

---

### 📦 Installation

#### Automatic Installation (Recommended):
1. Make sure Adobe Illustrator is closed.
2. Run **`Install_Plugin.bat`** as Administrator.
3. The interactive installer will scan your system for all installed Adobe Illustrator versions (`CC 2020` .. `2026+`) and copy `CropMarksAI.aip` into the corresponding `Plug-ins\` directories.
4. Launch Illustrator. Find the panel under **`Window` ➔ `CropMarksAI`** or press **F4**.

#### Manual Installation:
Copy `CropMarksAI.aip` directly into:
```
C:\Program Files\Adobe\Adobe Illustrator [Version]\Plug-ins\
```

To uninstall, simply run **`Uninstall_Plugin.bat`** as Administrator.

---

### ⌨️ Default Hotkeys

| Shortcut | Action | Description |
|---|---|---|
| **F4** | Toggle Panel | Shows or hides the CropMarksAI dockable palette |
| **Shift + F4** | Quick Apply | Instantly creates crop marks on selection using active preset |
| **Enter** *(in field)* | Create / Save | Creates marks from numeric fields; saves preset from name field |
| **Tab** / **Shift+Tab** | Navigate | Moves keyboard focus through input fields and buttons |
| **Ctrl + Z** | Undo | Restores artwork state in a single undo step |

---

### 🛠️ Building from Source

- **Requirements**:
  - Microsoft Visual Studio 2022 (MSVC v143, C++17 toolset)
  - Adobe Illustrator 2026 SDK (or compatible 2020–2025 SDK)
  - Windows 10 / 11 SDK (x64)
- **Compilation**:
  Run `build.bat` from Developer Command Prompt for VS 2022, or run `node build_dev.js` to compile the `.aip` binary and assemble the release package in `Build_DEV/`.

---

<a name="русский"></a>
## Русский

### ⚡ Преимущества и возможности

1. **Мгновенная работа (< 1 мс)**:
   - Прямой расчёт геометрии и создание векторных путей внутри адресного пространства `Illustrator.exe` через нативный C++ SDK (`sAIArt`, `sAIPath`, `sAIPathStyle`).
   - В десятки раз быстрее традиционных ExtendScript / JSX скриптов на плотных спусках полос и макетах с тысячами элементов.

2. **12 независимых направлений меток реза**:
   - Угловые метки: верхний левый (`TL`), верхний правый (`TR`), нижний левый (`BL`), нижний правый (`BR`).
   - Боковые метки: левый верх (`LT`), левый низ (`LB`), правый верх (`RT`), правый низ (`RB`).
   - Центральные / биговочные метки: верх центр (`TC`), низ центр (`BC`), лево центр (`LC`), право центр (`RC`).
   - Быстрые кнопки выбора: `All` (все), `Corners` (только угловые), `Clear` (снять все).

3. **Полиграфическая точность и допечатные стандарты**:
   - **Цвет Registration**: Меткам автоматически назначается 100% краска приводки Registration (`sAICustomColor->GetCurrentRegistrationColor`), выводящаяся на всех сепарациях.
   - **Белая маскирующая подложка (`+1 pt`)**: Дополнительная белая линия под меткой для идеальной читаемости на тёмных и насыщенных плашках.
   - **Режим "Only perimeter"**: Построение меток исключительно по внешнему габариту выделенного блока без лишних внутренних засечек между смежными карточками.
   - **Дедупликация линий**: Исключение наложения повторяющихся линий с точностью до `0.002 pt`.
   - **Обработка масок и направляющих**: Автоматическое определение обтравочных масок (`Clipping Mask`) и расчет меток по контуру видимого макета с полным игнорированием служебных направляющих.
   - **Send to Back**: Автоматическое опускание всех созданных меток в самый низ стопки слоев макета (`kPlaceInsideOnBottom`).
   - **Отмена в один шаг (Ctrl+Z)**: Полная интеграция в историю документа Illustrator.

4. **Адаптивный интерфейс и HiDPI / 4K калибровка**:
   - Точные нативные пропорции (1:1 с нативными палитрами Stroke, Paragraph, Character).
   - Поддержка пользовательского масштабирования Illustrator (`sAIUser->GetAppScaleFactor()`) и Per-Monitor DPI Windows (`WM_DPICHANGED`).
   - Поддержка всех 4 цветовых тем оформлений Illustrator (**Dark**, **Medium Dark**, **Medium Light**, **Light**) с мгновенной перерисовкой при смене темы (`kAIUIBrightnessChangedNotifier`).

5. **8 быстрых пресетов с произвольными именами**:
   - Переключение пресетов кнопками `[1]` .. `[8]`.
   - Редактируемое поле названия пресета: вписывайте понятные имена (`Визитка 90x50`, `Евробуклет 297x210`, `Вылет 3мм`, `Упаковка`).
   - Автоматическое сохранение в `%APPDATA%\MakeCropMarksAI\presets.json`.

6. **Горячие клавиши и фоновый хук**:
   - **F4**: Открытие / скрытие палитры плагина.
   - **Shift + F4**: Мгновенный Quick Apply по активному пресету без вызова диалога.
   - **Enter**: Запуск генерации меток из числовых полей; сохранение пресета из поля названия.
   - **Tab** / **Shift + Tab**: Удобная циклическая навигация по полям и кнопкам.
   - Опция `Disable plugin` в настройках (`⚙`) для временного отключения хука.

---

### 📦 Установка

1. Закройте Adobe Illustrator.
2. Запустите **`Install_Plugin.bat`** от имени Администратора.
3. Инсталлятор найдет установленные версии Illustrator (`CC 2020` .. `2026+`) и скопирует плагин `CropMarksAI.aip` в папки `Plug-ins\`.
4. Запустите Illustrator. Палитра доступна через меню **`Window` ➔ `CropMarksAI`** или по клавише **F4**.

---

## 🔗 Ссылки и ресурсы

- **Репозиторий проекта**: [github.com/SaidAuita/CropMarksAI](https://github.com/SaidAuita/CropMarksAI)
- **Каталог скриптов и плагинов**: [ph-cu-s.com/tools](https://ph-cu-s.com/tools)
- **Автор**: SaidAuita

---

### 📄 License

Copyright © 2026 SaidAuita. All rights reserved.
Distributed as freeware for prepress designers and print production specialists.
