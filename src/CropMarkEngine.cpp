#include "CropMarkEngine.h"
#include <cmath>
#include <vector>
#include <chrono>

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

AIRealRect CropMarkEngine::GetObjectBounds(AIArtHandle art, int boundsMode) {
    AIRealRect rc = { 0, 0, 0, 0 };
    if (!sAIArt || !art) return rc;

    // Check if object is a clipping group
    AIArtHandle clipPath = nullptr;
    if (IsClippingMask(art, &clipPath) && clipPath) {
        art = clipPath;
    }

    AIErr err = kBadParameterErr;
    if (boundsMode == kBoundsVisible) {
        // Visible bounds (includes stroke)
        err = sAIArt->GetArtBounds(art, &rc);
    } else {
        // Geometric bounds (control points / bezier outlines without stroke)
        err = sAIArt->GetArtTransformBounds(art, nullptr, kNoStrokeBounds | kExcludeGuideBounds, &rc);
        if (err != kNoErr || (rc.left == 0 && rc.right == 0 && rc.top == 0 && rc.bottom == 0)) {
            err = sAIArt->GetArtBounds(art, &rc);
        }
    }

    if (rc.left > rc.right) std::swap(rc.left, rc.right);
    if (rc.bottom > rc.top) std::swap(rc.bottom, rc.top);

    PluginLog("[GetObjectBounds] art=%p mode=%d -> L=%.2f, T=%.2f, R=%.2f, B=%.2f (err=%d)",
              art, boundsMode, rc.left, rc.top, rc.right, rc.bottom, err);
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

    // Determine target bounding boxes
    std::vector<AIRealRect> targetRects;
    std::vector<AIArtHandle> selectedArts;
    AIArtHandle lowestTargetArt = nullptr;

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
                        targetRects.push_back(abBounds);
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
            if (!a || IsGuide(a)) continue;
            rawList.push_back(a);
        }

        if (matches && sAIMdMemory) {
            sAIMdMemory->MdMemoryDisposeHandle((AIMdMemoryHandle)matches);
            matches = nullptr;
        }

        // Filter out layer groups and nested children of selected parents
        for (AIArtHandle a : rawList) {
            if (IsLayerArt(a)) continue;
            if (HasSelectedAncestor(a, rawList)) continue;
            selectedArts.push_back(a);
        }

        if (selectedArts.empty()) {
            res.message = "No valid objects found in selection (guides ignored).";
            return res;
        }

        lowestTargetArt = selectedArts[0];

        if (params.targetMode == kTargetEntireSelection) {
            AIRealRect unionRc = GetObjectBounds(selectedArts[0], params.boundsMode);
            for (size_t i = 1; i < selectedArts.size(); ++i) {
                AIRealRect r = GetObjectBounds(selectedArts[i], params.boundsMode);
                if (r.left < unionRc.left) unionRc.left = r.left;
                if (r.right > unionRc.right) unionRc.right = r.right;
                if (r.top > unionRc.top) unionRc.top = r.top;
                if (r.bottom < unionRc.bottom) unionRc.bottom = r.bottom;
            }
            targetRects.push_back(unionRc);
        } else {
            // Each object in selection
            for (AIArtHandle a : selectedArts) {
                targetRects.push_back(GetObjectBounds(a, params.boundsMode));
            }
        }
    }

    if (targetRects.empty()) {
        res.message = "No target bounds calculated.";
        return res;
    }

    // Calculate union bounding box of all target rects for perimeter filtering
    AIRealRect unionRc = targetRects[0];
    for (size_t i = 1; i < targetRects.size(); ++i) {
        if (targetRects[i].left < unionRc.left) unionRc.left = targetRects[i].left;
        if (targetRects[i].top > unionRc.top) unionRc.top = targetRects[i].top;
        if (targetRects[i].right > unionRc.right) unionRc.right = targetRects[i].right;
        if (targetRects[i].bottom < unionRc.bottom) unionRc.bottom = targetRects[i].bottom;
    }

    // Find active layer container or use lowestTargetArt
    AILayerHandle layer = nullptr;
    if (sAIArt && lowestTargetArt) {
        sAIArt->GetLayerOfArt(lowestTargetArt, &layer);
    }
    AIArtHandle layerGroup = nullptr;
    if (sAIArt) {
        sAIArt->GetFirstArtOfLayer(layer, &layerGroup);
    }

    // Undo registration
    if (sAIUndo) {
        sAIUndo->SetUndoTextUS(ai::UnicodeString("Undo Make Crop Marks"), ai::UnicodeString("Redo Make Crop Marks"));
    }

    // Dedicated group for crop marks - create inside-on-bottom of layer group to be behind artwork
    AIArtHandle marksGroup = nullptr;
    if (layerGroup) {
        sAIArt->NewArt(kGroupArt, kPlaceInsideOnBottom, layerGroup, &marksGroup);
    } else if (lowestTargetArt) {
        sAIArt->NewArt(kGroupArt, kPlaceBelow, lowestTargetArt, &marksGroup);
    } else {
        sAIArt->NewArt(kGroupArt, kPlaceBelowAll, nullptr, &marksGroup);
    }

    if (!marksGroup) {
        res.message = "Failed to create crop marks container group.";
        return res;
    }
    sAIArt->SetArtName(marksGroup, ai::UnicodeString("Crop Marks"));

    // Geometry values in points
    const double lVer = CropMarkParams::UnitToPoints(params.lengthVer, params.units);
    const double lHor = CropMarkParams::UnitToPoints(params.lengthHor, params.units);
    const double oVer = CropMarkParams::UnitToPoints(params.offsetVer, params.units);
    const double oHor = CropMarkParams::UnitToPoints(params.offsetHor, params.units);
    const double bVer = CropMarkParams::UnitToPoints(params.bleedVer, params.units);
    const double bHor = CropMarkParams::UnitToPoints(params.bleedHor, params.units);
    const double strokeWidthPt = CropMarkParams::UnitToPoints(params.strokeWidth, params.strokeUnits);

    std::vector<LineDedupeEntry> dedupeList;
    std::vector<AIArtHandle> allMarks;

    auto addMark = [&](double x1, double y1, double x2, double y2) {
        // 1. White underlay
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
                    allMarks.push_back(wArt);
                }
            }
        }

        // 2. Registration stroke
        LineDedupeEntry regEntry = { x1, y1, x2, y2, true };
        bool exists = false;
        for (const auto& d : dedupeList) {
            if (AreLinesEqual(d, regEntry)) { exists = true; break; }
        }
        if (!exists) {
            AIArtHandle rArt = CreateLineArt(marksGroup, x1, y1, x2, y2, true, strokeWidthPt);
            if (rArt) {
                dedupeList.push_back(regEntry);
                allMarks.push_back(rArt);
            }
        }
    };

    // Build crop marks for each rectangle
    for (const auto& rc : targetRects) {
        const double left = rc.left;
        const double top = rc.top;
        const double right = rc.right;
        const double bottom = rc.bottom;

        // Perimeter filtering: do not make marks on inner edges of the selection block
        bool canDoTop = !params.perimeterOnly || (std::abs(top - unionRc.top) <= 0.2);
        bool canDoBottom = !params.perimeterOnly || (std::abs(bottom - unionRc.bottom) <= 0.2);
        bool canDoLeft = !params.perimeterOnly || (std::abs(left - unionRc.left) <= 0.2);
        bool canDoRight = !params.perimeterOnly || (std::abs(right - unionRc.right) <= 0.2);

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

    if (allMarks.empty()) {
        sAIArt->DisposeArt(marksGroup);
        res.message = "No marks created (all directions unchecked or zero bounds).";
        return res;
    }

    // Apply Grouping Mode & Send To Back
    if (params.groupMode == kGroupMarksAndSelection && !selectedArts.empty()) {
        // Group marks and selected artwork together into one top-level group
        AIArtHandle parentGroup = nullptr;
        sAIArt->NewArt(kGroupArt, kPlaceBelow, lowestTargetArt, &parentGroup);
        if (parentGroup) {
            sAIArt->ReorderArt(marksGroup, kPlaceInsideOnBottom, parentGroup);
            for (AIArtHandle a : selectedArts) {
                sAIArt->ReorderArt(a, kPlaceInsideOnTop, parentGroup);
            }
        }
    } else if (params.groupMode == kGroupNone) {
        // Ungroup marks out of marksGroup and move them behind artwork
        for (AIArtHandle m : allMarks) {
            if (layerGroup) {
                sAIArt->ReorderArt(m, kPlaceInsideOnBottom, layerGroup);
            } else if (lowestTargetArt) {
                sAIArt->ReorderArt(m, kPlaceBelow, lowestTargetArt);
            }
        }
        sAIArt->DisposeArt(marksGroup);
    } else {
        // kGroupMarksOnly: ensure marksGroup is placed below all artwork
        if (layerGroup) {
            sAIArt->ReorderArt(marksGroup, kPlaceInsideOnBottom, layerGroup);
        } else if (lowestTargetArt) {
            sAIArt->ReorderArt(marksGroup, kPlaceBelow, lowestTargetArt);
        }
    }

    // Invalidate and force document redraw
    if (sAIDocument) {
        sAIDocument->RedrawDocument();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    res.elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    res.success = true;
    res.marksCount = (int)allMarks.size();
    res.objectsCount = (int)targetRects.size();

    char msgBuf[256];
    snprintf(msgBuf, sizeof(msgBuf), "Created %d crop marks for %d object(s) in %.2f ms",
             res.marksCount, res.objectsCount, res.elapsedMs);
    res.message = msgBuf;

    PluginLog("[CropMarkEngine] %s", res.message.c_str());
    return res;
}
