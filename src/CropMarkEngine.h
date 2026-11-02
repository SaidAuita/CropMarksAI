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

    int units = kUnitsMM;
    int strokeUnits = kUnitsPT;

    bool whiteContour = false;
    bool perimeterOnly = true;
    int boundsMode = kBoundsGeometric;
    int groupMode = kGroupMarksOnly;
    int targetMode = kTargetEachObject;

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

private:
    CropMarkEngine() = default;
    ~CropMarkEngine() = default;
    CropMarkEngine(const CropMarkEngine&) = delete;
    CropMarkEngine& operator=(const CropMarkEngine&) = delete;
};

#endif // CROP_MARK_ENGINE_H
