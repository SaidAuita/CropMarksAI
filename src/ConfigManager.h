#pragma once

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <windows.h>
#include <string>
#include <vector>
#include "CropMarkEngine.h"

struct HotkeyConfig {
    UINT vkCode;     // Virtual Key Code (e.g. VK_F4, 'M', VK_INSERT)
    UINT modifiers;  // Bitmask: 0 = None, 1 = Ctrl, 2 = Alt, 4 = Shift
};

class ConfigManager {
public:
    static ConfigManager& GetInstance();

    void Load();
    void Save();

    HotkeyConfig GetOpenUiHotkey() const { return m_openUiHotkey; }
    HotkeyConfig GetQuickApplyHotkey() const { return m_quickApplyHotkey; }
    bool IsPluginDisabled() const { return m_disabled; }
    int GetActivePresetIndex() const { return m_activePresetIndex; }

    void SetOpenUiHotkey(const HotkeyConfig& hk) { m_openUiHotkey = hk; }
    void SetQuickApplyHotkey(const HotkeyConfig& hk) { m_quickApplyHotkey = hk; }
    void SetPluginDisabled(bool disabled) { m_disabled = disabled; }
    void SetActivePresetIndex(int idx);

    std::string GetHotkeyDisplayString(const HotkeyConfig& hk);
    bool MatchHotkey(const HotkeyConfig& hk, UINT vkCode, UINT currentModifiers);

    // Presets (8 slots)
    const std::vector<PresetItem>& GetPresets() const { return m_presets; }
    PresetItem GetPreset(int index) const;
    void SetPreset(int index, const PresetItem& p);
    CropMarkParams GetActiveParams() const;
    void SetActiveParams(const CropMarkParams& params);

    // Displays the modal Preferences / Hotkeys dialog
    bool ShowSettingsDialog(HWND parentHwnd, float currentScale = 0.0f);
    bool IsDialogActive() const { return m_dialogActive; }

    std::string GetAppDataFolder() const;
    std::string GetIniPath() const;
    std::string GetPresetsJsonPath() const;

private:
    ConfigManager();
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    void InitDefaultPresets();
    void LoadPresetsFromJson();
    void SavePresetsToJson();

    HotkeyConfig m_openUiHotkey;
    HotkeyConfig m_quickApplyHotkey;
    bool m_disabled = false;
    int m_activePresetIndex = 0;
    bool m_dialogActive = false;

    std::vector<PresetItem> m_presets;
};

#endif // CONFIG_MANAGER_H
