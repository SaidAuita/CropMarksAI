#include "CropMarkEngine.h"
#include <cmath>
#include <vector>
#include <chrono>
#include <ctime>

double CropMarkParams::UnitToPoints(double val, int u) {
    switch (u) {
        case kUnitsMM: return val * (72.0 / 25.4);
        case kUnitsCM: return val * ((72.0 / 25.4) * 10.0);
        case kUnitsPT: return val;
        case kUnitsIN: return val * 72.0;
        default:       return val * (72.0 / 25.4);
    }
}

double CropMarkParams::PointsToUnit(double pt, int u) {
    switch (u) {
        case kUnitsMM: return pt / (72.0 / 25.4);
        case kUnitsCM: return pt / ((72.0 / 25.4) * 10.0);
        case kUnitsPT: return pt;
        case kUnitsIN: return pt / 72.0;
        default:       return pt / (72.0 / 25.4);
    }
}

const char* CropMarkParams::GetUnitName(int u) {
    switch (u) {
        case kUnitsMM: return "mm";
        case kUnitsCM: return "cm";
        case kUnitsPT: return "pt";
        case kUnitsIN: return "in";
        default:       return "mm";
    }
}

CropMarkEngine& CropMarkEngine::GetInstance() {
    static CropMarkEngine instance;
    return instance;
}

bool CropMarkEngine::IsGuide(AIArtHandle art) {
    if (!sAIArt || !art) return false;
    short type = 0;
    sAIArt->GetArtType(art, &type);
    if (type == kPathArt && sAIPath) {
        AIBoolean isG = false;
        if (sAIPath->GetPathGuide(art, &isG) == kNoErr) {
            return isG != 0;
        }
    }
    return false;
}

bool CropMarkEngine::IsClippingMask(AIArtHandle art, AIArtHandle* outClipPath) {
    if (!sAIArt || !art) return false;
    short type = 0;
    sAIArt->GetArtType(art, &type);
    if (type != kGroupArt) return false;

    ai::int32 attr = 0;
    sAIArt->GetArtUserAttr(art, kArtIsClipMask, &attr);
    if ((attr & kArtIsClipMask) == 0) return false;

    // Iterate children to find the clipping path
    AIArtHandle child = nullptr;
    sAIArt->GetArtFirstChild(art, &child);
    while (child) {
        ai::int32 childAttr = 0;
        sAIArt->GetArtUserAttr(child, kArtIsClipMask, &childAttr);
        if (childAttr & kArtIsClipMask) {
            if (outClipPath) *outClipPath = child;
            return true;
        }
        sAIArt->GetArtSibling(child, &child);
    }
    return false;
}

bool CropMarkEngine::IsLayerArt(AIArtHandle art) {
    if (!art || !sAIArt) return false;
    ASBoolean isLayer = false;
    if (sAIArt->IsArtLayerGroup(art, &isLayer) == kNoErr && isLayer) {
        return true;
    }
    return false;
}

bool CropMarkEngine::HasSelectedAncestor(AIArtHandle art, const std::vector<AIArtHandle>& allMatches) {
    if (!art || !sAIArt) return false;
    AIArtHandle curr = art;
    AIArtHandle parent = nullptr;
    while (sAIArt->GetArtParent(curr, &parent) == kNoErr && parent) {
        if (IsLayerArt(parent)) {
            break;
        }
        for (AIArtHandle m : allMatches) {
            if (m == parent) {
                return true;
            }
        }
        curr = parent;
    }
    return false;
}

bool CropMarkEngine::IsCropMarksGroup(AIArtHandle art) {
    if (!art || !sAIArt) return false;
    short type = 0;
    if (sAIArt->GetArtType(art, &type) != kNoErr) return false;
    if (type != kGroupArt) return false;
    ai::UnicodeString name;
    AIBoolean isDefaultName = false;
    if (sAIArt->GetArtName(art, name, &isDefaultName) == kNoErr) {
        std::string sName = name.as_Platform();
        if (sName == "Crop Marks") return true;
    }
    return false;
}

void CropMarkEngine::RemoveExistingCropMarks(AIArtHandle art) {
    if (!art || !sAIArt) return;
    short type = 0;
    if (sAIArt->GetArtType(art, &type) != kNoErr) return;

    if (type == kGroupArt) {
        AIArtHandle child = nullptr;
        sAIArt->GetArtFirstChild(art, &child);
        while (child) {
            AIArtHandle next = nullptr;
            sAIArt->GetArtSibling(child, &next);
            if (IsCropMarksGroup(child)) {
                sAIArt->DisposeArt(child);
            } else {
                RemoveExistingCropMarks(child);
            }
            child = next;
        }
    }

    // Also check immediate sibling below art (e.g. from previous kGroupMarksOnly run)
    AIArtHandle sibling = nullptr;
    if (sAIArt->GetArtSibling(art, &sibling) == kNoErr && sibling && IsCropMarksGroup(sibling)) {
        sAIArt->DisposeArt(sibling);
    }
}

AIRealRect CropMarkEngine::GetObjectBounds(AIArtHandle art, int boundsMode) {
    AIRealRect rc = { 0, 0, 0, 0 };
    if (!sAIArt || !art) return rc;

    // Check if object is a clipping group
    AIArtHandle clipPath = nullptr;
    if (IsClippingMask(art, &clipPath) && clipPath) {
        art = clipPath;
    }

    AIRealRect visBounds = { 0, 0, 0, 0 };
    AIErr visErr = sAIArt->GetArtBounds(art, &visBounds);
    if (visBounds.left > visBounds.right) std::swap(visBounds.left, visBounds.right);
    if (visBounds.bottom > visBounds.top) std::swap(visBounds.bottom, visBounds.top);

    if (boundsMode == kBoundsVisible) {
        rc = visBounds;
    } else {
        // Geometric bounds (control points / bezier outlines without stroke)
        AIRealRect geomBounds = { 0, 0, 0, 0 };
        AIErr geomErr = sAIArt->GetArtTransformBounds(art, nullptr, kNoStrokeBounds | kExcludeGuideBounds, &geomBounds);
        if (geomBounds.left > geomBounds.right) std::swap(geomBounds.left, geomBounds.right);
        if (geomBounds.bottom > geomBounds.top) std::swap(geomBounds.bottom, geomBounds.top);

        double visCx = (visBounds.left + visBounds.right) * 0.5;
        double visCy = (visBounds.top + visBounds.bottom) * 0.5;
        double geomCx = (geomBounds.left + geomBounds.right) * 0.5;
        double geomCy = (geomBounds.top + geomBounds.bottom) * 0.5;

        // If GetArtTransformBounds with nullptr failed or returned local coordinates (e.g. from artboard 1
        // while the object is placed on artboard 2), fall back to world bounds from GetArtBounds!
        if (geomErr != kNoErr ||
            (geomBounds.left == 0 && geomBounds.right == 0 && geomBounds.top == 0 && geomBounds.bottom == 0) ||
            std::abs(visCx - geomCx) > 5.0 ||
            std::abs(visCy - geomCy) > 5.0) {
            rc = visBounds;
        } else {
            rc = geomBounds;
        }
    }

    if (rc.left > rc.right) std::swap(rc.left, rc.right);
    if (rc.bottom > rc.top) std::swap(rc.bottom, rc.top);

    PluginLog("[GetObjectBounds] art=%p mode=%d -> L=%.2f, T=%.2f, R=%.2f, B=%.2f",
              art, boundsMode, rc.left, rc.top, rc.right, rc.bottom);
    return rc;
}

struct LineDedupeEntry {
    double x1, y1, x2, y2;
    bool isReg;
};

static bool AreLinesEqual(const LineDedupeEntry& a, const LineDedupeEntry& b, double prec = 0.002) {
    if (a.isReg != b.isReg) return false;
    return (std::abs(a.x1 - b.x1) <= prec && std::abs(a.y1 - b.y1) <= prec &&
            std::abs(a.x2 - b.x2) <= prec && std::abs(a.y2 - b.y2) <= prec) ||
           (std::abs(a.x1 - b.x2) <= prec && std::abs(a.y1 - b.y2) <= prec &&
            std::abs(a.x2 - b.x1) <= prec && std::abs(a.y2 - b.y1) <= prec);
}

static AIArtHandle CreateLineArt(AIArtHandle container, double x1, double y1, double x2, double y2,
                                 bool isRegistration, double strokeWidthPt) {
    if (!sAIArt || !sAIPath || !sAIPathStyle || !container) return nullptr;

    AIArtHandle pathArt = nullptr;
    AIErr err = sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, container, &pathArt);
    if (err != kNoErr || !pathArt) {
        PluginLog("  [CreateLineArt FAIL] NewArt err=%d container=%p", err, container);
        return nullptr;
    }

    sAIPath->SetPathSegmentCount(pathArt, 2);
    AIPathSegment segs[2];

    segs[0].p.h = (AIReal)x1;
    segs[0].p.v = (AIReal)y1;
    segs[0].in = segs[0].p;
    segs[0].out = segs[0].p;
    segs[0].corner = true;

    segs[1].p.h = (AIReal)x2;
    segs[1].p.v = (AIReal)y2;
    segs[1].in = segs[1].p;
    segs[1].out = segs[1].p;
    segs[1].corner = true;

    sAIPath->SetPathSegments(pathArt, 0, 2, segs);
    sAIPath->SetPathClosed(pathArt, false);

    AIPathStyle style;
    AIBoolean hasStyle = false;
    sAIPathStyle->GetPathStyle(pathArt, &style, &hasStyle);

    style.fillPaint = false;
    style.strokePaint = true;
    style.stroke.width = (AIReal)strokeWidthPt;
    style.stroke.cap = kAIButtCap;
    style.stroke.join = kAIMiterJoin;
    style.stroke.dash.length = 0;
    style.stroke.overprint = false;

    ai::int16 docColorModel = kDocCMYKColor;
    if (sAIDocument) sAIDocument->GetDocumentColorModel(&docColorModel);

    if (isRegistration) {
        AICustomColorHandle hReg = nullptr;
        if (sAICustomColor && sAICustomColor->GetCurrentRegistrationColor(&hReg) == kNoErr && hReg) {
            style.stroke.color.kind = kCustomColor;
            style.stroke.color.c.c.color = hReg;
            style.stroke.color.c.c.tint = 0.0f; // In Illustrator SDK, 0.0f is 100% ink (solid black registration)!
        } else {
            // Fallback: 100% black in active document color mode
            if (docColorModel == kDocRGBColor) {
                style.stroke.color.kind = kThreeColor;
                style.stroke.color.c.rgb.red = 0.0f;
                style.stroke.color.c.rgb.green = 0.0f;
                style.stroke.color.c.rgb.blue = 0.0f;
            } else {
                style.stroke.color.kind = kFourColor;
                style.stroke.color.c.f.cyan = 0.0f;
                style.stroke.color.c.f.magenta = 0.0f;
                style.stroke.color.c.f.yellow = 0.0f;
                style.stroke.color.c.f.black = 1.0f;
            }
        }
    } else {
        // White knockout underlay
        if (docColorModel == kDocRGBColor) {
            style.stroke.color.kind = kThreeColor;
            style.stroke.color.c.rgb.red = 1.0f;
            style.stroke.color.c.rgb.green = 1.0f;
            style.stroke.color.c.rgb.blue = 1.0f;
        } else {
            style.stroke.color.kind = kFourColor;
            style.stroke.color.c.f.cyan = 0.0f;
            style.stroke.color.c.f.magenta = 0.0f;
            style.stroke.color.c.f.yellow = 0.0f;
            style.stroke.color.c.f.black = 0.0f;
        }
    }

    sAIPathStyle->SetPathStyle(pathArt, &style);
    return pathArt;
}

static void CreateRegistrationCircle(AIArtHandle parentGroup, double cx, double cy, double r, bool isReg, double strokeWidthPt) {
    AIArtHandle pathArt = nullptr;
    sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, parentGroup, &pathArt);
    if (!pathArt) return;

    const double k = r * 0.5522847498307936;
    AIPathSegment segs[4];

    segs[0].p.h = (AIReal)(cx + r); segs[0].p.v = (AIReal)cy;
    segs[0].in.h = (AIReal)(cx + r); segs[0].in.v = (AIReal)(cy - k);
    segs[0].out.h = (AIReal)(cx + r); segs[0].out.v = (AIReal)(cy + k);
    segs[0].corner = false;

    segs[1].p.h = (AIReal)cx; segs[1].p.v = (AIReal)(cy + r);
    segs[1].in.h = (AIReal)(cx + k); segs[1].in.v = (AIReal)(cy + r);
    segs[1].out.h = (AIReal)(cx - k); segs[1].out.v = (AIReal)(cy + r);
    segs[1].corner = false;

    segs[2].p.h = (AIReal)(cx - r); segs[2].p.v = (AIReal)cy;
    segs[2].in.h = (AIReal)(cx - r); segs[2].in.v = (AIReal)(cy + k);
    segs[2].out.h = (AIReal)(cx - r); segs[2].out.v = (AIReal)(cy - k);
    segs[2].corner = false;

    segs[3].p.h = (AIReal)cx; segs[3].p.v = (AIReal)(cy - r);
    segs[3].in.h = (AIReal)(cx - k); segs[3].in.v = (AIReal)(cy - r);
    segs[3].out.h = (AIReal)(cx + k); segs[3].out.v = (AIReal)(cy - r);
    segs[3].corner = false;

    sAIPath->SetPathSegments(pathArt, 0, 4, segs);
    sAIPath->SetPathClosed(pathArt, true);

    AIPathStyle style;
    AIBoolean hasStyle = false;
    sAIPathStyle->GetPathStyle(pathArt, &style, &hasStyle);
    style.fillPaint = false;
    style.strokePaint = true;
    style.stroke.width = (AIReal)strokeWidthPt;
    style.stroke.cap = kAIButtCap;
    style.stroke.join = kAIMiterJoin;
    style.stroke.dash.length = 0;
    style.stroke.overprint = false;

    ai::int16 docColorModel = kDocCMYKColor;
    if (sAIDocument) sAIDocument->GetDocumentColorModel(&docColorModel);

    if (isReg) {
        AICustomColorHandle hReg = nullptr;
        if (sAICustomColor && sAICustomColor->GetCurrentRegistrationColor(&hReg) == kNoErr && hReg) {
            style.stroke.color.kind = kCustomColor;
            style.stroke.color.c.c.color = hReg;
            style.stroke.color.c.c.tint = 0.0f;
        } else {
            if (docColorModel == kDocRGBColor) {
                style.stroke.color.kind = kThreeColor;
                style.stroke.color.c.rgb.red = style.stroke.color.c.rgb.green = style.stroke.color.c.rgb.blue = 0.0f;
            } else {
                style.stroke.color.kind = kFourColor;
                style.stroke.color.c.f.cyan = style.stroke.color.c.f.magenta = style.stroke.color.c.f.yellow = 0.0f;
                style.stroke.color.c.f.black = 1.0f;
            }
        }
    } else {
        if (docColorModel == kDocRGBColor) {
            style.stroke.color.kind = kThreeColor;
            style.stroke.color.c.rgb.red = style.stroke.color.c.rgb.green = style.stroke.color.c.rgb.blue = 1.0f;
        } else {
            style.stroke.color.kind = kFourColor;
            style.stroke.color.c.f.cyan = style.stroke.color.c.f.magenta = style.stroke.color.c.f.yellow = style.stroke.color.c.f.black = 0.0f;
        }
    }
    sAIPathStyle->SetPathStyle(pathArt, &style);
}

static void CreateRegistrationTarget(AIArtHandle parentGroup, double cx, double cy, bool whiteUnderlay, double strokeWidthPt) {
    // User specs: circle diam = 4.0 mm (r = 2.0 mm), crosshair length = 6.0 mm (arm = 3.0 mm)
    const double ptPerMm = 2.83464567;
    const double rOuter = 2.0 * ptPerMm; // diam 4.0 mm
    const double arm = 3.0 * ptPerMm;    // span 6.0 mm

    // White underlay (+1.2 pt) ensures crosshair is clearly visible even over color bars
    if (whiteUnderlay) {
        double wStroke = strokeWidthPt + 1.2;
        CreateRegistrationCircle(parentGroup, cx, cy, rOuter, false, wStroke);
        CreateLineArt(parentGroup, cx - arm, cy, cx + arm, cy, false, wStroke);
        CreateLineArt(parentGroup, cx, cy - arm, cx, cy + arm, false, wStroke);
    }

    CreateRegistrationCircle(parentGroup, cx, cy, rOuter, true, strokeWidthPt);
    CreateLineArt(parentGroup, cx - arm, cy, cx + arm, cy, true, strokeWidthPt);
    CreateLineArt(parentGroup, cx, cy - arm, cx, cy + arm, true, strokeWidthPt);
}

struct ColorPatchDef {
    float c, m, y, k;
};

// 1. ISO 12647-2 (Offset) - Short (23 patches)
static const ColorPatchDef g_iso2_short[] = {
    // Primary solids (C, M, Y, K)
    { 1.0f, 0.0f, 0.0f, 0.0f }, // Cyan 100%
    { 0.0f, 1.0f, 0.0f, 0.0f }, // Magenta 100%
    { 0.0f, 0.0f, 1.0f, 0.0f }, // Yellow 100%
    { 0.0f, 0.0f, 0.0f, 1.0f }, // Black 100%
    // Secondaries & Overprints
    { 0.0f, 1.0f, 1.0f, 0.0f }, // Red (M+Y)
    { 1.0f, 0.0f, 1.0f, 0.0f }, // Green (C+Y)
    { 1.0f, 1.0f, 0.0f, 0.0f }, // Blue (C+M)
    { 1.0f, 1.0f, 1.0f, 0.0f }, // 3-color CMY solid
    { 0.60f, 0.50f, 0.50f, 1.0f }, // CMYK Rich Black
    // Halftones (key ISO control points: 75%, 50%, 25%)
    { 0.75f, 0.0f, 0.0f, 0.0f }, // C 75%
    { 0.0f, 0.75f, 0.0f, 0.0f }, // M 75%
    { 0.0f, 0.0f, 0.75f, 0.0f }, // Y 75%
    { 0.0f, 0.0f, 0.0f, 0.75f }, // K 75%
    { 0.50f, 0.0f, 0.0f, 0.0f }, // C 50%
    { 0.0f, 0.50f, 0.0f, 0.0f }, // M 50%
    { 0.0f, 0.0f, 0.50f, 0.0f }, // Y 50%
    { 0.0f, 0.0f, 0.0f, 0.50f }, // K 50%
    { 0.25f, 0.0f, 0.0f, 0.0f }, // C 25%
    { 0.0f, 0.25f, 0.0f, 0.0f }, // M 25%
    { 0.0f, 0.0f, 0.25f, 0.0f }, // Y 25%
    { 0.0f, 0.0f, 0.0f, 0.25f }, // K 25%
    // ISO 12647-2 Midtone Gray Balance
    { 0.50f, 0.40f, 0.40f, 0.0f }, // CMY Neutral Midtone
    { 0.0f, 0.0f, 0.0f, 0.0f }     // Paper White
};

// 2. ISO 12647-2 (Offset) - Long (45 patches)
static const ColorPatchDef g_iso2_long[] = {
    // Primary solids
    { 1.0f, 0.0f, 0.0f, 0.0f }, // Cyan 100%
    { 0.0f, 1.0f, 0.0f, 0.0f }, // Magenta 100%
    { 0.0f, 0.0f, 1.0f, 0.0f }, // Yellow 100%
    { 0.0f, 0.0f, 0.0f, 1.0f }, // Black 100%
    // Secondaries & Trapping
    { 0.0f, 1.0f, 1.0f, 0.0f }, // Red (M+Y)
    { 1.0f, 0.0f, 1.0f, 0.0f }, // Green (C+Y)
    { 1.0f, 1.0f, 0.0f, 0.0f }, // Blue (C+M)
    { 1.0f, 1.0f, 1.0f, 0.0f }, // 3-Color CMY Solid
    { 0.60f, 0.50f, 0.50f, 1.0f }, // 4-Color Rich Black
    { 1.0f, 1.0f, 1.0f, 1.0f }, // Max TAC (C+M+Y+K)
    // Detailed TVI Halftone Ramps (80%, 70%, 50%, 40%, 25%, 10%)
    { 0.80f, 0.0f, 0.0f, 0.0f }, { 0.70f, 0.0f, 0.0f, 0.0f }, { 0.50f, 0.0f, 0.0f, 0.0f },
    { 0.40f, 0.0f, 0.0f, 0.0f }, { 0.25f, 0.0f, 0.0f, 0.0f }, { 0.10f, 0.0f, 0.0f, 0.0f }, // Cyan
    { 0.0f, 0.80f, 0.0f, 0.0f }, { 0.0f, 0.70f, 0.0f, 0.0f }, { 0.0f, 0.50f, 0.0f, 0.0f },
    { 0.0f, 0.40f, 0.0f, 0.0f }, { 0.0f, 0.25f, 0.0f, 0.0f }, { 0.0f, 0.10f, 0.0f, 0.0f }, // Magenta
    { 0.0f, 0.0f, 0.80f, 0.0f }, { 0.0f, 0.0f, 0.70f, 0.0f }, { 0.0f, 0.0f, 0.50f, 0.0f },
    { 0.0f, 0.0f, 0.40f, 0.0f }, { 0.0f, 0.0f, 0.25f, 0.0f }, { 0.0f, 0.0f, 0.10f, 0.0f }, // Yellow
    { 0.0f, 0.0f, 0.0f, 0.80f }, { 0.0f, 0.0f, 0.0f, 0.70f }, { 0.0f, 0.0f, 0.0f, 0.50f },
    { 0.0f, 0.0f, 0.0f, 0.40f }, { 0.0f, 0.0f, 0.0f, 0.25f }, { 0.0f, 0.0f, 0.0f, 0.10f }, // Black
    // Gray balance checkpoints (bvdm / ISO 12647-2)
    { 0.25f, 0.19f, 0.19f, 0.0f }, // Gray Highlight vs K25
    { 0.0f, 0.0f, 0.0f, 0.25f },
    { 0.50f, 0.40f, 0.40f, 0.0f }, // Gray Midtone vs K50
    { 0.0f, 0.0f, 0.0f, 0.50f },
    { 0.75f, 0.64f, 0.64f, 0.0f }, // Gray Shadow vs K75
    { 0.0f, 0.0f, 0.0f, 0.75f },
    // Secondary halftones & neutral
    { 0.0f, 0.50f, 0.50f, 0.0f }, // Red 50%
    { 0.50f, 0.0f, 0.50f, 0.0f }, // Green 50%
    { 0.50f, 0.50f, 0.0f, 0.0f }, // Blue 50%
    { 0.50f, 0.50f, 0.50f, 0.0f }, // 3-Color Mid
    { 0.0f, 0.0f, 0.0f, 0.0f }     // Paper White
};

// 3. ISO 12647-7 (Digital Proof) - Short (23 patches, Fogra Media Wedge subset)
static const ColorPatchDef g_iso7_short[] = {
    // Primary solids
    { 1.0f, 0.0f, 0.0f, 0.0f }, // Cyan 100%
    { 0.0f, 1.0f, 0.0f, 0.0f }, // Magenta 100%
    { 0.0f, 0.0f, 1.0f, 0.0f }, // Yellow 100%
    { 0.0f, 0.0f, 0.0f, 1.0f }, // Black 100%
    // Secondaries & 3-Color
    { 0.0f, 1.0f, 1.0f, 0.0f }, // Red (M+Y)
    { 1.0f, 0.0f, 1.0f, 0.0f }, // Green (C+Y)
    { 1.0f, 1.0f, 0.0f, 0.0f }, // Blue (C+M)
    { 1.0f, 1.0f, 1.0f, 0.0f }, // 3-Color CMY
    // Fogra MW3 Primary Tints (70%, 40%, 10%)
    { 0.70f, 0.0f, 0.0f, 0.0f }, { 0.40f, 0.0f, 0.0f, 0.0f }, { 0.10f, 0.0f, 0.0f, 0.0f }, // Cyan
    { 0.0f, 0.70f, 0.0f, 0.0f }, { 0.0f, 0.40f, 0.0f, 0.0f }, { 0.0f, 0.10f, 0.0f, 0.0f }, // Magenta
    { 0.0f, 0.0f, 0.70f, 0.0f }, { 0.0f, 0.0f, 0.40f, 0.0f }, { 0.0f, 0.0f, 0.10f, 0.0f }, // Yellow
    { 0.0f, 0.0f, 0.0f, 0.70f }, { 0.0f, 0.0f, 0.0f, 0.40f }, { 0.0f, 0.0f, 0.0f, 0.10f }, // Black
    // Proof Near-Neutrals
    { 0.50f, 0.40f, 0.40f, 0.0f }, // CMY Neutral Mid
    { 0.30f, 0.23f, 0.23f, 0.0f }, // CMY Neutral Low
    { 0.0f, 0.0f, 0.0f, 0.0f }     // Paper White
};

// 4. ISO 12647-7 (Digital Proof) - Long (47 patches, Extended Fogra Media Wedge subset)
static const ColorPatchDef g_iso7_long[] = {
    // Primary solids
    { 1.0f, 0.0f, 0.0f, 0.0f }, // Cyan 100%
    { 0.0f, 1.0f, 0.0f, 0.0f }, // Magenta 100%
    { 0.0f, 0.0f, 1.0f, 0.0f }, // Yellow 100%
    { 0.0f, 0.0f, 0.0f, 1.0f }, // Black 100%
    // Secondaries & Tints (100%, 70%, 40%)
    { 0.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, 0.70f, 0.70f, 0.0f }, { 0.0f, 0.40f, 0.40f, 0.0f }, // Red
    { 1.0f, 0.0f, 1.0f, 0.0f }, { 0.70f, 0.0f, 0.70f, 0.0f }, { 0.40f, 0.0f, 0.40f, 0.0f }, // Green
    { 1.0f, 1.0f, 0.0f, 0.0f }, { 0.70f, 0.70f, 0.0f, 0.0f }, { 0.40f, 0.40f, 0.0f, 0.0f }, // Blue
    { 1.0f, 1.0f, 1.0f, 0.0f }, // 3-Color CMY
    // Extended Primary Tints (90%, 70%, 50%, 40%, 20%, 10%)
    { 0.90f, 0.0f, 0.0f, 0.0f }, { 0.70f, 0.0f, 0.0f, 0.0f }, { 0.50f, 0.0f, 0.0f, 0.0f },
    { 0.40f, 0.0f, 0.0f, 0.0f }, { 0.20f, 0.0f, 0.0f, 0.0f }, { 0.10f, 0.0f, 0.0f, 0.0f }, // Cyan
    { 0.0f, 0.90f, 0.0f, 0.0f }, { 0.0f, 0.70f, 0.0f, 0.0f }, { 0.0f, 0.50f, 0.0f, 0.0f },
    { 0.0f, 0.40f, 0.0f, 0.0f }, { 0.0f, 0.20f, 0.0f, 0.0f }, { 0.0f, 0.10f, 0.0f, 0.0f }, // Magenta
    { 0.0f, 0.0f, 0.90f, 0.0f }, { 0.0f, 0.0f, 0.70f, 0.0f }, { 0.0f, 0.0f, 0.50f, 0.0f },
    { 0.0f, 0.0f, 0.40f, 0.0f }, { 0.0f, 0.0f, 0.20f, 0.0f }, { 0.0f, 0.0f, 0.10f, 0.0f }, // Yellow
    { 0.0f, 0.0f, 0.0f, 0.90f }, { 0.0f, 0.0f, 0.0f, 0.70f }, { 0.0f, 0.0f, 0.0f, 0.50f },
    { 0.0f, 0.0f, 0.0f, 0.40f }, { 0.0f, 0.0f, 0.0f, 0.20f }, { 0.0f, 0.0f, 0.0f, 0.10f }, // Black
    // Fogra Proofing Near-Neutrals & Standard Test Patches
    { 0.15f, 0.11f, 0.11f, 0.0f }, // Light Neutral
    { 0.40f, 0.30f, 0.30f, 0.0f }, // Mid Neutral
    { 0.70f, 0.60f, 0.60f, 0.0f }, // Dark Neutral
    { 0.15f, 0.30f, 0.35f, 0.0f }, // Skin Tone
    { 0.35f, 0.15f, 0.05f, 0.0f }, // Sky Blue
    { 0.45f, 0.15f, 0.70f, 0.0f }, // Foliage Green
    { 0.50f, 0.40f, 0.40f, 0.0f }, // 3-Color Mid
    { 1.0f, 1.0f, 1.0f, 0.60f }, // Rich Black TAC
    { 0.0f, 0.0f, 0.0f, 0.0f }     // Paper White
};

static int CreateColorBarStrip(
    AIArtHandle parentGroup,
    double startX, double startY,
    bool isHorizontal,
    double patchSizePt,
    double limitCoordPt,
    int standard,
    int length
) {
    const ColorPatchDef* patches = nullptr;
    size_t numPatches = 0;

    if (standard == kStandardISO12647_7) {
        if (length == kColorBarLong) {
            patches = g_iso7_long;
            numPatches = sizeof(g_iso7_long) / sizeof(g_iso7_long[0]);
        } else {
            patches = g_iso7_short;
            numPatches = sizeof(g_iso7_short) / sizeof(g_iso7_short[0]);
        }
    } else {
        // Default: kStandardISO12647_2
        if (length == kColorBarLong) {
            patches = g_iso2_long;
            numPatches = sizeof(g_iso2_long) / sizeof(g_iso2_long[0]);
        } else {
            patches = g_iso2_short;
            numPatches = sizeof(g_iso2_short) / sizeof(g_iso2_short[0]);
        }
    }

    if (numPatches == 0 || patchSizePt <= 0.0) {
        return 0;
    }

    const double ptPerMm = 2.83464567;
    const double gapPt = 4.0 * ptPerMm;
    double currentOffset = 0.0;
    bool reachedLimit = false;
    int createdCount = 0;

    while (!reachedLimit) {
        for (size_t p = 0; p < numPatches; ++p) {
            double px = isHorizontal ? (startX + currentOffset) : startX;
            double py = isHorizontal ? startY : (startY - currentOffset);

            // Check if patch exceeds boundary:
            // Horizontal: right edge of patch (px + patchSizePt) must not exceed limitCoordPt (X_max = last_crop - 3mm)
            // Vertical: bottom edge of patch (py - patchSizePt) must not be below limitCoordPt (Y_min = last_crop + 3mm)
            if (isHorizontal) {
                if (px + patchSizePt > limitCoordPt + 0.01) {
                    reachedLimit = true;
                    break; // Stop drawing further patches
                }
            } else {
                if (py - patchSizePt < limitCoordPt - 0.01) {
                    reachedLimit = true;
                    break; // Stop drawing further patches
                }
            }

            AIArtHandle pathArt = nullptr;
            sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, parentGroup, &pathArt);
            if (!pathArt) continue;

            AIPathSegment segs[4];
            segs[0].p.h = (AIReal)px; segs[0].p.v = (AIReal)(py - patchSizePt);
            segs[0].in = segs[0].out = segs[0].p; segs[0].corner = true;

            segs[1].p.h = (AIReal)px; segs[1].p.v = (AIReal)py;
            segs[1].in = segs[1].out = segs[1].p; segs[1].corner = true;

            segs[2].p.h = (AIReal)(px + patchSizePt); segs[2].p.v = (AIReal)py;
            segs[2].in = segs[2].out = segs[2].p; segs[2].corner = true;

            segs[3].p.h = (AIReal)(px + patchSizePt); segs[3].p.v = (AIReal)(py - patchSizePt);
            segs[3].in = segs[3].out = segs[3].p; segs[3].corner = true;

            sAIPath->SetPathSegments(pathArt, 0, 4, segs);
            sAIPath->SetPathClosed(pathArt, true);

            AIPathStyle style;
            AIBoolean hasStyle = false;
            sAIPathStyle->GetPathStyle(pathArt, &style, &hasStyle);

            style.fillPaint = true;
            style.strokePaint = false;
            style.fill.color.kind = kFourColor;
            style.fill.color.c.f.cyan = patches[p].c;
            style.fill.color.c.f.magenta = patches[p].m;
            style.fill.color.c.f.yellow = patches[p].y;
            style.fill.color.c.f.black = patches[p].k;

            sAIPathStyle->SetPathStyle(pathArt, &style);
            createdCount++;

            currentOffset += patchSizePt;
        }

        if (reachedLimit) {
            break;
        }

        // 4 mm gap between consecutive color bar strips
        currentOffset += gapPt;
    }

    return createdCount;
}

template <typename Func>
static void GenerateMarksForRect(
    const AIRealRect& rc,
    const CropMarkParams& params,
    bool canDoTop,
    bool canDoBottom,
    bool canDoLeft,
    bool canDoRight,
    double lVer, double lHor,
    double oVer, double oHor,
    double bVer, double bHor,
    Func addMark
) {
    const double left = rc.left;
    const double top = rc.top;
    const double right = rc.right;
    const double bottom = rc.bottom;

    // TOP BLOCK (vertical lines)
    if (canDoTop) {
        if (params.dirs.topLeft) {
            double x = left + bHor;
            double y1 = top + oVer;
            double y2 = y1 + lVer;
            addMark(x, y1, x, y2);
        }
        if (params.dirs.topCenter) {
            double x = (left + right) * 0.5;
            double y1 = top + oVer;
            double y2 = y1 + lVer;
            addMark(x, y1, x, y2);
        }
        if (params.dirs.topRight) {
            double x = right - bHor;
            double y1 = top + oVer;
            double y2 = y1 + lVer;
            addMark(x, y1, x, y2);
        }
    }

    // BOTTOM BLOCK (vertical lines)
    if (canDoBottom) {
        if (params.dirs.bottomLeft) {
            double x = left + bHor;
            double y1 = bottom - oVer;
            double y2 = y1 - lVer;
            addMark(x, y1, x, y2);
        }
        if (params.dirs.bottomCenter) {
            double x = (left + right) * 0.5;
            double y1 = bottom - oVer;
            double y2 = y1 - lVer;
            addMark(x, y1, x, y2);
        }
        if (params.dirs.bottomRight) {
            double x = right - bHor;
            double y1 = bottom - oVer;
            double y2 = y1 - lVer;
            addMark(x, y1, x, y2);
        }
    }

    // LEFT BLOCK (horizontal lines)
    if (canDoLeft) {
        if (params.dirs.leftTop) {
            double y = top - bVer;
            double x1 = left - oHor;
            double x2 = x1 - lHor;
            addMark(x1, y, x2, y);
        }
        if (params.dirs.leftCenter) {
            double y = (top + bottom) * 0.5;
            double x1 = left - oHor;
            double x2 = x1 - lHor;
            addMark(x1, y, x2, y);
        }
        if (params.dirs.leftBottom) {
            double y = bottom + bVer;
            double x1 = left - oHor;
            double x2 = x1 - lHor;
            addMark(x1, y, x2, y);
        }
    }

    // RIGHT BLOCK (horizontal lines)
    if (canDoRight) {
        if (params.dirs.rightTop) {
            double y = top - bVer;
            double x1 = right + oHor;
            double x2 = x1 + lHor;
            addMark(x1, y, x2, y);
        }
        if (params.dirs.rightCenter) {
            double y = (top + bottom) * 0.5;
            double x1 = right + oHor;
            double x2 = x1 + lHor;
            addMark(x1, y, x2, y);
        }
        if (params.dirs.rightBottom) {
            double y = bottom + bVer;
            double x1 = right + oHor;
            double x2 = x1 + lHor;
            addMark(x1, y, x2, y);
        }
    }
}

struct TargetItem {
    AIArtHandle art = nullptr;
    AIRealRect bounds = { 0, 0, 0, 0 };
};

struct Cluster {
    std::vector<TargetItem> items;
    AIRealRect unionRc;
};

static double RectDistance(const AIRealRect& a, const AIRealRect& b) {
    double dx = 0.0;
    if (a.right < b.left) dx = b.left - a.right;
    else if (b.right < a.left) dx = a.left - b.right;

    double dy = 0.0;
    // In Illustrator: top > bottom
    if (a.bottom > b.top) dy = a.bottom - b.top;
    else if (b.bottom > a.top) dy = b.bottom - a.top;

    return std::sqrt(dx * dx + dy * dy);
}

static std::vector<Cluster> BuildClusters(const std::vector<TargetItem>& allItems, double gapThreshold = 50.0) {
    std::vector<Cluster> clusters;
    if (allItems.empty()) return clusters;

    size_t n = allItems.size();
    std::vector<bool> visited(n, false);

    for (size_t i = 0; i < n; ++i) {
        if (visited[i]) continue;

        Cluster c;
        std::vector<size_t> queue;
        queue.push_back(i);
        visited[i] = true;

        while (!queue.empty()) {
            size_t curr = queue.back();
            queue.pop_back();

            c.items.push_back(allItems[curr]);

            for (size_t j = 0; j < n; ++j) {
                if (!visited[j]) {
                    double dist = RectDistance(allItems[curr].bounds, allItems[j].bounds);
                    if (dist <= gapThreshold) {
                        visited[j] = true;
                        queue.push_back(j);
                    }
                }
            }
        }

        // Calculate unionRc for cluster
        c.unionRc = c.items[0].bounds;
        for (size_t k = 1; k < c.items.size(); ++k) {
            if (c.items[k].bounds.left < c.unionRc.left) c.unionRc.left = c.items[k].bounds.left;
            if (c.items[k].bounds.top > c.unionRc.top) c.unionRc.top = c.items[k].bounds.top;
            if (c.items[k].bounds.right > c.unionRc.right) c.unionRc.right = c.items[k].bounds.right;
            if (c.items[k].bounds.bottom < c.unionRc.bottom) c.unionRc.bottom = c.items[k].bounds.bottom;
        }

        clusters.push_back(c);
    }

    return clusters;
}

CropMarkResult CropMarkEngine::CreateCropMarks(const CropMarkParams& params) {
    auto startTime = std::chrono::high_resolution_clock::now();
    CropMarkResult res;

    if (!sAIArt || !sAIDocument || !sAIPath) {
        res.message = "Illustrator SDK suites not initialized.";
        return res;
    }

    // Verify open document
    AIDocumentHandle doc = nullptr;
    if (sAIDocument->GetDocument(&doc) != kNoErr || !doc) {
        res.message = "No active document found.";
        return res;
    }

    // Geometry values in points
    const double lVer = CropMarkParams::UnitToPoints(params.lengthVer, params.units);
    const double lHor = CropMarkParams::UnitToPoints(params.lengthHor, params.units);
    const double oVer = CropMarkParams::UnitToPoints(params.offsetVer, params.units);
    const double oHor = CropMarkParams::UnitToPoints(params.offsetHor, params.units);
    const double bVer = CropMarkParams::UnitToPoints(params.bleedVer, params.units);
    const double bHor = CropMarkParams::UnitToPoints(params.bleedHor, params.units);
    const double strokeWidthPt = CropMarkParams::UnitToPoints(params.strokeWidth, params.strokeUnits);

    std::vector<TargetItem> targetItems;

    if (params.targetMode == kTargetArtboard) {
        if (!sAIArtboard) {
            res.message = "Artboard suite not available.";
            return res;
        }
        ai::ArtboardList abList;
        if (sAIArtboard->GetArtboardList(abList) == kNoErr) {
            ai::ArtboardID activeIndex = 0;
            if (sAIArtboard->GetActive(abList, activeIndex) == kNoErr) {
                ai::ArtboardProperties props;
                sAIArtboard->Init(props);
                if (sAIArtboard->GetArtboardProperties(abList, activeIndex, props) == kNoErr) {
                    AIRealRect abBounds;
                    if (sAIArtboard->GetPosition(props, abBounds) == kNoErr) {
                        if (abBounds.left > abBounds.right) std::swap(abBounds.left, abBounds.right);
                        if (abBounds.bottom > abBounds.top) std::swap(abBounds.bottom, abBounds.top);
                        targetItems.push_back({ nullptr, abBounds });
                    }
                }
                sAIArtboard->Dispose(props);
            }
            sAIArtboard->ReleaseArtboardList(abList);
        }
    } else {
        // Selected art mode
        if (!sAIMatchingArt) {
            res.message = "MatchingArt suite not available.";
            return res;
        }

        AIArtHandle** matches = nullptr;
        ai::int32 artCount = 0;
        AIErr err = sAIMatchingArt->GetSelectedArt(&matches, &artCount);

        if (err != kNoErr || !matches || !(*matches) || artCount <= 0) {
            res.message = "Please select at least one object in the artwork.";
            return res;
        }

        std::vector<AIArtHandle> rawList;
        rawList.reserve(artCount);
        for (ai::int32 i = 0; i < artCount; ++i) {
            AIArtHandle a = (*matches)[i];
            if (!a || IsGuide(a) || IsCropMarksGroup(a)) continue;
            rawList.push_back(a);
        }

        if (matches && sAIMdMemory) {
            sAIMdMemory->MdMemoryDisposeHandle((AIMdMemoryHandle)matches);
            matches = nullptr;
        }

        // Filter out layer groups and nested children of selected parents
        std::vector<AIArtHandle> selectedArts;
        for (AIArtHandle a : rawList) {
            if (IsLayerArt(a)) continue;
            if (HasSelectedAncestor(a, rawList)) continue;
            selectedArts.push_back(a);
        }

        if (selectedArts.empty()) {
            res.message = "No valid objects found in selection (guides and crop marks ignored).";
            return res;
        }

        // Clean any existing embedded crop marks inside selected groups before computing bounds
        for (AIArtHandle a : selectedArts) {
            RemoveExistingCropMarks(a);
            AIRealRect b = GetObjectBounds(a, params.boundsMode);
            if (std::abs(b.right - b.left) > 0.001 && std::abs(b.top - b.bottom) > 0.001) {
                targetItems.push_back({ a, b });
            }
        }
    }

    if (targetItems.empty()) {
        res.message = "No target bounds calculated.";
        return res;
    }

    // Undo registration
    if (sAIUndo) {
        sAIUndo->SetUndoTextUS(ai::UnicodeString("Undo Make Crop Marks"), ai::UnicodeString("Redo Make Crop Marks"));
    }

    int totalMarksCreated = 0;

    // Prepress Registration Targets and CMYK Color Bars helpers
    auto addColorBars = [&](AIArtHandle grp, const AIRealRect& box) {
        if (!params.colorBarsLong && !params.colorBarsShort) return;

        const double w = box.right - box.left;
        const double h = box.top - box.bottom;
        const double ptPerMm = 2.83464567;
        const double patchSize = 4.0 * ptPerMm;
        const double barsOff = CropMarkParams::UnitToPoints(params.barsOffset, params.units);

        // Requirement 3:
        // Horizontal: start 3 mm to the right of leftmost crop mark, stop 3 mm before rightmost crop mark
        // Vertical: start 3 mm down from topmost crop mark, stop 3 mm before bottommost crop mark
        const double horizStartX = (box.left + bHor) + 3.0 * ptPerMm;
        const double horizLimitX = (box.right - bHor) - 3.0 * ptPerMm;

        const double vertStartY = (box.top - bVer) - 3.0 * ptPerMm;
        const double vertLimitY = (box.bottom + bVer) + 3.0 * ptPerMm;

        const bool isLandscape = (w >= h);

        if (params.colorBarsLong) {
            if (isLandscape) {
                // Long side in landscape: Top horizontal bar
                double barStartY = box.top + oVer + barsOff + patchSize;
                totalMarksCreated += CreateColorBarStrip(
                    grp, horizStartX, barStartY, true, patchSize, horizLimitX,
                    params.barStandard, params.barLength
                );
            } else {
                // Long side in portrait: Left vertical bar
                double barStartX = box.left - oHor - barsOff - patchSize;
                totalMarksCreated += CreateColorBarStrip(
                    grp, barStartX, vertStartY, false, patchSize, vertLimitY,
                    params.barStandard, params.barLength
                );
            }
        }

        if (params.colorBarsShort) {
            if (isLandscape) {
                // Short sides in landscape: ONLY Left vertical bar (Right side removed as requested)
                double leftStartX = box.left - oHor - barsOff - patchSize;
                totalMarksCreated += CreateColorBarStrip(
                    grp, leftStartX, vertStartY, false, patchSize, vertLimitY,
                    params.barStandard, params.barLength
                );
            } else {
                // Short sides in portrait: Top and Bottom horizontal bars
                // Top side
                double topStartY = box.top + oVer + barsOff + patchSize;
                totalMarksCreated += CreateColorBarStrip(
                    grp, horizStartX, topStartY, true, patchSize, horizLimitX,
                    params.barStandard, params.barLength
                );

                // Bottom side
                double botStartY = box.bottom - oVer - barsOff;
                totalMarksCreated += CreateColorBarStrip(
                    grp, horizStartX, botStartY, true, patchSize, horizLimitX,
                    params.barStandard, params.barLength
                );
            }
        }
    };

    auto addDocInfo = [&](AIArtHandle grp, const AIRealRect& box) {
        if (!params.docInfo) return;

        const double ptPerMm = 2.83464567;
        const double patchSize = 4.0 * ptPerMm;
        const double barsOff = CropMarkParams::UnitToPoints(params.barsOffset, params.units);
        const double horizStartX = (box.left + bHor) + 3.0 * ptPerMm;
        const double topBarStartY = box.top + oVer + barsOff + patchSize;
        const double textAnchorY = topBarStartY + 1.0 * ptPerMm;
        const double textAnchorX = horizStartX;

        std::string docName = "Untitled";
        if (sAIDocument) {
            ai::UnicodeString aiDocName;
            if (sAIDocument->GetDocumentFileName(aiDocName) == kNoErr && !aiDocName.empty()) {
                std::string full = aiDocName.as_UTF8();
                size_t lastSlash = full.find_last_of("/\\");
                if (lastSlash != std::string::npos) {
                    docName = full.substr(lastSlash + 1);
                } else {
                    docName = full;
                }
            }
        }

        time_t now = time(nullptr);
        struct tm t;
        localtime_s(&t, &now);
        char timeBuf[64];
        strftime(timeBuf, sizeof(timeBuf), "%d.%m.%Y  %H:%M", &t);
        std::string infoText = docName + "  " + timeBuf;

        PluginLog("[addDocInfo] Start: textAnchorX=%.2f, textAnchorY=%.2f, infoText='%s', sAITextFrame=%p",
                  textAnchorX, textAnchorY, infoText.c_str(), sAITextFrame);

        if (sAITextFrame) {
            AIArtHandle textArt = nullptr;
            AIRealPoint anchor = { (AIReal)textAnchorX, (AIReal)textAnchorY };
            AIErr err = sAITextFrame->NewPointText(kPlaceInsideOnTop, grp, kHorizontalTextOrientation, anchor, &textArt);
            PluginLog("[addDocInfo] NewPointText err=%d, textArt=%p", err, textArt);
            if (err == kNoErr && textArt) {
                TextRangeRef textRange = nullptr;
                if (sAITextFrame->GetATETextRange(textArt, &textRange) == kNoErr && textRange) {
                    ai::UnicodeString uText(infoText);
                    if (sTextRange) {
                        sTextRange->InsertAfter_AsUnicode(textRange, uText.as_ASUnicode().c_str(), -1);
                        sTextRange->Release(textRange);
                        textRange = nullptr;
                    }
                }
                // Acquire fresh text range spanning the newly inserted characters
                if (sAITextFrame->GetATETextRange(textArt, &textRange) == kNoErr && textRange) {
                    if (sCharFeatures) {
                        ATE::CharFeaturesRef charFeatures = nullptr;
                        if (sCharFeatures->Initialize(&charFeatures) == 0 && charFeatures) {
                            sCharFeatures->SetFontSize(charFeatures, 5.0f);
                            if (sAICustomColor && sAIATEPaint) {
                                AICustomColorHandle hReg = nullptr;
                                if (sAICustomColor->GetCurrentRegistrationColor(&hReg) == kNoErr && hReg) {
                                    AIColor regColor;
                                    regColor.kind = kCustomColor;
                                    regColor.c.c.color = hReg;
                                    regColor.c.c.tint = 0.0f;
                                    ATE::ApplicationPaintRef paintRef = nullptr;
                                    if (sAIATEPaint->CreateATEApplicationPaint(&regColor, &paintRef) == kNoErr && paintRef) {
                                        sCharFeatures->SetFillColor(charFeatures, paintRef);
                                        if (sApplicationPaint) sApplicationPaint->Release(paintRef);
                                    }
                                }
                            }
                            if (sTextRange) sTextRange->SetLocalCharFeatures(textRange, charFeatures);
                            sCharFeatures->Release(charFeatures);
                        }
                    }
                    if (sTextRange) sTextRange->Release(textRange);
                }
                sAIArt->SetArtName(textArt, ai::UnicodeString("Document Info"));
                sAIArt->SetArtUserAttr(textArt, kArtSelected, 0);
                totalMarksCreated++;
                PluginLog("[addDocInfo] Success: Document Info created and styled with 5pt Registration color.");
            }
        }
    };

    auto addRegistrationTargets = [&](AIArtHandle grp, const AIRealRect& box) {
        if (!params.regMarks) return;

        const double ptPerMm = 2.83464567;
        const double patchSize = 4.0 * ptPerMm;
        const double crossOff = CropMarkParams::UnitToPoints(params.crossOffset, params.units);

        // Top Center
        double topCx = (box.left + box.right) * 0.5;
        double topCy = box.top + oVer + crossOff + 0.5 * patchSize;
        CreateRegistrationTarget(grp, topCx, topCy, true, strokeWidthPt);

        // Bottom Center
        double botCx = (box.left + box.right) * 0.5;
        double botCy = box.bottom - oVer - crossOff - 0.5 * patchSize;
        CreateRegistrationTarget(grp, botCx, botCy, true, strokeWidthPt);

        // Left Center
        double leftCx = box.left - oHor - crossOff - 0.5 * patchSize;
        double leftCy = (box.top + box.bottom) * 0.5;
        CreateRegistrationTarget(grp, leftCx, leftCy, true, strokeWidthPt);

        // Right Center
        double rightCx = box.right + oHor + crossOff + 0.5 * patchSize;
        double rightCy = (box.top + box.bottom) * 0.5;
        CreateRegistrationTarget(grp, rightCx, rightCy, true, strokeWidthPt);

        totalMarksCreated += 4;
    };

    if (params.targetMode == kTargetArtboard) {
        // Artboard target: create marks in a top-level group
        AIArtHandle marksGroup = nullptr;
        sAIArt->NewArt(kGroupArt, kPlaceAboveAll, nullptr, &marksGroup);
        if (!marksGroup) {
            res.message = "Failed to create crop marks container group.";
            return res;
        }
        sAIArt->SetArtName(marksGroup, ai::UnicodeString("Crop Marks"));

        std::vector<LineDedupeEntry> dedupeList;
        std::vector<AIArtHandle> groupMarks;

        auto addMark = [&](double x1, double y1, double x2, double y2) {
            if (params.whiteContour) {
                LineDedupeEntry whiteEntry = { x1, y1, x2, y2, false };
                bool exists = false;
                for (const auto& d : dedupeList) {
                    if (AreLinesEqual(d, whiteEntry)) { exists = true; break; }
                }
                if (!exists) {
                    AIArtHandle wArt = CreateLineArt(marksGroup, x1, y1, x2, y2, false, strokeWidthPt + 1.0);
                    if (wArt) {
                        dedupeList.push_back(whiteEntry);
                        groupMarks.push_back(wArt);
                    }
                }
            }

            LineDedupeEntry regEntry = { x1, y1, x2, y2, true };
            bool exists = false;
            for (const auto& d : dedupeList) {
                if (AreLinesEqual(d, regEntry)) { exists = true; break; }
            }
            if (!exists) {
                AIArtHandle rArt = CreateLineArt(marksGroup, x1, y1, x2, y2, true, strokeWidthPt);
                if (rArt) {
                    dedupeList.push_back(regEntry);
                    groupMarks.push_back(rArt);
                }
            }
        };

        auto generateArtboardMarks = [&]() {
            GenerateMarksForRect(targetItems[0].bounds, params, true, true, true, true,
                                 lVer, lHor, oVer, oHor, bVer, bHor, addMark);
        };

        if (params.crossMarksLayerOrder == kCrossMarksBelowPatches) {
            addRegistrationTargets(marksGroup, targetItems[0].bounds);
        }
        if (params.cropMarksLayerOrder == kCropMarksBelowPatches) {
            generateArtboardMarks();
        }

        addColorBars(marksGroup, targetItems[0].bounds);
        addDocInfo(marksGroup, targetItems[0].bounds);

        if (params.cropMarksLayerOrder == kCropMarksAbovePatches) {
            generateArtboardMarks();
        }
        if (params.crossMarksLayerOrder == kCrossMarksAbovePatches) {
            addRegistrationTargets(marksGroup, targetItems[0].bounds);
        }

        sAIArt->SetArtUserAttr(marksGroup, kArtSelected, 0);
        for (AIArtHandle m : groupMarks) {
            sAIArt->SetArtUserAttr(m, kArtSelected, 0);
        }
        totalMarksCreated = (int)groupMarks.size();

    } else {
        // Selected items: split into spatial clusters (e.g. 1 cluster per artboard or card block)
        std::vector<Cluster> clusters = BuildClusters(targetItems, 50.0);

        for (const auto& cluster : clusters) {
            if (cluster.items.empty()) continue;

            AIArtHandle anchorArt = cluster.items[0].art;
            AIArtHandle marksGroup = nullptr;
            AIErr gErr = sAIArt->NewArt(kGroupArt, kPlaceBelow, anchorArt, &marksGroup);
            if (gErr != kNoErr || !marksGroup) continue;
            sAIArt->SetArtName(marksGroup, ai::UnicodeString("Crop Marks"));

            std::vector<LineDedupeEntry> dedupeList;
            std::vector<AIArtHandle> clusterMarks;

            auto addMark = [&](double x1, double y1, double x2, double y2) {
                if (params.whiteContour) {
                    LineDedupeEntry whiteEntry = { x1, y1, x2, y2, false };
                    bool exists = false;
                    for (const auto& d : dedupeList) {
                        if (AreLinesEqual(d, whiteEntry)) { exists = true; break; }
                    }
                    if (!exists) {
                        AIArtHandle wArt = CreateLineArt(marksGroup, x1, y1, x2, y2, false, strokeWidthPt + 1.0);
                        if (wArt) {
                            dedupeList.push_back(whiteEntry);
                            clusterMarks.push_back(wArt);
                        }
                    }
                }

                LineDedupeEntry regEntry = { x1, y1, x2, y2, true };
                bool exists = false;
                for (const auto& d : dedupeList) {
                    if (AreLinesEqual(d, regEntry)) { exists = true; break; }
                }
                if (!exists) {
                    AIArtHandle rArt = CreateLineArt(marksGroup, x1, y1, x2, y2, true, strokeWidthPt);
                    if (rArt) {
                        dedupeList.push_back(regEntry);
                        clusterMarks.push_back(rArt);
                    }
                }
            };

            auto generateClusterCropMarks = [&]() {
                if (params.targetMode == kTargetEntireSelection) {
                    GenerateMarksForRect(cluster.unionRc, params, true, true, true, true,
                                         lVer, lHor, oVer, oHor, bVer, bHor, addMark);
                } else {
                    for (size_t i = 0; i < cluster.items.size(); ++i) {
                        const auto& item = cluster.items[i];
                        const AIRealRect& rc = item.bounds;

                        auto candidateAddMark = [&](double x1, double y1, double x2, double y2) {
                            if (params.perimeterOnly && cluster.items.size() > 1) {
                                const double mLeft = (std::min)(x1, x2);
                                const double mRight = (std::max)(x1, x2);
                                const double mBottom = (std::min)(y1, y2);
                                const double mTop = (std::max)(y1, y2);

                                const double tol = 0.02;

                                for (size_t j = 0; j < cluster.items.size(); ++j) {
                                    if (i == j) continue;
                                    const AIRealRect& other = cluster.items[j].bounds;

                                    const double bLeft = other.left - oHor + tol;
                                    const double bRight = other.right + oHor - tol;
                                    const double bBottom = other.bottom - oVer + tol;
                                    const double bTop = other.top + oVer - tol;

                                    const bool overlapX = ((std::max)(mLeft, bLeft) <= (std::min)(mRight, bRight));
                                    const bool overlapY = ((std::max)(mBottom, bBottom) <= (std::min)(mTop, bTop));

                                    if (overlapX && overlapY) {
                                        return;
                                    }
                                }
                            }

                            addMark(x1, y1, x2, y2);
                        };

                        GenerateMarksForRect(rc, params, true, true, true, true,
                                             lVer, lHor, oVer, oHor, bVer, bHor, candidateAddMark);
                    }
                }
            };

            if (params.crossMarksLayerOrder == kCrossMarksBelowPatches) {
                addRegistrationTargets(marksGroup, cluster.unionRc);
            }
            if (params.cropMarksLayerOrder == kCropMarksBelowPatches) {
                generateClusterCropMarks();
            }

            addColorBars(marksGroup, cluster.unionRc);
            addDocInfo(marksGroup, cluster.unionRc);

            if (params.cropMarksLayerOrder == kCropMarksAbovePatches) {
                generateClusterCropMarks();
            }
            if (params.crossMarksLayerOrder == kCrossMarksAbovePatches) {
                addRegistrationTargets(marksGroup, cluster.unionRc);
            }

            if (clusterMarks.empty() && !params.regMarks && !params.colorBarsLong && !params.colorBarsShort && !params.docInfo) {
                sAIArt->DisposeArt(marksGroup);
                continue;
            }

            totalMarksCreated += (int)clusterMarks.size();

            sAIArt->SetArtUserAttr(marksGroup, kArtSelected, 0);
            for (AIArtHandle m : clusterMarks) {
                sAIArt->SetArtUserAttr(m, kArtSelected, 0);
            }

            // CRITICAL: Ensure ALL artwork in cluster is strictly ABOVE marksGroup!
            // Reordering every art above marksGroup guarantees marks are NEVER on top of artwork!
            for (const auto& ti : cluster.items) {
                if (ti.art) {
                    sAIArt->ReorderArt(ti.art, kPlaceAbove, marksGroup);
                }
            }

            // Apply Grouping Mode
            if (params.groupMode == kGroupMarksAndSelection) {
                AIArtHandle clusterGroup = nullptr;
                sAIArt->NewArt(kGroupArt, kPlaceBelow, marksGroup, &clusterGroup);
                if (clusterGroup) {
                    sAIArt->ReorderArt(marksGroup, kPlaceInsideOnBottom, clusterGroup);
                    for (const auto& ti : cluster.items) {
                        if (ti.art) {
                            sAIArt->ReorderArt(ti.art, kPlaceInsideOnTop, clusterGroup);
                        }
                    }
                    sAIArt->SetArtUserAttr(clusterGroup, kArtSelected, 0);
                }
            } else if (params.groupMode == kGroupNone) {
                for (AIArtHandle m : clusterMarks) {
                    sAIArt->ReorderArt(m, kPlaceBelow, marksGroup);
                }
                sAIArt->DisposeArt(marksGroup);
            } else {
                // kGroupMarksOnly: marksGroup already contains ALL marks for this cluster,
                // and all artwork is strictly above marksGroup!
            }
        }
    }

    if (totalMarksCreated == 0) {
        res.message = "No marks created (all directions unchecked or zero bounds).";
        return res;
    }

    // Invalidate and force document redraw
    if (sAIDocument) {
        sAIDocument->RedrawDocument();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    res.elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    res.success = true;
    res.marksCount = totalMarksCreated;
    res.objectsCount = (int)targetItems.size();

    char msgBuf[256];
    snprintf(msgBuf, sizeof(msgBuf), "Created %d crop marks for %d object(s) in %.2f ms",
             res.marksCount, res.objectsCount, res.elapsedMs);
    res.message = msgBuf;

    PluginLog("[CropMarkEngine] %s", res.message.c_str());
    return res;
}
