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

        GenerateMarksForRect(targetItems[0].bounds, params, true, true, true, true,
                             lVer, lHor, oVer, oHor, bVer, bHor, addMark);

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

            if (params.targetMode == kTargetEntireSelection) {
                // Draw around the entire cluster bounds
                GenerateMarksForRect(cluster.unionRc, params, true, true, true, true,
                                     lVer, lHor, oVer, oHor, bVer, bHor, addMark);
            } else {
                // kTargetEachObject: draw marks for items in this cluster
                for (size_t i = 0; i < cluster.items.size(); ++i) {
                    const auto& item = cluster.items[i];
                    const AIRealRect& rc = item.bounds;

                    bool canDoTop = true;
                    bool canDoBottom = true;
                    bool canDoLeft = true;
                    bool canDoRight = true;

                    if (params.perimeterOnly && cluster.items.size() > 1) {
                        const double tol = 2.0; // 2 pt tolerance for touching edges
                        for (size_t j = 0; j < cluster.items.size(); ++j) {
                            if (i == j) continue;
                            const AIRealRect& other = cluster.items[j].bounds;

                            // Touching right
                            if (std::abs(other.left - rc.right) <= tol) {
                                double overlapB = (std::max)(rc.bottom, other.bottom);
                                double overlapT = (std::min)(rc.top, other.top);
                                if (overlapT - overlapB > tol) canDoRight = false;
                            }

                            // Touching left
                            if (std::abs(other.right - rc.left) <= tol) {
                                double overlapB = (std::max)(rc.bottom, other.bottom);
                                double overlapT = (std::min)(rc.top, other.top);
                                if (overlapT - overlapB > tol) canDoLeft = false;
                            }

                            // Touching top
                            if (std::abs(other.bottom - rc.top) <= tol) {
                                double overlapL = (std::max)(rc.left, other.left);
                                double overlapR = (std::min)(rc.right, other.right);
                                if (overlapR - overlapL > tol) canDoTop = false;
                            }

                            // Touching bottom
                            if (std::abs(other.top - rc.bottom) <= tol) {
                                double overlapL = (std::max)(rc.left, other.left);
                                double overlapR = (std::min)(rc.right, other.right);
                                if (overlapR - overlapL > tol) canDoBottom = false;
                            }
                        }
                    }

                    GenerateMarksForRect(rc, params, canDoTop, canDoBottom, canDoLeft, canDoRight,
                                         lVer, lHor, oVer, oHor, bVer, bHor, addMark);
                }
            }

            if (clusterMarks.empty()) {
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
