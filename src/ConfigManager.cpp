#include "ConfigManager.h"
#include "Suites.h"
#include <shlobj.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>

#pragma comment(lib, "shell32.lib")

struct KeyEntry {
    UINT vk;
    const char* name;
};

static const KeyEntry g_supportedKeys[] = {
    { VK_F1,  "F1" },  { VK_F2,  "F2" },  { VK_F3,  "F3" },  { VK_F4,  "F4" },
    { VK_F5,  "F5" },  { VK_F6,  "F6" },  { VK_F7,  "F7" },  { VK_F8,  "F8" },
    { VK_F9,  "F9" },  { VK_F10, "F10" }, { VK_F11, "F11" }, { VK_F12, "F12" },
    { 'A', "A" }, { 'B', "B" }, { 'C', "C" }, { 'D', "D" }, { 'E', "E" },
    { 'F', "F" }, { 'G', "G" }, { 'H', "H" }, { 'I', "I" }, { 'J', "J" },
    { 'K', "K" }, { 'L', "L" }, { 'M', "M" }, { 'N', "N" }, { 'O', "O" },
    { 'P', "P" }, { 'Q', "Q" }, { 'R', "R" }, { 'S', "S" }, { 'T', "T" },
    { 'U', "U" }, { 'V', "V" }, { 'W', "W" }, { 'X', "X" }, { 'Y', "Y" }, { 'Z', "Z" },
    { '0', "0" }, { '1', "1" }, { '2', "2" }, { '3', "3" }, { '4', "4" },
    { '5', "5" }, { '6', "6" }, { '7', "7" }, { '8', "8" }, { '9', "9" },
    { VK_INSERT, "Insert" }, { VK_DELETE, "Delete" },
    { VK_HOME, "Home" },     { VK_END, "End" },
    { VK_PRIOR, "Page Up" }, { VK_NEXT, "Page Down" }
};

struct ModEntry {
    UINT mod;
    const char* name;
};

static const ModEntry g_supportedMods[] = {
    { 0, "None" },
    { MOD_CONTROL, "Ctrl" },
    { MOD_ALT, "Alt" },
    { MOD_SHIFT, "Shift" },
    { MOD_CONTROL | MOD_SHIFT, "Ctrl + Shift" },
    { MOD_CONTROL | MOD_ALT, "Ctrl + Alt" },
    { MOD_ALT | MOD_SHIFT, "Alt + Shift" },
    { MOD_CONTROL | MOD_ALT | MOD_SHIFT, "Ctrl + Alt + Shift" }
};

ConfigManager& ConfigManager::GetInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    m_openUiHotkey = { VK_F4, 0 };                 // F4
    m_quickApplyHotkey = { VK_F4, MOD_SHIFT };     // Shift + F4
    m_disabled = false;
    m_activePresetIndex = 0;

    InitDefaultPresets();
    Load();
}

std::string ConfigManager::GetAppDataFolder() const {
    char appData[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
        std::string folder = std::string(appData) + "\\MakeCropMarksAI";
        CreateDirectoryA(folder.c_str(), NULL);
        return folder;
    }
    return ".";
}

std::string ConfigManager::GetIniPath() const {
    return GetAppDataFolder() + "\\settings.ini";
}

std::string ConfigManager::GetPresetsJsonPath() const {
    return GetAppDataFolder() + "\\presets.json";
}

void ConfigManager::InitDefaultPresets() {
    m_presets.clear();
    m_presets.resize(8);

    // Preset 1: Default 5/2
    m_presets[0].id = 1;
    m_presets[0].name = "Default 5/2";
    m_presets[0].params.lengthVer = 5.0;
    m_presets[0].params.lengthHor = 5.0;
    m_presets[0].params.offsetVer = 2.0;
    m_presets[0].params.offsetHor = 2.0;
    m_presets[0].params.bleedVer = 0.0;
    m_presets[0].params.bleedHor = 0.0;
    m_presets[0].params.strokeWidth = 0.25;
    m_presets[0].params.units = kUnitsMM;
    m_presets[0].params.strokeUnits = kUnitsPT;

    // Preset 2: Standard 5/3/2
    m_presets[1].id = 2;
    m_presets[1].name = "Standard 5/3/2";
    m_presets[1].params.lengthVer = 5.0;
    m_presets[1].params.lengthHor = 5.0;
    m_presets[1].params.offsetVer = 3.0;
    m_presets[1].params.offsetHor = 3.0;
    m_presets[1].params.bleedVer = 2.0;
    m_presets[1].params.bleedHor = 2.0;
    m_presets[1].params.strokeWidth = 0.25;

    // Preset 3: Card 90x50
    m_presets[2].id = 3;
    m_presets[2].name = "Card 90x50";
    m_presets[2].params.lengthVer = 4.0;
    m_presets[2].params.lengthHor = 4.0;
    m_presets[2].params.offsetVer = 2.0;
    m_presets[2].params.offsetHor = 2.0;
    m_presets[2].params.bleedVer = 2.0;
    m_presets[2].params.bleedHor = 2.0;
    m_presets[2].params.strokeWidth = 0.25;

    // Preset 4: Flyer A4
    m_presets[3].id = 4;
    m_presets[3].name = "Flyer A4";
    m_presets[3].params.lengthVer = 5.0;
    m_presets[3].params.lengthHor = 5.0;
    m_presets[3].params.offsetVer = 3.0;
    m_presets[3].params.offsetHor = 3.0;
    m_presets[3].params.bleedVer = 3.0;
    m_presets[3].params.bleedHor = 3.0;
    m_presets[3].params.strokeWidth = 0.25;

    // Preset 5: Offset 4/3 White
    m_presets[4].id = 5;
    m_presets[4].name = "Offset 4/3 White";
    m_presets[4].params.lengthVer = 4.0;
    m_presets[4].params.lengthHor = 4.0;
    m_presets[4].params.offsetVer = 3.0;
    m_presets[4].params.offsetHor = 3.0;
    m_presets[4].params.bleedVer = 2.0;
    m_presets[4].params.bleedHor = 2.0;
    m_presets[4].params.whiteContour = true;
    m_presets[4].params.strokeWidth = 0.25;

    // Preset 6: Packaging
    m_presets[5].id = 6;
    m_presets[5].name = "Packaging";
    m_presets[5].params.lengthVer = 6.0;
    m_presets[5].params.lengthHor = 6.0;
    m_presets[5].params.offsetVer = 4.0;
    m_presets[5].params.offsetHor = 4.0;
    m_presets[5].params.bleedVer = 3.0;
    m_presets[5].params.bleedHor = 3.0;
    m_presets[5].params.strokeWidth = 0.3;

    // Preset 7: Fine 0.1pt
    m_presets[6].id = 7;
    m_presets[6].name = "Fine 0.1pt";
    m_presets[6].params.lengthVer = 4.0;
    m_presets[6].params.lengthHor = 4.0;
    m_presets[6].params.offsetVer = 2.0;
    m_presets[6].params.offsetHor = 2.0;
    m_presets[6].params.bleedVer = 0.0;
    m_presets[6].params.bleedHor = 0.0;
    m_presets[6].params.strokeWidth = 0.1;

    // Preset 8: Bleed 5mm
    m_presets[7].id = 8;
    m_presets[7].name = "Bleed 5mm";
    m_presets[7].params.lengthVer = 5.0;
    m_presets[7].params.lengthHor = 5.0;
    m_presets[7].params.offsetVer = 3.0;
    m_presets[7].params.offsetHor = 3.0;
    m_presets[7].params.bleedVer = 5.0;
    m_presets[7].params.bleedHor = 5.0;
    m_presets[7].params.strokeWidth = 0.25;
}

PresetItem ConfigManager::GetPreset(int index) const {
    if (index >= 0 && index < (int)m_presets.size()) {
        return m_presets[index];
    }
    return m_presets[0];
}

void ConfigManager::SetPreset(int index, const PresetItem& p) {
    if (index >= 0 && index < (int)m_presets.size()) {
        m_presets[index] = p;
        SavePresetsToJson();
    }
}

CropMarkParams ConfigManager::GetActiveParams() const {
    return GetPreset(m_activePresetIndex).params;
}

void ConfigManager::SetActiveParams(const CropMarkParams& params) {
    if (m_activePresetIndex >= 0 && m_activePresetIndex < (int)m_presets.size()) {
        m_presets[m_activePresetIndex].params = params;
        SavePresetsToJson();
    }
}

void ConfigManager::SetActivePresetIndex(int idx) {
    if (idx >= 0 && idx < 8) {
        m_activePresetIndex = idx;
        Save();
    }
}

void ConfigManager::LoadPresetsFromJson() {
    std::string jsonPath = GetPresetsJsonPath();
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        SavePresetsToJson(); // generate default file
        return;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Simple robust property extraction for the 8 presets
    for (int i = 0; i < 8; ++i) {
        std::string idTag = "\"id\": " + std::to_string(i + 1);
        size_t pos = content.find(idTag);
        if (pos == std::string::npos) idTag = "\"id\":" + std::to_string(i + 1), pos = content.find(idTag);
        if (pos == std::string::npos) continue;

        size_t blockEnd = content.find("}", pos);
        if (blockEnd == std::string::npos) blockEnd = content.length();
        std::string block = content.substr(pos, blockEnd - pos);

        auto getDouble = [&](const std::string& key, double defVal) -> double {
            size_t kp = block.find("\"" + key + "\"");
            if (kp == std::string::npos) return defVal;
            size_t colon = block.find(":", kp);
            if (colon == std::string::npos) return defVal;
            try { return std::stod(block.substr(colon + 1)); } catch (...) { return defVal; }
        };

        auto getInt = [&](const std::string& key, int defVal) -> int {
            size_t kp = block.find("\"" + key + "\"");
            if (kp == std::string::npos) return defVal;
            size_t colon = block.find(":", kp);
            if (colon == std::string::npos) return defVal;
            try { return std::stoi(block.substr(colon + 1)); } catch (...) { return defVal; }
        };

        auto getBool = [&](const std::string& key, bool defVal) -> bool {
            size_t kp = block.find("\"" + key + "\"");
            if (kp == std::string::npos) return defVal;
            size_t colon = block.find(":", kp);
            if (colon == std::string::npos) return defVal;
            std::string sub = block.substr(colon + 1, 10);
            if (sub.find("true") != std::string::npos) return true;
            if (sub.find("false") != std::string::npos) return false;
            return defVal;
        };

        auto getString = [&](const std::string& key, const std::string& defVal) -> std::string {
            size_t kp = block.find("\"" + key + "\"");
            if (kp == std::string::npos) return defVal;
            size_t q1 = block.find("\"", kp + key.length() + 2);
            if (q1 == std::string::npos) return defVal;
            size_t q2 = block.find("\"", q1 + 1);
            if (q2 == std::string::npos) return defVal;
            return block.substr(q1 + 1, q2 - q1 - 1);
        };

        m_presets[i].name = getString("name", m_presets[i].name);
        m_presets[i].params.lengthVer = getDouble("lengthVer", m_presets[i].params.lengthVer);
        m_presets[i].params.lengthHor = getDouble("lengthHor", m_presets[i].params.lengthHor);
        m_presets[i].params.offsetVer = getDouble("offsetVer", m_presets[i].params.offsetVer);
        m_presets[i].params.offsetHor = getDouble("offsetHor", m_presets[i].params.offsetHor);
        m_presets[i].params.bleedVer = getDouble("bleedVer", m_presets[i].params.bleedVer);
        m_presets[i].params.bleedHor = getDouble("bleedHor", m_presets[i].params.bleedHor);
        m_presets[i].params.strokeWidth = getDouble("strokeWidth", m_presets[i].params.strokeWidth);
        m_presets[i].params.units = getInt("units", m_presets[i].params.units);
        m_presets[i].params.strokeUnits = getInt("strokeUnits", m_presets[i].params.strokeUnits);
        m_presets[i].params.whiteContour = getBool("whiteContour", m_presets[i].params.whiteContour);
        m_presets[i].params.perimeterOnly = getBool("perimeterOnly", true);
        m_presets[i].params.boundsMode = getInt("boundsMode", m_presets[i].params.boundsMode);
        m_presets[i].params.groupMode = getInt("groupMode", m_presets[i].params.groupMode);
        m_presets[i].params.targetMode = getInt("targetMode", m_presets[i].params.targetMode);

        m_presets[i].params.dirs.topLeft = getBool("tl", true);
        m_presets[i].params.dirs.topCenter = getBool("tc", false);
        m_presets[i].params.dirs.topRight = getBool("tr", true);

        m_presets[i].params.dirs.bottomLeft = getBool("bl", true);
        m_presets[i].params.dirs.bottomCenter = getBool("bc", false);
        m_presets[i].params.dirs.bottomRight = getBool("br", true);

        m_presets[i].params.dirs.leftTop = getBool("lt", true);
        m_presets[i].params.dirs.leftCenter = getBool("lc", false);
        m_presets[i].params.dirs.leftBottom = getBool("lb", true);

        m_presets[i].params.dirs.rightTop = getBool("rt", true);
        m_presets[i].params.dirs.rightCenter = getBool("rc", false);
        m_presets[i].params.dirs.rightBottom = getBool("rb", true);
    }
}

void ConfigManager::SavePresetsToJson() {
    std::string jsonPath = GetPresetsJsonPath();
    std::ofstream file(jsonPath);
    if (!file.is_open()) return;

    file << "{\n  \"presets\": [\n";
    for (size_t i = 0; i < m_presets.size(); ++i) {
        const auto& p = m_presets[i];
        file << "    {\n";
        file << "      \"id\": " << p.id << ",\n";
        file << "      \"name\": \"" << p.name << "\",\n";
        file << "      \"lengthVer\": " << p.params.lengthVer << ",\n";
        file << "      \"lengthHor\": " << p.params.lengthHor << ",\n";
        file << "      \"offsetVer\": " << p.params.offsetVer << ",\n";
        file << "      \"offsetHor\": " << p.params.offsetHor << ",\n";
        file << "      \"bleedVer\": " << p.params.bleedVer << ",\n";
        file << "      \"bleedHor\": " << p.params.bleedHor << ",\n";
        file << "      \"strokeWidth\": " << p.params.strokeWidth << ",\n";
        file << "      \"units\": " << p.params.units << ",\n";
        file << "      \"strokeUnits\": " << p.params.strokeUnits << ",\n";
        file << "      \"whiteContour\": " << (p.params.whiteContour ? "true" : "false") << ",\n";
        file << "      \"perimeterOnly\": " << (p.params.perimeterOnly ? "true" : "false") << ",\n";
        file << "      \"boundsMode\": " << p.params.boundsMode << ",\n";
        file << "      \"groupMode\": " << p.params.groupMode << ",\n";
        file << "      \"targetMode\": " << p.params.targetMode << ",\n";
        file << "      \"tl\": " << (p.params.dirs.topLeft ? "true" : "false") << ", ";
        file << "\"tc\": " << (p.params.dirs.topCenter ? "true" : "false") << ", ";
        file << "\"tr\": " << (p.params.dirs.topRight ? "true" : "false") << ", ";
        file << "\"bl\": " << (p.params.dirs.bottomLeft ? "true" : "false") << ", ";
        file << "\"bc\": " << (p.params.dirs.bottomCenter ? "true" : "false") << ", ";
        file << "\"br\": " << (p.params.dirs.bottomRight ? "true" : "false") << ", ";
        file << "\"lt\": " << (p.params.dirs.leftTop ? "true" : "false") << ", ";
        file << "\"lc\": " << (p.params.dirs.leftCenter ? "true" : "false") << ", ";
        file << "\"lb\": " << (p.params.dirs.leftBottom ? "true" : "false") << ", ";
        file << "\"rt\": " << (p.params.dirs.rightTop ? "true" : "false") << ", ";
        file << "\"rc\": " << (p.params.dirs.rightCenter ? "true" : "false") << ", ";
        file << "\"rb\": " << (p.params.dirs.rightBottom ? "true" : "false") << "\n";
        file << "    }" << (i + 1 < m_presets.size() ? "," : "") << "\n";
    }
    file << "  ]\n}\n";
    file.close();
}

void ConfigManager::Load() {
    std::string ini = GetIniPath();

    m_openUiHotkey.vkCode = (UINT)GetPrivateProfileIntA("Hotkeys", "OpenUi_VK", VK_F4, ini.c_str());
    m_openUiHotkey.modifiers = (UINT)GetPrivateProfileIntA("Hotkeys", "OpenUi_Mod", 0, ini.c_str());

    m_quickApplyHotkey.vkCode = (UINT)GetPrivateProfileIntA("Hotkeys", "QuickApply_VK", VK_F4, ini.c_str());
    m_quickApplyHotkey.modifiers = (UINT)GetPrivateProfileIntA("Hotkeys", "QuickApply_Mod", MOD_SHIFT, ini.c_str());

    m_disabled = (GetPrivateProfileIntA("General", "Disabled", 0, ini.c_str()) != 0);
    m_activePresetIndex = GetPrivateProfileIntA("General", "ActivePresetIndex", 0, ini.c_str());
    if (m_activePresetIndex < 0 || m_activePresetIndex >= 8) m_activePresetIndex = 0;

    LoadPresetsFromJson();
}

void ConfigManager::Save() {
    std::string ini = GetIniPath();

    WritePrivateProfileStringA("Hotkeys", "OpenUi_VK", std::to_string(m_openUiHotkey.vkCode).c_str(), ini.c_str());
    WritePrivateProfileStringA("Hotkeys", "OpenUi_Mod", std::to_string(m_openUiHotkey.modifiers).c_str(), ini.c_str());

    WritePrivateProfileStringA("Hotkeys", "QuickApply_VK", std::to_string(m_quickApplyHotkey.vkCode).c_str(), ini.c_str());
    WritePrivateProfileStringA("Hotkeys", "QuickApply_Mod", std::to_string(m_quickApplyHotkey.modifiers).c_str(), ini.c_str());

    WritePrivateProfileStringA("General", "Disabled", m_disabled ? "1" : "0", ini.c_str());
    WritePrivateProfileStringA("General", "ActivePresetIndex", std::to_string(m_activePresetIndex).c_str(), ini.c_str());

    SavePresetsToJson();
}

std::string ConfigManager::GetHotkeyDisplayString(const HotkeyConfig& hk) {
    std::string s;
    if (hk.modifiers & MOD_CONTROL) s += "Ctrl + ";
    if (hk.modifiers & MOD_ALT)     s += "Alt + ";
    if (hk.modifiers & MOD_SHIFT)   s += "Shift + ";

    for (const auto& k : g_supportedKeys) {
        if (k.vk == hk.vkCode) {
            s += k.name;
            return s;
        }
    }
    s += "Key_" + std::to_string(hk.vkCode);
    return s;
}

bool ConfigManager::MatchHotkey(const HotkeyConfig& hk, UINT vkCode, UINT currentModifiers) {
    if (hk.vkCode == 0) return false;
    return (hk.vkCode == vkCode) && (hk.modifiers == currentModifiers);
}

// ----------------------------------------------------------------------------
// Win32 Settings Dialog
// ----------------------------------------------------------------------------
#define IDC_MOD_OPENUI      1001
#define IDC_KEY_OPENUI      1002
#define IDC_MOD_QUICK       1003
#define IDC_KEY_QUICK       1004
#define IDC_CHK_DISABLE     1005
#define IDC_BTN_OK          1006
#define IDC_BTN_CANCEL      1007
#define IDC_BTN_DEFAULTS    1008

struct DlgData {
    HotkeyConfig openUiHk;
    HotkeyConfig quickHk;
    bool disabled;
    bool saved;
};

static DlgData g_dlgData;
static bool g_dialogRunning = false;
static HFONT g_hDlgFont = NULL;

static void PopulateSettingsControls(HWND hDlg) {
    HWND hModOpen = GetDlgItem(hDlg, IDC_MOD_OPENUI);
    HWND hKeyOpen = GetDlgItem(hDlg, IDC_KEY_OPENUI);
    HWND hModQuick = GetDlgItem(hDlg, IDC_MOD_QUICK);
    HWND hKeyQuick = GetDlgItem(hDlg, IDC_KEY_QUICK);

    for (const auto& m : g_supportedMods) {
        SendMessageA(hModOpen, CB_ADDSTRING, 0, (LPARAM)m.name);
        SendMessageA(hModQuick, CB_ADDSTRING, 0, (LPARAM)m.name);
    }

    for (const auto& k : g_supportedKeys) {
        SendMessageA(hKeyOpen, CB_ADDSTRING, 0, (LPARAM)k.name);
        SendMessageA(hKeyQuick, CB_ADDSTRING, 0, (LPARAM)k.name);
    }

    auto selectMod = [](HWND h, UINT mod) {
        for (size_t i = 0; i < sizeof(g_supportedMods) / sizeof(g_supportedMods[0]); ++i) {
            if (g_supportedMods[i].mod == mod) {
                SendMessage(h, CB_SETCURSEL, (WPARAM)i, 0);
                return;
            }
        }
        SendMessage(h, CB_SETCURSEL, 0, 0);
    };

    auto selectKey = [](HWND h, UINT vk) {
        for (size_t i = 0; i < sizeof(g_supportedKeys) / sizeof(g_supportedKeys[0]); ++i) {
            if (g_supportedKeys[i].vk == vk) {
                SendMessage(h, CB_SETCURSEL, (WPARAM)i, 0);
                return;
            }
        }
        SendMessage(h, CB_SETCURSEL, 0, 0);
    };

    selectMod(hModOpen, g_dlgData.openUiHk.modifiers);
    selectKey(hKeyOpen, g_dlgData.openUiHk.vkCode);

    selectMod(hModQuick, g_dlgData.quickHk.modifiers);
    selectKey(hKeyQuick, g_dlgData.quickHk.vkCode);

    CheckDlgButton(hDlg, IDC_CHK_DISABLE, g_dlgData.disabled ? BST_CHECKED : BST_UNCHECKED);
}

static LRESULT CALLBACK SettingsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            if (!g_hDlgFont) {
                g_hDlgFont = CreateFontA(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            }
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDC_BTN_OK) {
                HWND hModOpen = GetDlgItem(hwnd, IDC_MOD_OPENUI);
                HWND hKeyOpen = GetDlgItem(hwnd, IDC_KEY_OPENUI);
                HWND hModQuick = GetDlgItem(hwnd, IDC_MOD_QUICK);
                HWND hKeyQuick = GetDlgItem(hwnd, IDC_KEY_QUICK);

                int idxModOpen = (int)SendMessage(hModOpen, CB_GETCURSEL, 0, 0);
                int idxKeyOpen = (int)SendMessage(hKeyOpen, CB_GETCURSEL, 0, 0);
                if (idxModOpen >= 0 && idxModOpen < (int)(sizeof(g_supportedMods) / sizeof(g_supportedMods[0]))) {
                    g_dlgData.openUiHk.modifiers = g_supportedMods[idxModOpen].mod;
                }
                if (idxKeyOpen >= 0 && idxKeyOpen < (int)(sizeof(g_supportedKeys) / sizeof(g_supportedKeys[0]))) {
                    g_dlgData.openUiHk.vkCode = g_supportedKeys[idxKeyOpen].vk;
                }

                int idxModQuick = (int)SendMessage(hModQuick, CB_GETCURSEL, 0, 0);
                int idxKeyQuick = (int)SendMessage(hKeyQuick, CB_GETCURSEL, 0, 0);
                if (idxModQuick >= 0 && idxModQuick < (int)(sizeof(g_supportedMods) / sizeof(g_supportedMods[0]))) {
                    g_dlgData.quickHk.modifiers = g_supportedMods[idxModQuick].mod;
                }
                if (idxKeyQuick >= 0 && idxKeyQuick < (int)(sizeof(g_supportedKeys) / sizeof(g_supportedKeys[0]))) {
                    g_dlgData.quickHk.vkCode = g_supportedKeys[idxKeyQuick].vk;
                }

                g_dlgData.disabled = (IsDlgButtonChecked(hwnd, IDC_CHK_DISABLE) == BST_CHECKED);
                g_dlgData.saved = true;
                g_dialogRunning = false;
                PostMessageA(hwnd, WM_NULL, 0, 0);
                return 0;
            } else if (id == IDC_BTN_CANCEL) {
                g_dlgData.saved = false;
                g_dialogRunning = false;
                PostMessageA(hwnd, WM_NULL, 0, 0);
                return 0;
            } else if (id == IDC_BTN_DEFAULTS) {
                g_dlgData.openUiHk = { VK_F4, 0 };
                g_dlgData.quickHk = { VK_F4, MOD_SHIFT };
                g_dlgData.disabled = false;

                HWND hModOpen = GetDlgItem(hwnd, IDC_MOD_OPENUI);
                HWND hKeyOpen = GetDlgItem(hwnd, IDC_KEY_OPENUI);
                HWND hModQuick = GetDlgItem(hwnd, IDC_MOD_QUICK);
                HWND hKeyQuick = GetDlgItem(hwnd, IDC_KEY_QUICK);

                SendMessage(hModOpen, CB_RESETCONTENT, 0, 0);
                SendMessage(hKeyOpen, CB_RESETCONTENT, 0, 0);
                SendMessage(hModQuick, CB_RESETCONTENT, 0, 0);
                SendMessage(hKeyQuick, CB_RESETCONTENT, 0, 0);

                PopulateSettingsControls(hwnd);
                return 0;
            }
            break;
        }

        case WM_CLOSE: {
            g_dlgData.saved = false;
            g_dialogRunning = false;
            PostMessageA(hwnd, WM_NULL, 0, 0);
            return 0;
        }
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static float GetDialogScale(HWND hwnd) {
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

bool ConfigManager::ShowSettingsDialog(HWND parentHwnd, float currentScale) {
    m_dialogActive = true;

    g_dlgData.openUiHk = m_openUiHotkey;
    g_dlgData.quickHk = m_quickApplyHotkey;
    g_dlgData.disabled = m_disabled;
    g_dlgData.saved = false;

    float scale = currentScale;
    if (scale < 0.75f || scale > 4.0f) {
        scale = GetDialogScale(parentHwnd);
    }
    auto S = [scale](int v) -> int { return (int)(v * scale + 0.5f); };

    HINSTANCE hInst = GetModuleHandle(NULL);
    const char* className = "MakeCropMarksAISettingsClass";

    WNDCLASSEXA wc = { sizeof(wc) };
    if (!GetClassInfoExA(hInst, className, &wc)) {
        wc.lpfnWndProc = SettingsDlgProc;
        wc.hInstance = hInst;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = className;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassExA(&wc);
    }

    int clientW = S(340);
    int clientH = S(226);
    RECT rc = { 0, 0, clientW, clientH };
    DWORD dwStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
    DWORD dwExStyle = WS_EX_DLGMODALFRAME | WS_EX_TOPMOST;
    AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);
    int dlgW = rc.right - rc.left;
    int dlgH = rc.bottom - rc.top;

    RECT pr = { 0, 0, 1024, 768 };
    if (parentHwnd && IsWindow(parentHwnd)) GetWindowRect(parentHwnd, &pr);
    int x = pr.left + ((pr.right - pr.left) - dlgW) / 2;
    int y = pr.top + ((pr.bottom - pr.top) - dlgH) / 2;
    if (x < 50) x = 50;
    if (y < 50) y = 50;

    HWND hDlg = CreateWindowExA(
        dwExStyle,
        className,
        "CropMarksAI - Settings & Hotkeys",
        dwStyle,
        x, y, dlgW, dlgH,
        parentHwnd, NULL, hInst, NULL
    );

    if (!hDlg) {
        m_dialogActive = false;
        return false;
    }

    if (g_hDlgFont) {
        DeleteObject(g_hDlgFont);
        g_hDlgFont = NULL;
    }
    int fontH = -(int)(11.0f * scale + 0.5f);
    g_hDlgFont = CreateFontA(fontH, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    auto makeStatic = [&](const char* txt, int sx, int sy, int sw, int sh) {
        HWND h = CreateWindowExA(0, "STATIC", txt, WS_CHILD | WS_VISIBLE, sx, sy, sw, sh, hDlg, NULL, hInst, NULL);
        SendMessage(h, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);
        return h;
    };

    auto makeCombo = [&](int cid, int cx, int cy, int cw) {
        HWND h = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                                 cx, cy, cw, S(160), hDlg, (HMENU)(INT_PTR)cid, hInst, NULL);
        SendMessage(h, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);
        SendMessageA(h, CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)S(20));
        SendMessageA(h, CB_SETITEMHEIGHT, (WPARAM)0,  (LPARAM)S(18));
        return h;
    };

    // UI Group: Hotkeys
    HWND hGrp1 = CreateWindowExA(0, "BUTTON", " Keyboard Shortcuts ", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                 S(10), S(8), clientW - S(20), S(110), hDlg, NULL, hInst, NULL);
    SendMessage(hGrp1, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);

    makeStatic("1. Open / Toggle Panel:", S(20), S(26), S(140), S(18));
    makeCombo(IDC_MOD_OPENUI, S(164), S(24), S(84));
    makeCombo(IDC_KEY_OPENUI, S(252), S(24), S(68));

    makeStatic("2. Quick Apply Preset:", S(20), S(50), S(140), S(18));
    makeCombo(IDC_MOD_QUICK, S(164), S(48), S(84));
    makeCombo(IDC_KEY_QUICK, S(252), S(48), S(68));

    makeStatic("Hint: Quick Apply generates marks instantly without opening dialog.", S(20), S(74), S(300), S(36));

    // UI Group: General / Status
    HWND hGrp2 = CreateWindowExA(0, "BUTTON", " Status ", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                 S(10), S(124), clientW - S(20), S(52), hDlg, NULL, hInst, NULL);
    SendMessage(hGrp2, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);

    HWND hChkDis = CreateWindowExA(0, "BUTTON", "Disable plugin (disable all hotkeys)",
                                   WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                                   S(20), S(144), clientW - S(40), S(20), hDlg, (HMENU)(INT_PTR)IDC_CHK_DISABLE, hInst, NULL);
    SendMessage(hChkDis, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);

    // Bottom Action Buttons
    int btnH = S(24);
    int btnY = S(188);
    HWND hBtnDef = CreateWindowExA(0, "BUTTON", "Defaults", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                   S(10), btnY, S(70), btnH, hDlg, (HMENU)(INT_PTR)IDC_BTN_DEFAULTS, hInst, NULL);
    HWND hBtnOk = CreateWindowExA(0, "BUTTON", "Save", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                                  S(188), btnY, S(66), btnH, hDlg, (HMENU)(INT_PTR)IDC_BTN_OK, hInst, NULL);
    HWND hBtnCan = CreateWindowExA(0, "BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                  S(260), btnY, S(66), btnH, hDlg, (HMENU)(INT_PTR)IDC_BTN_CANCEL, hInst, NULL);

    SendMessage(hBtnDef, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);
    SendMessage(hBtnOk, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);
    SendMessage(hBtnCan, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);

    PopulateSettingsControls(hDlg);
    UpdateWindow(hDlg);

    if (parentHwnd) EnableWindow(parentHwnd, FALSE);

    g_dialogRunning = true;
    MSG msg;
    while (g_dialogRunning && GetMessageA(&msg, NULL, 0, 0)) {
        if (!IsDialogMessageA(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    if (parentHwnd) EnableWindow(parentHwnd, TRUE);
    DestroyWindow(hDlg);

    if (g_hDlgFont) {
        DeleteObject(g_hDlgFont);
        g_hDlgFont = NULL;
    }

    if (g_dlgData.saved) {
        m_openUiHotkey = g_dlgData.openUiHk;
        m_quickApplyHotkey = g_dlgData.quickHk;
        m_disabled = g_dlgData.disabled;
        Save();
    }

    m_dialogActive = false;
    return g_dlgData.saved;
}
