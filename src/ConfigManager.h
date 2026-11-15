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

    // Prepress general settings
    int GetCropMarksLayerOrder() const { return m_cropMarksLayerOrder; }
    int GetCrossMarksLayerOrder() const { return m_crossMarksLayerOrder; }
    int GetBarLength() const { return m_barLength; }
    int GetBarStandard() const { return m_barStandard; }
    double GetBarsOffset() const { return m_barsOffset; }
    double GetCrossOffset() const { return m_crossOffset; }
    bool GetDocInfo() const { return m_docInfo; }
    bool GetDocInfoTranslit() const { return m_docInfoTranslit; }

    void SetCropMarksLayerOrder(int order) { m_cropMarksLayerOrder = order; }
    void SetCrossMarksLayerOrder(int order) { m_crossMarksLayerOrder = order; }
    void SetBarLength(int length) { m_barLength = length; }
    void SetBarStandard(int std) { m_barStandard = std; }
    void SetBarsOffset(double off) { m_barsOffset = off; }
    void SetCrossOffset(double off) { m_crossOffset = off; }
    void SetDocInfo(bool enabled) { m_docInfo = enabled; }
    void SetDocInfoTranslit(bool enabled) { m_docInfoTranslit = enabled; }

    // Color Bars Advanced Settings
    int GetBarMode() const { return m_barMode; }
    void SetBarMode(int mode) { m_barMode = mode; }

    double GetPatchWidth() const { return m_patchWidth; }
    void SetPatchWidth(double w) { m_patchWidth = w; }

    double GetPatchHeight() const { return m_patchHeight; }
    void SetPatchHeight(double h) { m_patchHeight = h; }

    bool GetBarCyan() const { return m_barCyan; }
    void SetBarCyan(bool val) { m_barCyan = val; }

    bool GetBarMagenta() const { return m_barMagenta; }
    void SetBarMagenta(bool val) { m_barMagenta = val; }

    bool GetBarYellow() const { return m_barYellow; }
    void SetBarYellow(bool val) { m_barYellow = val; }

    bool GetBarBlack() const { return m_barBlack; }
    void SetBarBlack(bool val) { m_barBlack = val; }

    // Registration Marks Advanced Settings (Anosov model)
    double GetRegCircleDiam() const { return m_regCircleDiam; }
    void SetRegCircleDiam(double val) { m_regCircleDiam = val; }

    double GetRegCrossLength() const { return m_regCrossLength; }
    void SetRegCrossLength(double val) { m_regCrossLength = val; }

    double GetRegStrokeWidth() const { return m_regStrokeWidth; }
    void SetRegStrokeWidth(double val) { m_regStrokeWidth = val; }

    int GetRegStrokeUnits() const { return m_regStrokeUnits; }
    void SetRegStrokeUnits(int val) { m_regStrokeUnits = val; }

    double GetRegOffsetHor() const { return m_regOffsetHor; }
    void SetRegOffsetHor(double val) { m_regOffsetHor = val; }

    double GetRegOffsetVer() const { return m_regOffsetVer; }
    void SetRegOffsetVer(double val) { m_regOffsetVer = val; }

    int GetRegPositions() const { return m_regPositions; }
    void SetRegPositions(int val) { m_regPositions = val; }

    bool GetRegWhiteUnderlay() const { return m_regWhiteUnderlay; }
    void SetRegWhiteUnderlay(bool val) { m_regWhiteUnderlay = val; }

    // Document Spots
    const std::vector<DocumentSpotColor>& GetDocumentSpots() const { return m_documentSpots; }
    std::vector<DocumentSpotColor>& GetDocumentSpots() { return m_documentSpots; }
    void SetDocumentSpots(const std::vector<DocumentSpotColor>& spots) { m_documentSpots = spots; }

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

    // Prepress settings
    int m_cropMarksLayerOrder = kCropMarksAbovePatches;
    int m_crossMarksLayerOrder = kCrossMarksAbovePatches;
    int m_barLength = kColorBarShort;
    int m_barStandard = kStandardISO12647_2;
    double m_barsOffset = 0.0;
    double m_crossOffset = 0.0;
    bool m_docInfo = false;
    bool m_docInfoTranslit = false;

    // Color Bar Settings
    int m_barMode = kColorBar100Solids;
    double m_patchWidth = 5.0;  // mm
    double m_patchHeight = 5.0; // mm
    bool m_barCyan = true;
    bool m_barMagenta = true;
    bool m_barYellow = true;
    bool m_barBlack = true;

    // Registration Marks Settings
    double m_regCircleDiam = 4.0;   // mm
    double m_regCrossLength = 6.0;  // mm
    double m_regStrokeWidth = 0.10; // mm
    int m_regStrokeUnits = kUnitsMM;
    double m_regOffsetHor = 2.0;    // mm
    double m_regOffsetVer = 2.0;    // mm
    int m_regPositions = kRegPositionsCenters;
    bool m_regWhiteUnderlay = true;

    // Document Spots
    std::vector<DocumentSpotColor> m_documentSpots;

    std::vector<PresetItem> m_presets;
};

#endif // CONFIG_MANAGER_H
