# CropMarksAI 📐🚀

[![Adobe Illustrator](https://img.shields.io/badge/Adobe%20Illustrator-CC%202020%20--%202026%2B-FF9A00?style=for-the-badge&logo=adobeillustrator&logoColor=white)](https://www.adobe.com/products/illustrator.html)
[![Language](https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-blue?style=for-the-badge&logo=windows&logoColor=white)](https://microsoft.com)
[![Version](https://img.shields.io/badge/Version-v1.37-success?style=for-the-badge)](https://github.com/SaidAuita/CropMarksAI/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](LICENSE)

> **High-performance native C++ prepress plugin (`.aip`) for Adobe Illustrator (CC 2020 – 2026+ x64).**  
> Generates precision vector crop marks, fold marks, bleed offsets, Anosov registration crosshairs, ISO & 100% solid color bars with spot/Pantone scanning, and Cyrillic-transliterated document info banners in sub-millisecond time (<1 ms) directly in Illustrator process memory.

<p align="center">
  <img src="images/CropMarksAI_01.png" alt="CropMarksAI Main Panel and Perimeter Marks" width="900">
</p>

---

[English](#english) • [Русский](#-русский)

---

<a name="english"></a>
## English

> [!TIP]
> ### 📥 Download Latest Release
> **[CropMarksAI_Plugin_v1.37.zip](https://github.com/SaidAuita/CropMarksAI/releases/download/v1.37/CropMarksAI_Plugin_v1.37.zip)** *(Direct ZIP package with one-click installer)*  
> Full release notes & assets: **[Release v1.37](https://github.com/SaidAuita/CropMarksAI/releases/tag/v1.37)**

### ✨ What's New in v1.37
- **Clipping Masks Support in Nested Groups**: Fixed bounding box calculation for clipping masks (`kArtIsClipMask` / `GetGroupClipped` / `IsArtClipping`) — crop marks are now strictly placed along the visible clipping contour even when masks are nested inside regular groups, ignoring clipped-out hidden artwork.
- **Document Info Fix**: Fixed empty text frame creation in the Document Info stamp by passing the exact UTF-16 string length to `ATE::TextRangeSuite::InsertAfter_AsUnicode`.
- **Automatic Spot / Pantone Refresh**: Added `kAIDocumentNewNotifier` / `kAIDocumentOpenedNotifier` tracking and proper `AppContext` binding when switching to the `[ Pantones ]` tab or clicking `Rescan`, ensuring Pantone/spot colors always refresh accurately across documents.

---

### 🖥️ Unified 3-Tab Interface & Settings

<table>
  <tr>
    <td align="center" width="33%">
      <img src="images/CropMarksAI_01.png" width="280" alt="Tab 1: Marks" /><br/>
      <b>[ Marks ]</b><br/>
      <sub>Crop marks, bleed, presets & directions</sub>
    </td>
    <td align="center" width="33%">
      <img src="images/CropMarksAI_02.png" width="280" alt="Tab 2: Pantones" /><br/>
      <b>[ Pantones ]</b><br/>
      <sub>Spot color scanning & smart filter</sub>
    </td>
    <td align="center" width="33%">
      <img src="images/CropMarksAI_03.png" width="280" alt="Tab 3: Settings" /><br/>
      <b>[ Settings ]</b><br/>
      <sub>Anosov crosshairs, color bars & doc info</sub>
    </td>
  </tr>
</table>

<p align="center">
  <img src="images/CropMarksAI_04.png" alt="CropMarksAI Settings and Hotkeys Dialog" width="460"><br/>
  <em>CropMarksAI Settings & Hotkeys Modal Dialog (<code>⚙</code>)</em>
</p>

---

### ⚡ Key Features

1. **Ultra-Fast Native Performance (< 1 ms)**:
   - Direct geometry calculation and vector path construction inside `Illustrator.exe` address space using native Illustrator SDK suites (`sAIArt`, `sAIPath`, `sAIPathStyle`).
   - Up to 100× faster than traditional ExtendScript / JSX scripts on complex layouts with hundreds of objects.

2. **Unified 3-Tab Palette Architecture**:
   - **`[ Marks ]` Tab**: 12 independent crop directions, geometric settings (Length, Offset, Bleed, Stroke Width) with unit selector (mm, pt, in, px), Target selector (Each object / Entire selection / Artboard), Bounds mode, Grouping mode, Layer selector, Only perimeter mode, white knockout underlay, quick presets, and instant `CREATE CROP MARKS` action.
   - **`[ Pantones ]` Tab**: Process CMYK toggles (C, M, Y, K) and live document spot/Pantone scanning with smart technical ink filtering.
   - **`[ Settings ]` Tab**: Color bar modes & geometry (including customizable repeat gap), Anosov registration marks configuration, prepress document info with Cyrillic transliteration, export/import configuration, and default hotkeys.

3. **Spot / Pantone Scanning & Smart Technical Ink Filter**:
   - **Automatic Swatch Inspection**: Scans all custom and spot inks directly from document swatches via `sAISwatchList` and `sAICustomColor`.
   - **Intelligent Technical Color Filter (`IsTechnicalSpotName`)**: Automatically detects and excludes non-printing and finishing inks (die-cuts, knives, creases, UV varnishes, foils, embossing, white ink underlays, adhesives — e.g. *штамп, вырубка, нож, биг, лак, уф, фольга, клише, белила, клей, die, cut, crease, varnish, foil, white*).
   - **Interactive Selection**: Checklist of all detected spots with live patch swatches, one-click `Rescan`, `All`, `None`, and `Filter` buttons, and `APPLY PANTONES`.

4. **Entire Selection Mode (1 Box) & Inward Packaging Die Cuts**:
   - **Entire Selection (1 Box)**: Single unified bounding box mode for complex die-cut stamps, packaging, and carton layouts composed of hundreds of individual vector paths. Generates clean corner marks strictly around the outer boundary of the entire selection without any unwanted interior marks.
   - **Inward Corner Crop Marks (Anosov Die Model)**: Supports negative lengths (`Length V/H < 0`, e.g. `-6 mm`) with zero offset (`Offset = 0 mm`) to draw corner cuts directed inward into die stamps.

5. **Hierarchical 3-Level Grouping (`Cuts`, `Registration Marks`, `Color Bars`)**:
   - Internal structured nesting within the main `"Crop Marks"` container group: cuts are gathered into a `"Cuts"` subgroup, registration marks into `"Registration Marks"`, and color bars with document info into `"Color Bars"`.
   - Allows users to easily select and move just the color bar or just the registration crosshairs using Illustrator's Group Selection Tool (Direct Selection Tool / White Arrow with plus) without breaking the overall group.

6. **Target Layer Selector & Auto-Creation**:
   - Choose where to place the marks: `[ Current Layer ]`, automatically create a dedicated prepress layer `[ New: Crop Marks ]`, or target any existing document layer from a drop-down list.

7. **Export & Import Settings (JSON)**:
   - Save and restore presets and settings to external JSON files (`Export...` / `Import...` buttons in the Settings tab). Enables seamless backup, team sharing, and effortless migration of user preferences across plugin updates or different machines.

8. **Advanced Color Bars & 100% Primaries Mode**:
   - **100% Primaries Only (`CMYK + Spots, no overprints`)**: Clean 100% solid patches without halftones or binary/tertiary overprints (Red, Green, Blue, 3C gray).
   - **100% Solids Mode (`CMYK + Pantones`)**: Pure 100% solid patches with binary overprints (Red, Green, Blue, 3C grey) and document Pantones.
   - **ISO 12647-2 (Commercial Offset)** & **ISO 12647-7 (Digital Proofing)** standards.
   - **Customizable Repeat Gap (`Gap`, default 0.0 mm)**: Distance between repeated color bar sequences can be freely configured (0 mm for continuous strips or any custom offset).
   - **Safety Boundary**: Starts +3 mm from first crop mark, stops -3 mm before the last crop mark.

9. **Registration Marks (S. Anosov Prepress Model)**:
   - Precision prepress targets with dual concentric rings and crosshairs. Configurable circle diameter, arm length, line width (mm/pt/in/px), separate H/V offsets, and 4 placement schemes (Centers, Corners, Centers+Corners, and All 16 Directions). Includes white knockout underlay (+1.2 pt) and Z-order control.

10. **Prepress Document Info Banner with Cyrillic Transliteration**:
    - Automatically prints document filename, date, and timestamp in 5 pt Registration color positioned 1 mm above the top color bar.
    - **Transliterate Cyrillic to Latin (`[x] Transliterate Cyrillic to Latin`)**: Automatically transliterates Cyrillic document names into clean Latin characters, eliminating RIP, PostScript, and platesetter font encoding glitches.

11. **Only Perimeter Mode & Spatial Auto-Clustering**:
    - **Only Perimeter (No Inner Marks)**: Detects grouped layouts (cards, labels, stickers) and places marks strictly along the outer bounding perimeter, eliminating unwanted internal cuts.
    - **Multi-Artboard Spatial Clustering**: Automatically isolates imposition grids across multiple artboards into dedicated groups without cross-artboard interference. Line deduplication (0.002 pt) and clipping mask detection.

12. **8 Quick Presets & Synchronized Hotkeys**:
    - **8 Fast Preset Slots (`[1]` .. `[8]`)** with custom names. Quick Apply (`Shift + F4` / `F4`) automatically synchronizes active UI parameters, checkboxes, and preset parameters. Fully compatible with Adobe Illustrator CC 2020 through 2027+.

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

### ⌨️ Default Hotkeys & Preferences

| Shortcut | Action | Description |
|---|---|---|
| **F4** | Toggle Panel | Shows or hides the CropMarksAI dockable palette |
| **Shift + F4** | Quick Apply | Instantly creates crop marks on selection using active preset and prepress options |
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

### 🛠️ Other Projects

**[Clipboard Paste Native Plugin](https://github.com/SaidAuita/AI-Clipboard-Paste)**
* Ultra-fast in-memory vector duplication for Adobe Illustrator without OS clipboard overhead. Hotkeys F5–F8.

**[AI Dimension](https://github.com/SaidAuita/AI-Dimension)**
* Automatic dimensioning, gap measuring, and technical callout generator for Adobe Illustrator.

**[ID Dimension](https://github.com/SaidAuita/ID-Dimension)**
* Technical dimensioning and drafting annotation tool for Adobe InDesign (includes both CEP panel and ScriptUI palette).

**[ComfyUI Photoshop Plugin (PH-CU-S)](https://github.com/SaidAuita/ComfyUI_PH-CU-S)**
* A powerful Photoshop plugin powered by ComfyUI, providing direct integration with local generative models without any clouds, subscriptions, or recurring fees.

### 💎 QR Code with Custom Shape Logo

<table>
  <tr>
    <td align="center" width="50%">
      <a href="https://ph-cu-s.com/ai-code-pro">
        <img src="images/ai_code_pro.png" width="130" alt="AI Code PRO - Custom Shape Logo QR Code" /><br/>
        <b>AI Code PRO</b>
      </a><br/>
      <sub>Vector Barcodes (18 Standards) & Designer QR Codes for <b>Adobe Illustrator</b></sub><br/><br/>
      <a href="https://ph-cu-s.com/ai-code-pro"><img src="https://img.shields.io/badge/PRO-Illustrator%20Extension-orange.svg" alt="AI Code PRO" /></a>
    </td>
    <td align="center" width="50%">
      <a href="https://ph-cu-s.com/id-code-pro">
        <img src="images/id_code_pro.png" width="130" alt="ID Code PRO - Custom Shape Logo QR Code" /><br/>
        <b>ID Code PRO</b>
      </a><br/>
      <sub>Vector Barcodes (18 Standards) & Designer QR Codes for <b>Adobe InDesign</b></sub><br/><br/>
      <a href="https://ph-cu-s.com/id-code-pro"><img src="https://img.shields.io/badge/PRO-InDesign%20Extension-purple.svg" alt="ID Code PRO" /></a>
    </td>
  </tr>
</table>

---

### 🙏 Acknowledgments

Special thanks to **Sergey Anosov** for his foundational prepress scripting work and inspiration (`MakeCropMarksAI.jsx` and `MakeRegMarksAI.jsx`):
* **Sergey Anosov DTP Scripts:** [sites.google.com/site/dtpscripting](https://sites.google.com/site/dtpscripting)

---

<a name="-русский"></a>
## 🇷🇺 Русский

> [!TIP]
> ### 📥 Скачать последнюю версию плагина
> **[CropMarksAI_Plugin_v1.37.zip](https://github.com/SaidAuita/CropMarksAI/releases/download/v1.37/CropMarksAI_Plugin_v1.37.zip)** *(Прямая загрузка архива с автоматическим установщиком)*  
> Страница релиза и список изменений: **[Релиз v1.37](https://github.com/SaidAuita/CropMarksAI/releases/tag/v1.37)**

### ✨ Что нового в v1.37
- **Корректная работа с обтравочными масками (включая вложенные группы)**: Исправлен расчёт границ объектов с масками (`AIGroupSuite::GetGroupClipped`, `IsArtClipping`, рекурсивный обход групп). Метки обреза теперь строятся строго по контуру маски и не захватывают скрытые за маской элементы.
- **Исправление блока Document Info**: Устранена ошибка создания пустого текстового объекта без текста — в `ATE::TextRangeSuite::InsertAfter_AsUnicode` теперь передаётся точная длина UTF-16 строки вместо `-1`.
- **Автообновление списка пантонов при смене документов**: Добавлено отслеживание открытия/создания документов (`kAIDocumentOpenedNotifier`, `kAIDocumentNewNotifier`) и привязка `AppContext` при переключении вкладки `[ Pantones ]` и нажатии `Rescan`, чтобы спот-цвета (включая `PANTONE 877 C`) всегда корректно отображались и обновлялись.

---

### 🖥️ Единый 3-вкладочный интерфейс и настройки

<table>
  <tr>
    <td align="center" width="33%">
      <img src="images/CropMarksAI_01.png" width="280" alt="Вкладка 1: Метки" /><br/>
      <b>[ Marks ]</b><br/>
      <sub>Метки реза, вылеты, 8 пресетов и 12 направлений</sub>
    </td>
    <td align="center" width="33%">
      <img src="images/CropMarksAI_02.png" width="280" alt="Вкладка 2: Пантоны" /><br/>
      <b>[ Pantones ]</b><br/>
      <sub>Сканирование спот-красок и фильтр техцветов</sub>
    </td>
    <td align="center" width="33%">
      <img src="images/CropMarksAI_03.png" width="280" alt="Вкладка 3: Настройки" /><br/>
      <b>[ Settings ]</b><br/>
      <sub>Кресты Аносова, шкалы, транслитерация кириллицы</sub>
    </td>
  </tr>
</table>

<p align="center">
  <img src="images/CropMarksAI_04.png" alt="Диалог настроек горячих клавиш CropMarksAI" width="460"><br/>
  <em>Модальное окно параметров и глобальных хоткеев (<code>⚙</code>)</em>
</p>

---

### ⚡ Преимущества и возможности

1. **Мгновенная скорость (< 1 мс)**:
   - Прямой расчёт геометрии и создание векторных путей внутри адресного пространства процесса `Illustrator.exe` через официальный нативный C++ SDK (`sAIArt`, `sAIPath`, `sAIPathStyle`).
   - В десятки раз быстрее традиционных ExtendScript / JSX скриптов на плотных спусках полос и макетах с сотнями объектов.

2. **Единая 3-вкладочная архитектура палитры**:
   - **Вкладка `[ Marks ]`**: 12 независимых направлений меток реза, геометрия (длина, смещение, вылеты, толщина обводки) с выбором единиц (мм, pt, дюймы, px), выбор цели (каждый объект / всё выделение целиком / артборд), режим границ, группировка, выбор слоя, режим периметра, белая подложка, переключение 8 именованных пресетов и кнопка прямого построения `CREATE CROP MARKS`.
   - **Вкладка `[ Pantones ]`**: Выбор триадных красок CMYK (C, M, Y, K) и автоматическое сканирование спот-красок документа с умной фильтрацией.
   - **Вкладка `[ Settings ]`**: Тонкая настройка контрольных шкал (включая настраиваемый шаг повтора), параметров приводных крестов модели Аносова, инфоблока документа с транслитерацией кириллицы, экспорт/импорт настроек и системные хоткеи.

3. **Сканирование пантонов и умный фильтр технических цветов**:
   - **Автоматический опрос каталога свотчей**: Считывание всех плашечных / спот-красок документа через SDK (`sAISwatchList`, `sAICustomColor`).
   - **Умная классификация технических слоев (`IsTechnicalSpotName`)**: Автоматическое распознавание и исключение из контрольной шкалы непечатных и отделочных красок (штамп, вырубка, нож, биг, лак, уф, фольга, клише, белила, клей, die, cut, crease, varnish, foil, white и т.д.).
   - **Интерактивный список**: Список обнаруженных красок с чекбоксами, кнопками быстрого выбора `Rescan`, `All`, `None`, `Filter` и кнопкой `APPLY PANTONES`.

4. **Режим «Entire selection (1 box)» и внутренние метки для штампов**:
   - **Единый габарит выделения**: Режим расчета по общему внешнему габариту всего выделения для сложных вырубных штампов, коробок и упаковки (даже состоящих из сотен разрозненных контуров). Метки выставляются строго по 4 внешним углам без паразитных внутренних засечек.
   - **Внутренние угловые метки штампов (Модель Аносова)**: Полная поддержка отрицательной длины (`Length V/H < 0`, например `-6 мм`) при нулевом отступе (`Offset = 0 мм`) для отрисовки угловых засечек, направленных внутрь контура штампа.

5. **Трехуровневая иерархическая группировка (`Cuts`, `Registration Marks`, `Color Bars`)**:
   - Внутри общей группы `"Crop Marks"` создается четкая структура: метки реза объединяются в подгруппу `"Cuts"`, кресты совмещения — в `"Registration Marks"`, а шкалы и служебный текст — в `"Color Bars"`.
   - Позволяет инструментом «Групповое выделение» (Group Selection Tool / белая стрелка с плюсом) одним кликом выделить и сдвинуть, например, только цветовую шкалу или только приводные кресты, не разгруппировывая весь макет.

6. **Выбор целевого слоя и автосоздание (`Layer`)**:
   - Выпадающий список выбора слоя: размещение меток на текущем активном слое (`[ Current Layer ]`), автоматическое создание выделенного слоя допечатной подготовки (`[ New: Crop Marks ]`) или отправка меток на любой существующий слой документа.

7. **Экспорт и импорт настроек (JSON)**:
   - Сохранение и загрузка пресетов и параметров в файл JSON (кнопки `Export...` и `Import...` на вкладке Settings). Позволяет легко переносить настройки между версиями плагина, синхронизировать конфигурацию на нескольких рабочих станциях и делать резервные копии.

8. **Контрольные шкалы и режим 100% чистых плашек**:
   - **Режим 100% Primaries (`CMYK + Spots, no overprints`)**: Только чистые 100% плашки печатных красок без полутонов и без бинарных наложений (Red, Green, Blue, 3C серый).
   - **Режим 100% Solids (`CMYK + Pantones`)**: 100% плашки с бинарными наложениями и спот-цветами.
   - **Стандарты ISO 12647-2 (Офсетная печать)** и **ISO 12647-7 (Цветопроба)**.
   - **Настраиваемый зазор повтора шкалы (`Gap`, по умолчанию 0.0 мм)**: Возможность задать расстояние между повторяющимися блоками шкалы (0 мм для сплошной непрерывной ленты или любое необходимое расстояние).
   - **Защитные поля**: Старт на +3 мм от первой обрезной метки, остановка за -3 мм до последней метки.

9. **Приводные метки / кресты (Полиграфическая модель С. Аносова)**:
   - Высокоточные прицелы с двойным концентрическим кольцом и крестом. Диаметр кольца, размах перекрестия, толщина обводки (мм/pt/in/px), раздельные смещения H/V и 4 схемы (центры, углы, центры+углы, 16 направлений по периметру). Белая маскирующая контурная подложка (+1.2 pt) и выбор Z-Order.

10. **Служебный блок Document Info с транслитерацией кириллицы**:
    - Автоматический вывод имени файла, текущей даты и точного времени шрифтом 5 pt краской Registration с отступом 1 мм над верхней шкалой.
    - **Транслитерация кириллицы в латиницу (`[x] Transliterate Cyrillic to Latin`)**: Исключает сбои кодировок, кракозябры и ошибки в растровых процессорах (RIP), фотовыводных автоматах и CTP при выводе печатных форм.

11. **Режим «Только периметр» и мульти-артбордные спуски**:
    - **Режим Only perimeter**: Расставляет метки строго по внешнему периметру выделенной группы объектов, исключая внутренние паразитные засечки.
    - **Пространственная кластеризация**: Автоматически группирует спуски по артбордам. Дедупликация линий (0.002 pt) и учет обтравочных масок.

12. **8 быстрых пресетов и синхронизация хоткеев**:
    - **8 слотов быстрых пресетов (`[1]` .. `[8]`)** с именами. Синхронизация хоткеев (`Shift + F4` / `F4`). Полная совместимость с Adobe Illustrator CC 2020 – 2027+.

---

### 📦 Установка

#### Автоматическая установка (Рекомендуется):
1. Закройте Adobe Illustrator.
2. Запустите **`Install_Plugin.bat`** от имени Администратора.
3. Инсталлятор автоматически найдет установленные версии Illustrator (`CC 2020` .. `2026+`) и скопирует плагин `CropMarksAI.aip` в папки `Plug-ins\`.
4. Запустите Illustrator. Палитра доступна через меню **`Window` ➔ `CropMarksAI`** или по клавише **F4**.

#### Ручная установка:
Скопируйте `CropMarksAI.aip` в системную папку плагинов:
```
C:\Program Files\Adobe\Adobe Illustrator [Версия]\Plug-ins\
```

Для удаления запустите **`Uninstall_Plugin.bat`** от имени Администратора.

---

### ⌨️ Горячие клавиши по умолчанию

| Хоткей | Действие | Описание |
|---|---|---|
| **F4** | Открыть / свернуть панель | Открывает или сворачивает докируемую палитру CropMarksAI |
| **Shift + F4** | Быстрое применение | Мгновенно расставляет метки, кресты и шкалы по активному пресету к выделению |
| **Enter** *(в поле)* | Создать / Сохранить | В числовых полях запускает расстановку; в поле названия сохраняет пресет |
| **Tab** / **Shift+Tab** | Навигация | Перемещение фокуса ввода между полями и кнопками |
| **Ctrl + Z** | Отмена | Полная отмена действия в один шаг истории |

---

### 🛠️ Другие проекты (My Projects)

**[Clipboard Paste Native Plugin](https://github.com/SaidAuita/AI-Clipboard-Paste)**
* Сверхбыстрое дублирование векторных объектов в памяти Illustrator ядра SweetPea без задействования системного буфера Windows. Хоткеи F5–F8.

**[AI Dimension](https://github.com/SaidAuita/AI-Dimension)**
* Автоматическая простановка технических размеров, зазоров и выносных линий для Adobe Illustrator.

**[ID Dimension](https://github.com/SaidAuita/ID-Dimension)**
* Инструмент простановки чертежных размеров и аннотаций для Adobe InDesign (включает панель CEP и скрипт ScriptUI).

**[ComfyUI Photoshop Plugin (PH-CU-S)](https://github.com/SaidAuita/ComfyUI_PH-CU-S)**
* Мощный плагин для Photoshop на базе ComfyUI, обеспечивающий прямую интеграцию с локальными генеративными моделями без подписок и облаков.

### 💎 QR код с логотипом произвольной формы

<table>
  <tr>
    <td align="center" width="50%">
      <a href="https://ph-cu-s.com/ru/ai-code-pro">
        <img src="images/ai_code_pro_ru.png" width="130" alt="AI Code PRO - QR код с логотипом произвольной формы" /><br/>
        <b>AI Code PRO</b>
      </a><br/>
      <sub>Генератор векторных штрихкодов (18 стандартов) и дизайнерских QR-кодов для <b>Adobe Illustrator</b></sub><br/><br/>
      <a href="https://ph-cu-s.com/ru/ai-code-pro"><img src="https://img.shields.io/badge/PRO-Расширение%20Illustrator-orange.svg" alt="AI Code PRO" /></a>
    </td>
    <td align="center" width="50%">
      <a href="https://ph-cu-s.com/ru/id-code-pro">
        <img src="images/id_code_pro_ru.png" width="130" alt="ID Code PRO - QR код с логотипом произвольной формы" /><br/>
        <b>ID Code PRO</b>
      </a><br/>
      <sub>Генератор векторных штрихкодов (18 стандартов) и дизайнерских QR-кодов для <b>Adobe InDesign</b></sub><br/><br/>
      <a href="https://ph-cu-s.com/ru/id-code-pro"><img src="https://img.shields.io/badge/PRO-Расширение%20InDesign-purple.svg" alt="ID Code PRO" /></a>
    </td>
  </tr>
</table>

---

### 🙏 Благодарности (Acknowledgments)

Выражаю особую благодарность **Сергею Аносову** за многолетний фундаментальный вклад в автоматизацию допечатной подготовки для Adobe Illustrator и вдохновение для создания плагина (скрипты `MakeCropMarksAI.jsx` и `MakeRegMarksAI.jsx`):
* **Скрипты Сергея Аносова для DTP:** [sites.google.com/site/dtpscripting](https://sites.google.com/site/dtpscripting)

---

## 📄 License & Author

* **Author:** Said Auita ([GitHub: @SaidAuita](https://github.com/SaidAuita))
* **Inspiration & Prepress Algorithms:** Sergey Anosov ([sites.google.com/site/dtpscripting](https://sites.google.com/site/dtpscripting))
* **Project Repository:** [github.com/SaidAuita/CropMarksAI](https://github.com/SaidAuita/CropMarksAI)
* **Prepress Automation Hub:** [ph-cu-s.com/tools](https://ph-cu-s.com/tools)
* **License:** [MIT License](LICENSE)
