#pragma once

#ifndef MAKE_CROP_MARKS_AI_PLUGIN_H
#define MAKE_CROP_MARKS_AI_PLUGIN_H

#include "Suites.h"
#include "CropMarkEngine.h"
#include "ConfigManager.h"

enum PanelTab {
    kTabMarks = 0,
    kTabPantones = 1,
    kTabSettings = 2
};

class MakeCropMarksAIPlugin {
public:
    static MakeCropMarksAIPlugin& GetInstance();

    SPErr StartupPlugin(void* message);
    SPErr ShutdownPlugin(void* message);
    SPErr GoMenuItem(void* message);
    SPErr UpdateMenuItem(void* message);
    SPErr Notify(void* message);

    void SetPluginRef(SPPluginRef pluginRef) {
        m_pluginRef = pluginRef;
        gPluginRef = pluginRef;
    }
    SPPluginRef GetPluginRef() const { return m_pluginRef; }
    HWND GetPanelHWND() const { return m_hPanelDlg; }
    void OnVisibilityChanged(bool isVisible);

    void UpdateUI();
    void ReadParamsFromUI(CropMarkParams& params);
    void PopulateUIFromParams(const CropMarkParams& params);
    void ExecuteCreateMarks();
    void ExecuteQuickApply();
    void TogglePanel();

    // Tab Management
    void SwitchTab(int tabIndex);
    void UpdateTabVisibility();
    void PopulateLayersCombo();
    void PopulatePantonesList();
    void EnsureSpotsScannedForCurrentDoc(bool forceRescan = false);
    void UpdateSpotsFromList();
    static bool IsPopulatingSpots() { return s_isPopulatingSpots; }
    void ReadSettingsFromUI();
    void PopulateSettingsToUI();
    int GetCurrentTab() const { return m_currentTab; }

private:
    MakeCropMarksAIPlugin();
    ~MakeCropMarksAIPlugin();
    MakeCropMarksAIPlugin(const MakeCropMarksAIPlugin&) = delete;
    MakeCropMarksAIPlugin& operator=(const MakeCropMarksAIPlugin&) = delete;

    SPErr AddMenuItems();
    SPErr CreateDockablePanel();

    SPPluginRef       m_pluginRef;
    AIPanelRef        m_panelRef;
    HWND              m_hPanelDlg;
    AINotifierHandle  m_hBrightnessNotifier;
    AINotifierHandle  m_hDocNewNotifier = nullptr;
    AINotifierHandle  m_hDocOpenedNotifier = nullptr;
    AIDocumentHandle  m_lastScannedDoc = nullptr;

    AIMenuItemHandle  m_hMenuWindow;
    AIMenuItemHandle  m_hMenuCreate;
    AIMenuItemHandle  m_hMenuQuickApply;
    AIMenuItemHandle  m_hMenuSettings;
    AIMenuItemHandle  m_hMenuAbout;

    int               m_currentTab = kTabMarks;
    static bool       s_isPopulatingSpots;
};

#endif // MAKE_CROP_MARKS_AI_PLUGIN_H
