#include "Suites.h"
#include <cstdio>
#include <cstdarg>

extern "C" {
    SPBasicSuite*         sSPBasic = nullptr;
    SPBlocksSuite*        sSPBlocks = nullptr;
    void*                 sAIAssertion = nullptr;
    AIPanelSuite*         sAIPanel = nullptr;
    AIMenuSuite*          sAIMenu = nullptr;
    AIUnicodeStringSuite* sAIUnicodeString = nullptr;
    AIArtSuite*           sAIArt = nullptr;
    AIPathSuite*          sAIPath = nullptr;
    AIPathStyleSuite*     sAIPathStyle = nullptr;
    AICustomColorSuite*   sAICustomColor = nullptr;
    AISwatchListSuite*    sAISwatchList = nullptr;
    AIMatchingArtSuite*   sAIMatchingArt = nullptr;
    AIMdMemorySuite*      sAIMdMemory = nullptr;
    AIDocumentSuite*      sAIDocument = nullptr;
    AIArtboardSuite*      sAIArtboard = nullptr;
    AITransformArtSuite*  sAITransformArt = nullptr;
    AIUndoSuite*          sAIUndo = nullptr;
    AIAppContextSuite*    sAIAppContext = nullptr;
    AIDocumentViewSuite*  sAIDocumentView = nullptr;
    AIDocumentListSuite*  sAIDocumentList = nullptr;
    AIUIThemeSuite*       sAIUITheme = nullptr;
    AINotifierSuite*      sAINotifier = nullptr;
    AILayerSuite*         sAILayer = nullptr;
    AIUserSuite*          sAIUser = nullptr;
    AITextFrameSuite*     sAITextFrame = nullptr;
    AIATEPaintSuite*      sAIATEPaint = nullptr;
    ATE::TextRangeSuite*  sTextRange = nullptr;
    ATE::CharFeaturesSuite* sCharFeatures = nullptr;
    ATE::ApplicationPaintSuite* sApplicationPaint = nullptr;
}

extern "C" void ShowPlatformAssert(const bool success, const char* message) {
    (void)success;
    (void)message;
}

namespace ai {
    void ShowPlatformAssert(const bool success, const char* message) {
        (void)success;
        (void)message;
    }
}

static int32_t sSPBlocksVer = 0;
static int32_t sAIPanelVer = 0;
static int32_t sAIMenuVer = 0;
static int32_t sAIUnicodeStringVer = 0;
static int32_t sAIArtVer = 0;
static int32_t sAIPathVer = 0;
static int32_t sAIPathStyleVer = 0;
static int32_t sAICustomColorVer = 0;
static int32_t sAISwatchListVer = 0;
static int32_t sAIMatchingArtVer = 0;
static int32_t sAIMdMemoryVer = 0;
static int32_t sAIDocVer = 0;
static int32_t sAIArtboardVer = 0;
static int32_t sAITransformVer = 0;
static int32_t sAIUndoVer = 0;
static int32_t sAIAppContextVer = 0;
static int32_t sAIDocumentViewVer = 0;
static int32_t sAIDocumentListVer = 0;
static int32_t sAIUIThemeVer = 0;
static int32_t sAINotifierVer = 0;
static int32_t sAILayerVer = 0;
static int32_t sAIUserVer = 0;
static int32_t sAITextFrameVer = 0;
static int32_t sAIATEPaintVer = 0;
static int32_t sTextRangeVer = 0;
static int32_t sCharFeaturesVer = 0;
static int32_t sApplicationPaintVer = 0;

void PluginLog(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    OutputDebugStringA(buf);

    char tempPath[MAX_PATH];
    if (GetTempPathA(sizeof(tempPath), tempPath)) {
        char logFile[MAX_PATH];
        snprintf(logFile, sizeof(logFile), "%sMakeCropMarksAI_Native.log", tempPath);
        FILE* fp = nullptr;
        if (fopen_s(&fp, logFile, "a") == 0 && fp) {
            fputs(buf, fp);
            fputc('\n', fp);
            fclose(fp);
        }
    }
}

static SPErr AcquireSuiteAuto(SPBasicSuite* basic, const char* name, int32_t preferredVer, const void** suitePtr, int32_t* outVer) {
    if (!basic) return 1;

    SPErr err = basic->AcquireSuite(name, preferredVer, suitePtr);
    if (err == kSPNoError && *suitePtr) {
        *outVer = preferredVer;
        PluginLog("  [ACQUIRE OK] %s (preferred version %d)", name, preferredVer);
        return kSPNoError;
    }

    // Dynamic fallback version scan across supported Illustrator versions
    for (int32_t v = 30; v >= 1; --v) {
        if (v == preferredVer) continue;
        err = basic->AcquireSuite(name, v, suitePtr);
        if (err == kSPNoError && *suitePtr) {
            *outVer = v;
            PluginLog("  [ACQUIRE OK] %s (discovered version %d)", name, v);
            return kSPNoError;
        }
    }

    PluginLog("  [ACQUIRE FAIL] Could not acquire %s (tested versions 1..30)", name);
    *suitePtr = nullptr;
    *outVer = 0;
    return 1;
}

SPErr AcquireSuites(SPBasicSuite* basic) {
    if (!basic) {
        PluginLog("[AcquireSuites FAIL] basic pointer is NULL!");
        return 1;
    }
    sSPBasic = basic;

    PluginLog("[AcquireSuites] Starting automated suite acquisition (basic=%p)...", basic);

    AcquireSuiteAuto(basic, kSPBlocksSuite, kSPBlocksSuiteVersion, (const void**)&sSPBlocks, &sSPBlocksVer);
    AcquireSuiteAuto(basic, kAIPanelSuite, kAIPanelSuiteVersion, (const void**)&sAIPanel, &sAIPanelVer);
    AcquireSuiteAuto(basic, kAIMenuSuite, kAIMenuSuiteVersion, (const void**)&sAIMenu, &sAIMenuVer);
    AcquireSuiteAuto(basic, kAIUnicodeStringSuite, kAIUnicodeStringSuiteVersion, (const void**)&sAIUnicodeString, &sAIUnicodeStringVer);
    AcquireSuiteAuto(basic, kAIArtSuite, kAIArtSuiteVersion, (const void**)&sAIArt, &sAIArtVer);
    AcquireSuiteAuto(basic, kAIPathSuite, kAIPathSuiteVersion, (const void**)&sAIPath, &sAIPathVer);
    AcquireSuiteAuto(basic, kAIPathStyleSuite, kAIPathStyleSuiteVersion, (const void**)&sAIPathStyle, &sAIPathStyleVer);
    AcquireSuiteAuto(basic, kAICustomColorSuite, kAICustomColorSuiteVersion, (const void**)&sAICustomColor, &sAICustomColorVer);
    AcquireSuiteAuto(basic, kAISwatchListSuite, kAISwatchListSuiteVersion, (const void**)&sAISwatchList, &sAISwatchListVer);
    AcquireSuiteAuto(basic, kAIMatchingArtSuite, kAIMatchingArtSuiteVersion, (const void**)&sAIMatchingArt, &sAIMatchingArtVer);
    AcquireSuiteAuto(basic, kAIMdMemorySuite, kAIMdMemorySuiteVersion, (const void**)&sAIMdMemory, &sAIMdMemoryVer);
    AcquireSuiteAuto(basic, kAIDocumentSuite, kAIDocumentSuiteVersion, (const void**)&sAIDocument, &sAIDocVer);
    AcquireSuiteAuto(basic, kAIArtboardSuite, kAIArtboardSuiteVersion, (const void**)&sAIArtboard, &sAIArtboardVer);
    AcquireSuiteAuto(basic, kAITransformArtSuite, kAITransformArtSuiteVersion, (const void**)&sAITransformArt, &sAITransformVer);
    AcquireSuiteAuto(basic, kAIUndoSuite, kAIUndoSuiteVersion, (const void**)&sAIUndo, &sAIUndoVer);
    AcquireSuiteAuto(basic, kAIAppContextSuite, kAIAppContextSuiteVersion, (const void**)&sAIAppContext, &sAIAppContextVer);
    AcquireSuiteAuto(basic, kAIDocumentViewSuite, kAIDocumentViewSuiteVersion, (const void**)&sAIDocumentView, &sAIDocumentViewVer);
    AcquireSuiteAuto(basic, kAIDocumentListSuite, kAIDocumentListSuiteVersion, (const void**)&sAIDocumentList, &sAIDocumentListVer);
    AcquireSuiteAuto(basic, kAIUIThemeSuite, kAIUIThemeSuiteVersion, (const void**)&sAIUITheme, &sAIUIThemeVer);
    AcquireSuiteAuto(basic, kAINotifierSuite, kAINotifierSuiteVersion, (const void**)&sAINotifier, &sAINotifierVer);
    AcquireSuiteAuto(basic, kAILayerSuite, kAILayerSuiteVersion, (const void**)&sAILayer, &sAILayerVer);
    AcquireSuiteAuto(basic, kAIUserSuite, kAIUserSuiteVersion, (const void**)&sAIUser, &sAIUserVer);
    AcquireSuiteAuto(basic, kAITextFrameSuite, kAITextFrameVersion, (const void**)&sAITextFrame, &sAITextFrameVer);
    AcquireSuiteAuto(basic, kAIATEPaintSuite, kAIATEPaintVersion, (const void**)&sAIATEPaint, &sAIATEPaintVer);
    AcquireSuiteAuto(basic, kTextRangeSuite, kTextRangeSuiteVersion, (const void**)&sTextRange, &sTextRangeVer);
    AcquireSuiteAuto(basic, kCharFeaturesSuite, kCharFeaturesSuiteVersion, (const void**)&sCharFeatures, &sCharFeaturesVer);
    AcquireSuiteAuto(basic, kApplicationPaintSuite, kApplicationPaintSuiteVersion, (const void**)&sApplicationPaint, &sApplicationPaintVer);

    PluginLog("[AcquireSuites] Completed. sAIPanel=%p, sAIMenu=%p, sAIArt=%p, sAIPath=%p, sAIPathStyle=%p, sAICustomColor=%p, sAIUser=%p, sAITextFrame=%p",
              sAIPanel, sAIMenu, sAIArt, sAIPath, sAIPathStyle, sAICustomColor, sAIUser, sAITextFrame);
    return kSPNoError;
}

#define RELEASE_AUTO(suiteName, ver, ptr) \
    if (ptr && ver > 0) { \
        basic->ReleaseSuite(suiteName, ver); \
        ptr = nullptr; \
        ver = 0; \
    }

void ReleaseSuites(SPBasicSuite* basic) {
    if (!basic) return;

    PluginLog("[ReleaseSuites] Releasing all acquired suites...");

    RELEASE_AUTO(kSPBlocksSuite, sSPBlocksVer, sSPBlocks);
    RELEASE_AUTO(kAIPanelSuite, sAIPanelVer, sAIPanel);
    RELEASE_AUTO(kAIMenuSuite, sAIMenuVer, sAIMenu);
    RELEASE_AUTO(kAIUnicodeStringSuite, sAIUnicodeStringVer, sAIUnicodeString);
    RELEASE_AUTO(kAIArtSuite, sAIArtVer, sAIArt);
    RELEASE_AUTO(kAIPathSuite, sAIPathVer, sAIPath);
    RELEASE_AUTO(kAIPathStyleSuite, sAIPathStyleVer, sAIPathStyle);
    RELEASE_AUTO(kAICustomColorSuite, sAICustomColorVer, sAICustomColor);
    RELEASE_AUTO(kAISwatchListSuite, sAISwatchListVer, sAISwatchList);
    RELEASE_AUTO(kAIMatchingArtSuite, sAIMatchingArtVer, sAIMatchingArt);
    RELEASE_AUTO(kAIMdMemorySuite, sAIMdMemoryVer, sAIMdMemory);
    RELEASE_AUTO(kAIDocumentSuite, sAIDocVer, sAIDocument);
    RELEASE_AUTO(kAIArtboardSuite, sAIArtboardVer, sAIArtboard);
    RELEASE_AUTO(kAITransformArtSuite, sAITransformVer, sAITransformArt);
    RELEASE_AUTO(kAIUndoSuite, sAIUndoVer, sAIUndo);
    RELEASE_AUTO(kAIAppContextSuite, sAIAppContextVer, sAIAppContext);
    RELEASE_AUTO(kAIDocumentViewSuite, sAIDocumentViewVer, sAIDocumentView);
    RELEASE_AUTO(kAIDocumentListSuite, sAIDocumentListVer, sAIDocumentList);
    RELEASE_AUTO(kAIUIThemeSuite, sAIUIThemeVer, sAIUITheme);
    RELEASE_AUTO(kAINotifierSuite, sAINotifierVer, sAINotifier);
    RELEASE_AUTO(kAILayerSuite, sAILayerVer, sAILayer);
    RELEASE_AUTO(kAIUserSuite, sAIUserVer, sAIUser);
    RELEASE_AUTO(kAITextFrameSuite, sAITextFrameVer, sAITextFrame);
    RELEASE_AUTO(kAIATEPaintSuite, sAIATEPaintVer, sAIATEPaint);
    RELEASE_AUTO(kTextRangeSuite, sTextRangeVer, sTextRange);
    RELEASE_AUTO(kCharFeaturesSuite, sCharFeaturesVer, sCharFeatures);
    RELEASE_AUTO(kApplicationPaintSuite, sApplicationPaintVer, sApplicationPaint);

    sSPBasic = nullptr;
    PluginLog("[ReleaseSuites] All suites successfully released.");
}
