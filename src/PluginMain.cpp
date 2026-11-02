#include "Suites.h"
#include "MakeCropMarksAIPlugin.h"
#include <cstring>

static SPErr SafeStartup(void* message) {
    SPInterfaceMessage* ifaceMsg = static_cast<SPInterfaceMessage*>(message);
    PluginLog("[PluginMain] Handling 'Startup' notification (msg=%p)...", message);
    PluginLog("  [SPMessageData] self=%p, globals=%p, basic=%p",
              ifaceMsg->d.self, ifaceMsg->d.globals, ifaceMsg->d.basic);

    MakeCropMarksAIPlugin::GetInstance().SetPluginRef(ifaceMsg->d.self);
    SPErr error = AcquireSuites(ifaceMsg->d.basic);
    PluginLog("  [AcquireSuites result: %d]", error);
    if (error == kSPNoError) {
        error = MakeCropMarksAIPlugin::GetInstance().StartupPlugin(message);
        PluginLog("  [StartupPlugin result: %d]", error);
    }
    return error;
}

extern "C" ASAPI ASErr PluginMain(char* caller, char* selector, void* message) {
    SPErr error = kSPNoError;

    if (!caller || !selector || !message) {
        return 1;
    }

    PluginLog("[PluginMain] >>> caller='%s', selector='%s'", caller, selector);

    // 1. SweetPea Interface Caller ("SP Interface")
    if (std::strcmp(caller, kSPInterfaceCaller) == 0) {
        SPInterfaceMessage* ifaceMsg = static_cast<SPInterfaceMessage*>(message);

        if (std::strcmp(selector, kSPInterfaceStartupSelector) == 0) {
            __try {
                error = SafeStartup(message);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                PluginLog("[FATAL] SEH Exception 0x%08X in Startup!", GetExceptionCode());
                error = 1;
            }
        } else if (std::strcmp(selector, kSPInterfaceShutdownSelector) == 0) {
            PluginLog("[PluginMain] Handling 'Shutdown' notification...");
            MakeCropMarksAIPlugin::GetInstance().ShutdownPlugin(message);
            ReleaseSuites(ifaceMsg->d.basic);
        } else if (std::strcmp(selector, kSPInterfaceAboutSelector) == 0) {
            PluginLog("[PluginMain] Handling 'About' notification...");
            MessageBoxA(NULL,
                "Make Crop Marks AI Native Plugin v1.00 (x64)\n\n"
                "High-performance native Prepress Crop Marks Generator for Adobe Illustrator.\n"
                "Author: SaidAuita (ph-cu-s.com/tools)",
                "About Make Crop Marks AI", MB_OK | MB_ICONINFORMATION);
        }
    }
    // 2. SweetPea Access Caller ("SP Access")
    else if (std::strcmp(caller, kSPAccessCaller) == 0) {
        if (std::strcmp(selector, kSPAccessUnloadSelector) == 0) {
            PluginLog("[PluginMain] Handling 'Unload' notification...");
        } else if (std::strcmp(selector, kSPAccessReloadSelector) == 0) {
            PluginLog("[PluginMain] Handling 'Reload' notification...");
        }
    }
    // 3. Illustrator Menu Caller ("AI Menu")
    else if (std::strcmp(caller, kCallerAIMenu) == 0) {
        if (std::strcmp(selector, kSelectorAIGoMenuItem) == 0) {
            PluginLog("[PluginMain] Handling 'AI Go' menu action...");
            error = MakeCropMarksAIPlugin::GetInstance().GoMenuItem(message);
        } else if (std::strcmp(selector, kSelectorAIUpdateMenuItem) == 0) {
            error = MakeCropMarksAIPlugin::GetInstance().UpdateMenuItem(message);
        }
    }
    // 4. Illustrator Notifier Caller ("AI Notifier")
    else if (std::strcmp(caller, kCallerAINotify) == 0) {
        if (std::strcmp(selector, kSelectorAINotify) == 0) {
            error = MakeCropMarksAIPlugin::GetInstance().Notify(message);
        }
    }

    return error;
}

#if defined(_WIN32)
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            PluginLog("[DllMain] DLL_PROCESS_ATTACH (hinstDLL=%p)", hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            PluginLog("[DllMain] DLL_PROCESS_DETACH");
            break;
    }
    return TRUE;
}
#endif
