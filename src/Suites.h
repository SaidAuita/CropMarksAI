#pragma once

#ifndef SUITES_H
#define SUITES_H

#include <windows.h>
#include <cstdint>
#include <cstdio>

// Adobe Illustrator 2026 SDK Headers
#include "IllustratorSDK.h"
#include "AIPanel.h"
#include "AIMenu.h"
#include "AIMenuGroups.h"
#include "AIArt.h"
#include "AIPath.h"
#include "AIPathStyle.h"
#include "AICustomColor.h"
#include "AISwatchList.h"
#include "AIMatchingArt.h"
#include "AIMdMemory.h"
#include "AIDocument.h"
#include "AIArtboard.h"
#include "AITransformArt.h"
#include "AIUndo.h"
#include "AIUnicodeString.h"
#include "AIContext.h"
#include "AIDocumentView.h"
#include "AIDocumentList.h"
#include "AIUITheme.h"
#include "AINotifier.h"
#include "AILayer.h"
#include "AIUser.h"
#include "SPInterf.h"
#include "SPAccess.h"

// Suite function pointer declarations
extern "C" {
    extern SPBasicSuite*         sSPBasic;
    extern SPBlocksSuite*        sSPBlocks;
    extern void*                 sAIAssertion;
    extern AIPanelSuite*         sAIPanel;
    extern AIMenuSuite*          sAIMenu;
    extern AIUnicodeStringSuite* sAIUnicodeString;
    extern AIArtSuite*           sAIArt;
    extern AIPathSuite*          sAIPath;
    extern AIPathStyleSuite*     sAIPathStyle;
    extern AICustomColorSuite*   sAICustomColor;
    extern AISwatchListSuite*    sAISwatchList;
    extern AIMatchingArtSuite*   sAIMatchingArt;
    extern AIMdMemorySuite*      sAIMdMemory;
    extern AIDocumentSuite*      sAIDocument;
    extern AIArtboardSuite*      sAIArtboard;
    extern AITransformArtSuite*  sAITransformArt;
    extern AIUndoSuite*          sAIUndo;
    extern AIAppContextSuite*    sAIAppContext;
    extern AIDocumentViewSuite*  sAIDocumentView;
    extern AIDocumentListSuite*  sAIDocumentList;
    extern AIUIThemeSuite*       sAIUITheme;
    extern AINotifierSuite*      sAINotifier;
    extern AILayerSuite*         sAILayer;
    extern AIUserSuite*          sAIUser;
}

class AppContext {
private:
    AIAppContextHandle m_context;
public:
    explicit AppContext(SPPluginRef pluginRef) : m_context(nullptr) {
        if (sAIAppContext && pluginRef) {
            sAIAppContext->PushAppContext(pluginRef, &m_context);
        }
    }
    ~AppContext() {
        if (sAIAppContext && m_context) {
            sAIAppContext->PopAppContext(m_context);
            m_context = nullptr;
        }
    }
    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;
};

SPErr AcquireSuites(SPBasicSuite* basic);
void ReleaseSuites(SPBasicSuite* basic);
void PluginLog(const char* fmt, ...);

#endif // SUITES_H
