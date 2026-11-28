#include "Suites.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>

extern "C" {
    SPBasicSuite*         sSPBasic = nullptr;
    SPBlocksSuite*        sSPBlocks = nullptr;
    void*                 sAIAssertion = nullptr;
    AIPanelSuite*         sAIPanel = nullptr;
    AIMenuSuite*          sAIMenu = nullptr;
    AIUnicodeStringSuite* sAIUnicodeString = nullptr;
    AIArtSuite*           sAIArt = nullptr;
    AIGroupSuite*         sAIGroup = nullptr;
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
    SPPluginRef           gPluginRef = nullptr;
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
static int32_t sAIGroupVer = 0;
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

static const AIUnicodeStringSuite* sAIUnicodeStringRaw = nullptr;
static AIUnicodeStringSuite        sAIUnicodeStringProxy = {};

int32_t GetCharFeaturesSuiteVer() { return sCharFeaturesVer; }
int32_t GetDocumentSuiteVer()     { return sAIDocVer; }
int32_t GetUnicodeStringSuiteVer(){ return sAIUnicodeStringVer; }

static ai::int32 AIAPI ProxyUnicodeCompare_V8V9(const ai::UnicodeString& str, const ai::UnicodeString& str2) {
    if (sAIUnicodeStringProxy.CompareSubString) {
        return sAIUnicodeStringProxy.CompareSubString(str, 0, ai::UnicodeString::npos, str2, 0, ai::UnicodeString::npos);
    }
    return 0;
}

static ai::int32 AIAPI ProxyUnicodeCaseCompare_V8V9(const ai::UnicodeString& str, const ai::UnicodeString& str2) {
    if (sAIUnicodeStringProxy.CaseCompareSubString) {
        return sAIUnicodeStringProxy.CaseCompareSubString(str, 0, ai::UnicodeString::npos, str2, 0, ai::UnicodeString::npos);
    }
    return 0;
}

static void SetupUnicodeStringSuiteProxy() {
    if (!sAIUnicodeStringRaw) {
        sAIUnicodeString = nullptr;
        return;
    }
    if (sAIUnicodeStringVer >= 10) {
        sAIUnicodeString = const_cast<AIUnicodeStringSuite*>(sAIUnicodeStringRaw);
        return;
    }

    std::memset(&sAIUnicodeStringProxy, 0, sizeof(sAIUnicodeStringProxy));
    void* const* src = reinterpret_cast<void* const*>(sAIUnicodeStringRaw);
    void** dst = reinterpret_cast<void**>(&sAIUnicodeStringProxy);

    if (sAIUnicodeStringVer == 9) {
        // AI 2024-2025 (40 slots): slots 10 (Compare) and 14 (CaseCompare) were added in v10
        for (int i = 0; i <= 9; ++i) dst[i] = src[i];
        dst[10] = reinterpret_cast<void*>(&ProxyUnicodeCompare_V8V9);
        for (int i = 11; i <= 13; ++i) dst[i] = src[i - 1];
        dst[14] = reinterpret_cast<void*>(&ProxyUnicodeCaseCompare_V8V9);
        for (int i = 15; i <= 41; ++i) dst[i] = src[i - 2];
        PluginLog("  [UnicodeString Proxy] Activated v9 -> v10 vtable bridge");
    } else {
        // AI 2020-2023 (v8, 38 slots): also lacks 2 slots before HasSurrogates
        for (int i = 0; i <= 9; ++i) dst[i] = src[i];
        dst[10] = reinterpret_cast<void*>(&ProxyUnicodeCompare_V8V9);
        for (int i = 11; i <= 13; ++i) dst[i] = src[i - 1];
        dst[14] = reinterpret_cast<void*>(&ProxyUnicodeCaseCompare_V8V9);
        for (int i = 15; i <= 31; ++i) dst[i] = src[i - 2];
        dst[32] = src[30]; // Resize
        dst[35] = src[31]; // SubStr
        dst[36] = src[32]; // SwapStr
        for (int i = 37; i <= 41; ++i) dst[i] = src[i - 4]; // HasSurrogates..Normalize
        PluginLog("  [UnicodeString Proxy] Activated v%d -> v10 vtable bridge", sAIUnicodeStringVer);
    }

    sAIUnicodeString = &sAIUnicodeStringProxy;
}

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
        snprintf(logFile, sizeof(logFile), "%sCropMarksAI_Native.log", tempPath);
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
    int32_t maxVer = (preferredVer > 30) ? preferredVer : 30;
    for (int32_t v = maxVer; v >= 1; --v) {
        if (v == preferredVer) continue;
        // Skip unlikely middle range for 100+ ATE suite versions to keep startup instant
        if (preferredVer >= 100 && v < 100 && v > 10) continue;
        err = basic->AcquireSuite(name, v, suitePtr);
        if (err == kSPNoError && *suitePtr) {
            *outVer = v;
            PluginLog("  [ACQUIRE OK] %s (discovered version %d)", name, v);
            return kSPNoError;
        }
    }

    PluginLog("  [ACQUIRE FAIL] Could not acquire %s (tested versions 1..%d)", name, maxVer);
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
    AcquireSuiteAuto(basic, kAIUnicodeStringSuite, kAIUnicodeStringSuiteVersion, (const void**)&sAIUnicodeStringRaw, &sAIUnicodeStringVer);
    SetupUnicodeStringSuiteProxy();
    AcquireSuiteAuto(basic, kAIArtSuite, kAIArtSuiteVersion, (const void**)&sAIArt, &sAIArtVer);
    AcquireSuiteAuto(basic, kAIGroupSuite, kAIGroupSuiteVersion, (const void**)&sAIGroup, &sAIGroupVer);
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
    if (sAIUnicodeStringRaw && sAIUnicodeStringVer > 0) {
        basic->ReleaseSuite(kAIUnicodeStringSuite, sAIUnicodeStringVer);
        sAIUnicodeStringRaw = nullptr;
        sAIUnicodeString = nullptr;
        sAIUnicodeStringVer = 0;
    }
    RELEASE_AUTO(kAIArtSuite, sAIArtVer, sAIArt);
    RELEASE_AUTO(kAIGroupSuite, sAIGroupVer, sAIGroup);
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
