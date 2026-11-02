#pragma once

#ifndef MAKE_CROP_MARKS_AI_PLUGIN_H
#define MAKE_CROP_MARKS_AI_PLUGIN_H

#include "Suites.h"
#include "CropMarkEngine.h"
#include "ConfigManager.h"

class MakeCropMarksAIPlugin {
public:
    static MakeCropMarksAIPlugin& GetInstance();

    SPErr StartupPlugin(void* message);
    SPErr ShutdownPlugin(void* message);
    SPErr GoMenuItem(void* message);
    SPErr UpdateMenuItem(void* message);
    SPErr Notify(void* message);

    void SetPluginRef(SPPluginRef pluginRef) { m_pluginRef = pluginRef; }
    SPPluginRef GetPluginRef() const { return m_pluginRef; }
    HWND GetPanelHWND() const { return m_hPanelDlg; }
    void OnVisibilityChanged(bool isVisible);

    void UpdateUI();
    void ReadParamsFromUI(CropMarkParams& params);
    void PopulateUIFromParams(const CropMarkParams& params);
    void ExecuteCreateMarks();
    void ExecuteQuickApply();
    void TogglePanel();

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

    AIMenuItemHandle  m_hMenuWindow;
    AIMenuItemHandle  m_hMenuCreate;
    AIMenuItemHandle  m_hMenuQuickApply;
    AIMenuItemHandle  m_hMenuSettings;
    AIMenuItemHandle  m_hMenuAbout;
};

#endif // MAKE_CROP_MARKS_AI_PLUGIN_H
