#pragma once

#ifndef CROP_MARK_ENGINE_H
#define CROP_MARK_ENGINE_H

#include "Suites.h"
#include <string>
#include <vector>

enum CropUnits {
    kUnitsMM = 0,
    kUnitsCM = 1,
    kUnitsPT = 2,
    kUnitsIN = 3
};

enum BoundsMode {
    kBoundsGeometric = 0,
    kBoundsVisible = 1
};

enum GroupMode {
    kGroupNone = 0,
    kGroupMarksOnly = 1,
    kGroupMarksAndSelection = 2
};

enum TargetMode {
    kTargetEachObject = 0,
    kTargetEntireSelection = 1,
    kTargetArtboard = 2
};

enum CropMarkLayerOrder {
    kCropMarksAbovePatches = 0, // Above color bars (default)
    kCropMarksBelowPatches = 1  // Below color bars
};

enum CrossMarksLayerOrder {
    kCrossMarksAbovePatches = 0, // Above color bars (default)
    kCrossMarksBelowPatches = 1  // Below color bars
};

enum ColorBarLength {
    kColorBarShort = 0, // Short
    kColorBarLong = 1   // Long
};

enum ColorBarStandard {
    kStandardISO12647_2 = 0, // ISO 12647-2 (Offset)
    kStandardISO12647_7 = 1  // ISO 12647-7 (Digital Proof)
};

enum ColorBarMode {
    kColorBar100Solids = 0,         // 100% Solids + RGB Binaries + Pantones
    kColorBar100PrimariesOnly = 1,  // 100% Primaries (CMYK + Pantones only, no RGB/binaries)
    kColorBarISO12647_2Mode = 2,    // ISO 12647-2 (Halftones)
    kColorBarISO12647_7Mode = 3     // ISO 12647-7 (Proofing)
};

enum CropMarkLayerMode {
    kLayerCurrent = 0,    // Current / active layer
    kLayerSpecific = 1    // Specific or new layer
};

enum RegMarkPositions {
    kRegPositionsCenters = 0,        // 4 marks: Top, Bottom, Left, Right centers
    kRegPositionsCorners = 1,        // 4 marks: TL, TR, BL, BR
    kRegPositionsCentersCorners = 2, // 8 marks: Centers + Corners
    kRegPositionsAll16 = 3           // 16 marks: Anosov perimeter (5 top, 5 bottom, 3 left, 3 right)
};

struct DocumentSpotColor {
    std::string name;
    AICustomColorHandle handle = nullptr;
    float c = 0.0f;
    float m = 0.0f;
    float y = 0.0f;
    float k = 0.0f;
    bool isSelected = true;
    bool isTechnical = false;
};

struct CropMarkDirections {
    // Top
    bool topLeft = true;
    bool topCenter = false;
    bool topRight = true;

    // Bottom
    bool bottomLeft = true;
    bool bottomCenter = false;
    bool bottomRight = true;

    // Left
    bool leftTop = true;
    bool leftCenter = false;
    bool leftBottom = true;

    // Right
    bool rightTop = true;
    bool rightCenter = false;
    bool rightBottom = true;
};

struct CropMarkParams {
    CropMarkDirections dirs;

    // Values in active units
    double lengthVer = 5.0;
    double lengthHor = 5.0;
    double offsetVer = 2.0;
    double offsetHor = 2.0;
    double bleedVer = 0.0;
    double bleedHor = 0.0;
    double strokeWidth = 0.25;
    double barsOffset = 0.0;
    double crossOffset = 0.0;
    double barRepeatGap = 0.0; // mm gap between repeated strips, default 0.0

    int units = kUnitsMM;
    int strokeUnits = kUnitsPT;

    bool whiteContour = false;
    bool perimeterOnly = true;
    bool regMarks = false;
    bool colorBarsLong = false;
    bool colorBarsShort = false;
    int cropMarksLayerOrder = kCropMarksAbovePatches;
    int crossMarksLayerOrder = kCrossMarksAbovePatches;
    int barLength = kColorBarShort;
    int barStandard = kStandardISO12647_2;
    bool docInfo = false;
    bool docInfoTranslit = false;
    int boundsMode = kBoundsGeometric;
    int groupMode = kGroupMarksOnly;
    int targetMode = kTargetEachObject;
    int layerMode = kLayerCurrent;
    std::string layerName = "";

    // Registration Marks Advanced Adjustments (Anosov model)
    double regCircleDiam = 4.0;     // mm
    double regCrossLength = 6.0;    // mm
    double regStrokeWidth = 0.10;   // in regStrokeUnits
    int regStrokeUnits = kUnitsMM;
    double regOffsetHor = 2.0;      // in units
    double regOffsetVer = 2.0;      // in units
    int regPositions = kRegPositionsCenters;
    bool regWhiteUnderlay = true;

    // Color Bars Advanced Adjustments (100% Solids mode + Spot selection)
    int barMode = kColorBar100Solids;
    double patchWidth = 5.0;        // in units (default mm)
    double patchHeight = 5.0;       // in units (default mm)
    bool barCyan = true;
    bool barMagenta = true;
    bool barYellow = true;
    bool barBlack = true;
    std::vector<DocumentSpotColor> selectedSpots;

    // Conversion helpers
    static double UnitToPoints(double val, int u);
    static double PointsToUnit(double pt, int u);
    static const char* GetUnitName(int u);
};

struct PresetItem {
    int id = 1;
    std::string name;
    CropMarkParams params;
};

struct CropMarkResult {
    bool success = false;
    int marksCount = 0;
    int objectsCount = 0;
    double elapsedMs = 0.0;
    std::string message;
};

class CropMarkEngine {
public:
    static CropMarkEngine& GetInstance();

    CropMarkResult CreateCropMarks(const CropMarkParams& params);

    static AIRealRect GetObjectBounds(AIArtHandle art, int boundsMode);
    static bool IsClippingMask(AIArtHandle art, AIArtHandle* outClipPath);
    static bool IsGuide(AIArtHandle art);
    static bool IsLayerArt(AIArtHandle art);
    static bool HasSelectedAncestor(AIArtHandle art, const std::vector<AIArtHandle>& allMatches);
    static bool IsCropMarksGroup(AIArtHandle art);
    static void RemoveExistingCropMarks(AIArtHandle art);

    // Document Pantone / Spot Color Scanning & Prepress Filtering
    static bool IsTechnicalSpotName(const std::string& name);
    static std::vector<DocumentSpotColor> ScanDocumentPantones();
    static std::string TransliterateCyrillic(const std::string& src);

private:
    CropMarkEngine() = default;
    ~CropMarkEngine() = default;
    CropMarkEngine(const CropMarkEngine&) = delete;
    CropMarkEngine& operator=(const CropMarkEngine&) = delete;
};

#endif // CROP_MARK_ENGINE_H
