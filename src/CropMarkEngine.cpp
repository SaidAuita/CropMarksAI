#include "CropMarkEngine.h"
#include "ConfigManager.h"
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

    AIBoolean isClipped = false;
    if (sAIGroup) {
        sAIGroup->GetGroupClipped(art, &isClipped);
    }
    ai::int32 attr = 0;
    sAIArt->GetArtUserAttr(art, kArtIsClipMask, &attr);
    bool groupMarkedClip = (isClipped != false) || ((attr & kArtIsClipMask) != 0);

    // Iterate children to find the clipping path (even if group attribute isn't reported consistently)
    AIArtHandle child = nullptr;
    sAIArt->GetArtFirstChild(art, &child);
    while (child) {
        ai::int32 childAttr = 0;
        sAIArt->GetArtUserAttr(child, kArtIsClipMask, &childAttr);
        bool childIsClip = (childAttr & kArtIsClipMask) != 0;
        if (!childIsClip && groupMarkedClip && sAIArt->IsArtClipping) {
            childIsClip = (sAIArt->IsArtClipping(child) != false);
        }
        if (childIsClip) {
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
    int depth = 100;
    while (--depth > 0 && sAIArt->GetArtParent(curr, &parent) == kNoErr && parent && parent != curr) {
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

static bool IsInsideCropMarksGroup(AIArtHandle art) {
    if (!art || !sAIArt) return false;
    AIArtHandle curr = art;
    AIArtHandle parent = nullptr;
    int depth = 50;
    while (--depth > 0 && sAIArt->GetArtParent(curr, &parent) == kNoErr && parent && parent != curr) {
        if (CropMarkEngine::IsLayerArt(parent)) break;
        if (CropMarkEngine::IsCropMarksGroup(parent)) return true;
        curr = parent;
    }
    return false;
}

static AIArtHandle GetSafePlacementAnchor(AIArtHandle art) {
    if (!art || !sAIArt) return nullptr;
    AIArtHandle curr = art;
    AIArtHandle parent = nullptr;
    int depth = 50;
    while (--depth > 0 && sAIArt->GetArtParent(curr, &parent) == kNoErr && parent && parent != curr) {
        if (CropMarkEngine::IsLayerArt(parent)) {
            return curr;
        }
        short pType = 0;
        sAIArt->GetArtType(parent, &pType);
        if (pType == kCompoundPathArt) {
            curr = parent;
            continue;
        }
        curr = parent;
    }
    return curr;
}

void CropMarkEngine::RemoveExistingCropMarks(AIArtHandle art) {
    if (!art || !sAIArt) return;
    short type = 0;
    if (sAIArt->GetArtType(art, &type) != kNoErr) return;

    if (type == kGroupArt) {
        AIArtHandle child = nullptr;
        sAIArt->GetArtFirstChild(art, &child);
        int safetyLimit = 5000;
        while (child && --safetyLimit > 0) {
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
}

static AIRealRect GetObjectBoundsInternal(AIArtHandle art, int boundsMode, int depth) {
    AIRealRect rc = { 0, 0, 0, 0 };
    if (!sAIArt || !art || depth > 64) return rc;

    short type = 0;
    sAIArt->GetArtType(art, &type);
    ai::int32 selfAttr = 0;
    sAIArt->GetArtUserAttr(art, kArtIsClipMask | kArtHidden, &selfAttr);

    if (type == kGroupArt) {
        AIBoolean isClipped = false;
        if (sAIGroup) {
            sAIGroup->GetGroupClipped(art, &isClipped);
        }
        bool groupMarkedClip = (isClipped != false) || ((selfAttr & kArtIsClipMask) != 0);

        if (depth <= 3) {
            PluginLog("  [BoundsTree depth=%d] group=%p selfAttr=0x%X isClipped=%d",
                      depth, art, selfAttr, (int)isClipped);
        }

        // 1. Collect clipping children first (even if groupMarkedClip wasn't reported by GetArtUserAttr)
        AIRealRect clipUnion = { 0, 0, 0, 0 };
        bool hasClip = false;
        AIArtHandle child = nullptr;
        sAIArt->GetArtFirstChild(art, &child);
        int safety = 5000;
        while (child && --safety > 0) {
            ai::int32 childAttr = 0;
            sAIArt->GetArtUserAttr(child, kArtIsClipMask | kArtHidden, &childAttr);
            AIBoolean childIsArtClip = (sAIArt->IsArtClipping) ? sAIArt->IsArtClipping(child) : false;
            bool isClipChild = ((childAttr & kArtIsClipMask) != 0) || (groupMarkedClip && childIsArtClip != false);

            if (depth <= 3) {
                short cType = 0;
                sAIArt->GetArtType(child, &cType);
                PluginLog("    [Child depth=%d] art=%p type=%d attr=0x%X isArtClip=%d",
                          depth + 1, child, (int)cType, childAttr, (int)childIsArtClip);
            }

            if (isClipChild) {
                AIRealRect cb = GetObjectBoundsInternal(child, boundsMode, depth + 1);
                if (std::abs(cb.right - cb.left) > 0.0001 || std::abs(cb.top - cb.bottom) > 0.0001) {
                    if (!hasClip) {
                        clipUnion = cb;
                        hasClip = true;
                    } else {
                        if (cb.left < clipUnion.left) clipUnion.left = cb.left;
                        if (cb.right > clipUnion.right) clipUnion.right = cb.right;
                        if (cb.bottom < clipUnion.bottom) clipUnion.bottom = cb.bottom;
                        if (cb.top > clipUnion.top) clipUnion.top = cb.top;
                    }
                }
            }
            sAIArt->GetArtSibling(child, &child);
        }
        if (hasClip) {
            if (depth <= 3) {
                PluginLog("  [BoundsTree depth=%d] -> CLIP UNION L=%.2f T=%.2f R=%.2f B=%.2f",
                          depth, clipUnion.left, clipUnion.top, clipUnion.right, clipUnion.bottom);
            }
            return clipUnion;
        }

        // 2. Regular group: recursively union visible non-guide children so nested clip groups
        //    do not leak bounds of clipped-out objects
        AIRealRect groupUnion = { 0, 0, 0, 0 };
        bool hasChildBounds = false;
        child = nullptr;
        sAIArt->GetArtFirstChild(art, &child);
        safety = 5000;
        while (child && --safety > 0) {
            ai::int32 childAttr = 0;
            sAIArt->GetArtUserAttr(child, kArtHidden, &childAttr);
            if ((childAttr & kArtHidden) == 0 && !CropMarkEngine::IsGuide(child) && !CropMarkEngine::IsCropMarksGroup(child)) {
                AIRealRect cb = GetObjectBoundsInternal(child, boundsMode, depth + 1);
                if (std::abs(cb.right - cb.left) > 0.0001 || std::abs(cb.top - cb.bottom) > 0.0001) {
                    if (!hasChildBounds) {
                        groupUnion = cb;
                        hasChildBounds = true;
                    } else {
                        if (cb.left < groupUnion.left) groupUnion.left = cb.left;
                        if (cb.right > groupUnion.right) groupUnion.right = cb.right;
                        if (cb.bottom < groupUnion.bottom) groupUnion.bottom = cb.bottom;
                        if (cb.top > groupUnion.top) groupUnion.top = cb.top;
                    }
                }
            }
            sAIArt->GetArtSibling(child, &child);
        }
        if (hasChildBounds) {
            return groupUnion;
        }
    }

    AIRealRect visBounds = { 0, 0, 0, 0 };
    sAIArt->GetArtBounds(art, &visBounds);
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
    return rc;
}

AIRealRect CropMarkEngine::GetObjectBounds(AIArtHandle art, int boundsMode) {
    AIRealRect rc = GetObjectBoundsInternal(art, boundsMode, 0);
    PluginLog("[GetObjectBounds] art=%p mode=%d -> L=%.2f, T=%.2f, R=%.2f, B=%.2f",
              art, boundsMode, rc.left, rc.top, rc.right, rc.bottom);
    return rc;
}

bool CropMarkEngine::IsTechnicalSpotName(const std::string& name) {
    if (name.empty()) return false;

    // Convert to lowercase using LCMapStringA (supports UTF-8 Cyrillic & Latin)
    std::string lower = name;
    std::vector<char> buf(name.length() * 2 + 16, 0);
    int len = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, name.c_str(), (int)name.length(), buf.data(), (int)buf.size() - 1);
    if (len > 0) {
        buf[len] = '\0';
        lower = std::string(buf.data());
    } else {
        for (char& ch : lower) ch = (char)tolower((unsigned char)ch);
    }

    // Technical keywords list (Prepress conventions: English & Russian)
    static const char* techKeywords[] = {
        // Cutting, Die, Creasing, Perforation
        "cut", "die", "dieline", "knife", "crease", "fold", "score", "perfo", "cutter",
        "штамп", "выруб", "нож", "биг", "перфо", "линия реза", "рез",
        // Varnishes & Coatings
        "varnish", "lack", "lak", "gloss", "matte", "mat", "uv", "лак", "уф", "глянц", "матовый",
        // Foils, Stamping & Embossing
        "foil", "stamp", "cliche", "тиснен", "клише", "фольг", "конгрев", "emboss", "deboss",
        // Whites & Opaque Underlay
        "white", "белил", "белый", "opaque",
        // Adhesives & Technical Marks
        "glue", "клей", "reg", "registr", "привод", "техн", "служеб", "service"
    };

    for (const char* kw : techKeywords) {
        if (lower.find(kw) != std::string::npos) {
            return true;
        }
    }
    return false;
}

std::string CropMarkEngine::TransliterateCyrillic(const std::string& src) {
    std::string out;
    out.reserve(src.size() * 2);

    for (size_t i = 0; i < src.size(); ) {
        unsigned char c1 = (unsigned char)src[i];
        if (c1 < 0x80) {
            out.push_back((char)c1);
            i++;
            continue;
        }

        if (c1 == 0xD0 && i + 1 < src.size()) {
            unsigned char c2 = (unsigned char)src[i + 1];
            i += 2;
            switch (c2) {
                // Russian uppercase
                case 0x81: out += "Yo"; break; // Ё
                case 0x90: out += "A";  break; // А
                case 0x91: out += "B";  break; // Б
                case 0x92: out += "V";  break; // В
                case 0x93: out += "G";  break; // Г
                case 0x94: out += "D";  break; // Д
                case 0x95: out += "E";  break; // Е
                case 0x96: out += "Zh"; break; // Ж
                case 0x97: out += "Z";  break; // З
                case 0x98: out += "I";  break; // И
                case 0x99: out += "Y";  break; // Й
                case 0x9A: out += "K";  break; // К
                case 0x9B: out += "L";  break; // Л
                case 0x9C: out += "M";  break; // М
                case 0x9D: out += "N";  break; // Н
                case 0x9E: out += "O";  break; // О
                case 0x9F: out += "P";  break; // П
                case 0xA0: out += "R";  break; // Р
                case 0xA1: out += "S";  break; // С
                case 0xA2: out += "T";  break; // Т
                case 0xA3: out += "U";  break; // У
                case 0xA4: out += "F";  break; // Ф
                case 0xA5: out += "Kh"; break; // Х
                case 0xA6: out += "Ts"; break; // Ц
                case 0xA7: out += "Ch"; break; // Ч
                case 0xA8: out += "Sh"; break; // Ш
                case 0xA9: out += "Shch"; break; // Щ
                case 0xAA: break;              // Ъ (silent)
                case 0xAB: out += "Y";  break; // Ы
                case 0xAC: break;              // Ь (silent)
                case 0xAD: out += "E";  break; // Э
                case 0xAE: out += "Yu"; break; // Ю
                case 0xAF: out += "Ya"; break; // Я

                // Russian lowercase (first half)
                case 0xB0: out += "a";  break; // а
                case 0xB1: out += "b";  break; // б
                case 0xB2: out += "v";  break; // в
                case 0xB3: out += "g";  break; // г
                case 0xB4: out += "d";  break; // д
                case 0xB5: out += "e";  break; // е
                case 0xB6: out += "zh"; break; // ж
                case 0xB7: out += "z";  break; // з
                case 0xB8: out += "i";  break; // и
                case 0xB9: out += "y";  break; // й
                case 0xBA: out += "k";  break; // к
                case 0xBB: out += "l";  break; // л
                case 0xBC: out += "m";  break; // м
                case 0xBD: out += "n";  break; // н
                case 0xBE: out += "o";  break; // о
                case 0xBF: out += "p";  break; // п

                // Ukrainian additions
                case 0x84: out += "Ye"; break; // Є
                case 0x86: out += "I";  break; // І
                case 0x87: out += "Yi"; break; // Ї
                default:
                    out.push_back((char)c1);
                    out.push_back((char)c2);
                    break;
            }
            continue;
        }

        if (c1 == 0xD1 && i + 1 < src.size()) {
            unsigned char c2 = (unsigned char)src[i + 1];
            i += 2;
            switch (c2) {
                // Russian lowercase (second half)
                case 0x80: out += "r";  break; // р
                case 0x81: out += "s";  break; // с
                case 0x82: out += "t";  break; // т
                case 0x83: out += "u";  break; // у
                case 0x84: out += "f";  break; // ф
                case 0x85: out += "kh"; break; // х
                case 0x86: out += "ts"; break; // ц
                case 0x87: out += "ch"; break; // ч
                case 0x88: out += "sh"; break; // ш
                case 0x89: out += "shch"; break; // щ
                case 0x8A: break;              // ъ (silent)
                case 0x8B: out += "y";  break; // ы
                case 0x8C: break;              // ь (silent)
                case 0x8D: out += "e";  break; // э
                case 0x8E: out += "yu"; break; // ю
                case 0x8F: out += "ya"; break; // я
                case 0x91: out += "yo"; break; // ё

                // Ukrainian additions
                case 0x94: out += "ye"; break; // є
                case 0x96: out += "i";  break; // і
                case 0x97: out += "yi"; break; // ї
                default:
                    out.push_back((char)c1);
                    out.push_back((char)c2);
                    break;
            }
            continue;
        }

        if (c1 == 0xD2 && i + 1 < src.size()) {
            unsigned char c2 = (unsigned char)src[i + 1];
            i += 2;
            if (c2 == 0x90) { out += "G"; continue; } // Ґ
            if (c2 == 0x91) { out += "g"; continue; } // ґ
            out.push_back((char)c1);
            out.push_back((char)c2);
            continue;
        }

        // Other UTF-8 sequences pass through
        out.push_back((char)c1);
        i++;
    }
    return out;
}

std::vector<DocumentSpotColor> CropMarkEngine::ScanDocumentPantones() {
    std::vector<DocumentSpotColor> spots;
    if (!sAIDocument || !sAISwatchList || !sAICustomColor) {
        PluginLog("[ScanDocumentPantones] Suites missing: sAIDoc=%p, sAISwatch=%p, sAICustom=%p",
                  sAIDocument, sAISwatchList, sAICustomColor);
        return spots;
    }

    AIDocumentHandle doc = nullptr;
    if (sAIDocument) {
        sAIDocument->GetDocument(&doc);
    }
    if (!doc && sAIDocumentList) {
        ai::int32 docCount = 0;
        if (sAIDocumentList->Count(&docCount) == kNoErr && docCount > 0) {
            sAIDocumentList->GetNthDocument(&doc, 0);
        }
    }

    PluginLog("[ScanDocumentPantones] Scanning doc=%p...", doc);

    std::vector<std::string> seenNames;

    auto addSpotCandidate = [&](const std::string& name, AICustomColorHandle hColor, const AICustomColor& cc) {
        if (name.empty()) return;
        if ((cc.flag & kCustomRegistrationColor) != 0 ||
            name == "[Registration]" || name == "Registration" ||
            name == "[None]" || name == "None") {
            return;
        }

        // Deduplicate by name (case-insensitive)
        for (const auto& sn : seenNames) {
            if (_stricmp(sn.c_str(), name.c_str()) == 0) return;
        }
        seenNames.push_back(name);

        DocumentSpotColor spot;
        spot.name = name;
        spot.handle = hColor;

        if (cc.kind == kCustomFourColor) {
            spot.c = (float)cc.c.f.cyan;
            spot.m = (float)cc.c.f.magenta;
            spot.y = (float)cc.c.f.yellow;
            spot.k = (float)cc.c.f.black;
        } else if (sAICustomColor) {
            AIColor nonGlobal;
            std::memset(&nonGlobal, 0, sizeof(nonGlobal));
            if (sAICustomColor->ConvertToNonGlobal(hColor, 0.0f, &nonGlobal) == kNoErr) {
                if (nonGlobal.kind == kFourColor) {
                    spot.c = (float)nonGlobal.c.f.cyan;
                    spot.m = (float)nonGlobal.c.f.magenta;
                    spot.y = (float)nonGlobal.c.f.yellow;
                    spot.k = (float)nonGlobal.c.f.black;
                } else if (nonGlobal.kind == kGrayColor) {
                    spot.c = spot.m = spot.y = 0.0f;
                    spot.k = (float)nonGlobal.c.g.gray;
                } else {
                    spot.c = 0.0f; spot.m = 0.0f; spot.y = 0.0f; spot.k = 1.0f;
                }
            } else {
                spot.c = 0.0f; spot.m = 0.0f; spot.y = 0.0f; spot.k = 1.0f;
            }
        } else {
            spot.c = 0.0f; spot.m = 0.0f; spot.y = 0.0f; spot.k = 1.0f;
        }

        spot.isTechnical = IsTechnicalSpotName(name);
        spot.isSelected = !spot.isTechnical; // Technical spots excluded by default!

        PluginLog("  [Spot Added] '%s' (c=%.2f m=%.2f y=%.2f k=%.2f, isTech=%d)",
                  name.c_str(), spot.c, spot.m, spot.y, spot.k, spot.isTechnical ? 1 : 0);
        spots.push_back(spot);
    };

    // 1. Enumerate via sAICustomColorSuite (all custom and spot colors in the document)
    if (sAICustomColor) {
        ai::int32 ccCount = 0;
        AIErr err = sAICustomColor->CountCustomColors(&ccCount);
        PluginLog("[ScanDocumentPantones] CountCustomColors err=%d count=%d", err, ccCount);
        if (err == kNoErr) {
            for (ai::int32 i = 0; i < ccCount; ++i) {
                AICustomColorHandle hColor = nullptr;
                if (sAICustomColor->GetNthCustomColor(i, &hColor) == kNoErr && hColor) {
                    AICustomColor cc;
                    if (sAICustomColor->GetCustomColor(hColor, &cc) == kNoErr) {
                        ai::UnicodeString uName;
                        sAICustomColor->GetCustomColorName(hColor, uName);
                        std::string name = uName.as_UTF8();
                        PluginLog("    CustomColor[%d]: '%s' flag=0x%04X kind=%d", i, name.c_str(), cc.flag, cc.kind);

                        bool isSpot = (cc.flag & kCustomSpotColor) != 0;
                        if (!isSpot) {
                            std::string lowerName = name;
                            for (char& ch : lowerName) ch = (char)tolower((unsigned char)ch);
                            if (lowerName.find("pantone") != std::string::npos) isSpot = true;
                        }

                        if (isSpot) {
                            addSpotCandidate(name, hColor, cc);
                        }
                    }
                }
            }
        }
    }

    // 2. Enumerate via sAISwatchListSuite (document swatch panel)
    if (sAISwatchList) {
        AISwatchListRef swList = nullptr;
        sAISwatchList->GetSwatchList(doc, &swList);
        if (!swList) {
            sAISwatchList->GetSwatchList(nullptr, &swList);
        }
        if (swList) {
            ai::int32 swCount = sAISwatchList->CountSwatches(swList);
            PluginLog("[ScanDocumentPantones] SwatchList CountSwatches = %d", swCount);
            for (ai::int32 i = 0; i < swCount; ++i) {
                AISwatchRef sw = sAISwatchList->GetNthSwatch(swList, i);
                if (!sw) continue;

                AIColor color;
                if (sAISwatchList->GetAIColor(sw, &color) != kNoErr) continue;

                ai::UnicodeString swName;
                sAISwatchList->GetSwatchName(sw, swName);
                std::string sName = swName.as_UTF8();

                if (color.kind == kCustomColor && color.c.c.color && sAICustomColor) {
                    AICustomColorHandle hColor = color.c.c.color;
                    AICustomColor cc;
                    if (sAICustomColor->GetCustomColor(hColor, &cc) == kNoErr) {
                        ai::UnicodeString uName;
                        sAICustomColor->GetCustomColorName(hColor, uName);
                        std::string name = uName.as_UTF8();
                        if (name.empty()) name = sName;

                        PluginLog("    Swatch[%d]: '%s' flag=0x%04X kind=%d", i, name.c_str(), cc.flag, cc.kind);

                        bool isSpot = (cc.flag & kCustomSpotColor) != 0;
                        if (!isSpot) {
                            std::string lowerName = name;
                            for (char& ch : lowerName) ch = (char)tolower((unsigned char)ch);
                            if (lowerName.find("pantone") != std::string::npos) isSpot = true;
                        }

                        if (isSpot) {
                            addSpotCandidate(name, hColor, cc);
                        }
                    }
                }
            }
        } else {
            PluginLog("[ScanDocumentPantones] swList is null!");
        }
    }

    PluginLog("[ScanDocumentPantones] Total spots found: %zu", spots.size());
    ConfigManager::GetInstance().SetDocumentSpots(spots);
    return spots;
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

static void CreateRegistrationTarget(AIArtHandle parentGroup, double cx, double cy,
                                     bool whiteUnderlay, double strokeWidthPt,
                                     double rOuterPt, double armPt) {
    if (whiteUnderlay) {
        double wStroke = strokeWidthPt + 1.2;
        CreateRegistrationCircle(parentGroup, cx, cy, rOuterPt, false, wStroke);
        CreateLineArt(parentGroup, cx - armPt, cy, cx + armPt, cy, false, wStroke);
        CreateLineArt(parentGroup, cx, cy - armPt, cx, cy + armPt, false, wStroke);
    }

    CreateRegistrationCircle(parentGroup, cx, cy, rOuterPt, true, strokeWidthPt);
    CreateLineArt(parentGroup, cx - armPt, cy, cx + armPt, cy, true, strokeWidthPt);
    CreateLineArt(parentGroup, cx, cy - armPt, cx, cy + armPt, true, strokeWidthPt);
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

struct PatchItem {
    bool isSpot;
    AICustomColorHandle spotHandle;
    float c, m, y, k;
};

static int CreateColorBarStrip(
    AIArtHandle parentGroup,
    double startX, double startY,
    bool isHorizontal,
    double patchWidthPt,
    double patchHeightPt,
    double limitCoordPt,
    int barMode,
    int standard,
    int length,
    bool barCyan, bool barMagenta, bool barYellow, bool barBlack,
    const std::vector<DocumentSpotColor>& selectedSpots,
    double repeatGapMm = 0.0
) {
    if (patchWidthPt <= 0.0 || patchHeightPt <= 0.0) return 0;

    std::vector<PatchItem> patchSequence;

    if (barMode == kColorBar100Solids || barMode == kColorBar100PrimariesOnly) {
        // 100% Solid Patches mode (CMYK + Pantones without halftones)
        // 1. Process Primaries
        if (barCyan)    patchSequence.push_back({ false, nullptr, 1.0f, 0.0f, 0.0f, 0.0f });
        if (barMagenta) patchSequence.push_back({ false, nullptr, 0.0f, 1.0f, 0.0f, 0.0f });
        if (barYellow)  patchSequence.push_back({ false, nullptr, 0.0f, 0.0f, 1.0f, 0.0f });
        if (barBlack)   patchSequence.push_back({ false, nullptr, 0.0f, 0.0f, 0.0f, 1.0f });

        // 2. Overprints (Binaries & 3-Color Gray) - ONLY in 100% Solids mode with binaries
        if (barMode == kColorBar100Solids) {
            if (barMagenta && barYellow) patchSequence.push_back({ false, nullptr, 0.0f, 1.0f, 1.0f, 0.0f }); // Red
            if (barCyan && barYellow)    patchSequence.push_back({ false, nullptr, 1.0f, 0.0f, 1.0f, 0.0f }); // Green
            if (barCyan && barMagenta)   patchSequence.push_back({ false, nullptr, 1.0f, 1.0f, 0.0f, 0.0f }); // Blue
            if (barCyan && barMagenta && barYellow)
                                         patchSequence.push_back({ false, nullptr, 1.0f, 1.0f, 1.0f, 0.0f }); // 3C Gray
        }

        // 3. Selected Pantones
        for (const auto& sp : selectedSpots) {
            if (sp.isSelected) {
                patchSequence.push_back({ true, sp.handle, sp.c, sp.m, sp.y, sp.k });
            }
        }

        // Fallback if none selected
        if (patchSequence.empty()) {
            patchSequence.push_back({ false, nullptr, 1.0f, 0.0f, 0.0f, 0.0f });
            patchSequence.push_back({ false, nullptr, 0.0f, 1.0f, 0.0f, 0.0f });
            patchSequence.push_back({ false, nullptr, 0.0f, 0.0f, 1.0f, 0.0f });
            patchSequence.push_back({ false, nullptr, 0.0f, 0.0f, 0.0f, 1.0f });
        }
    } else {
        // ISO Standards mode
        const ColorPatchDef* isoPatches = nullptr;
        size_t isoCount = 0;
        bool isIso7 = (barMode == kColorBarISO12647_7Mode || standard == kStandardISO12647_7);
        if (isIso7) {
            if (length == kColorBarLong) {
                isoPatches = g_iso7_long;
                isoCount = sizeof(g_iso7_long) / sizeof(g_iso7_long[0]);
            } else {
                isoPatches = g_iso7_short;
                isoCount = sizeof(g_iso7_short) / sizeof(g_iso7_short[0]);
            }
        } else {
            if (length == kColorBarLong) {
                isoPatches = g_iso2_long;
                isoCount = sizeof(g_iso2_long) / sizeof(g_iso2_long[0]);
            } else {
                isoPatches = g_iso2_short;
                isoCount = sizeof(g_iso2_short) / sizeof(g_iso2_short[0]);
            }
        }
        for (size_t i = 0; i < isoCount; ++i) {
            patchSequence.push_back({ false, nullptr, isoPatches[i].c, isoPatches[i].m, isoPatches[i].y, isoPatches[i].k });
        }
    }

    if (patchSequence.empty()) return 0;

    const double ptPerMm = 2.83464567;
    const double gapPt = (repeatGapMm < 0.0 ? 0.0 : repeatGapMm) * ptPerMm;
    double currentOffset = 0.0;
    bool reachedLimit = false;
    int createdCount = 0;

    // For horizontal bar: patch dimension along bar is patchWidthPt, perpendicular is patchHeightPt
    // For vertical bar: patch dimension along bar is patchHeightPt, perpendicular is patchWidthPt
    const double stepSizePt = isHorizontal ? patchWidthPt : patchHeightPt;
    const double crossSizePt = isHorizontal ? patchHeightPt : patchWidthPt;

    int maxLoops = 2000;
    while (!reachedLimit && --maxLoops > 0) {
        for (size_t p = 0; p < patchSequence.size(); ++p) {
            double px = isHorizontal ? (startX + currentOffset) : startX;
            double py = isHorizontal ? startY : (startY - currentOffset);

            if (isHorizontal) {
                if (px + stepSizePt > limitCoordPt + 0.01) {
                    reachedLimit = true;
                    break;
                }
            } else {
                if (py - stepSizePt < limitCoordPt - 0.01) {
                    reachedLimit = true;
                    break;
                }
            }

            AIArtHandle pathArt = nullptr;
            sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, parentGroup, &pathArt);
            if (!pathArt) {
                currentOffset += stepSizePt;
                continue;
            }

            AIPathSegment segs[4];
            if (isHorizontal) {
                segs[0].p.h = (AIReal)px; segs[0].p.v = (AIReal)(py - crossSizePt);
                segs[1].p.h = (AIReal)px; segs[1].p.v = (AIReal)py;
                segs[2].p.h = (AIReal)(px + stepSizePt); segs[2].p.v = (AIReal)py;
                segs[3].p.h = (AIReal)(px + stepSizePt); segs[3].p.v = (AIReal)(py - crossSizePt);
            } else {
                segs[0].p.h = (AIReal)px; segs[0].p.v = (AIReal)(py - stepSizePt);
                segs[1].p.h = (AIReal)px; segs[1].p.v = (AIReal)py;
                segs[2].p.h = (AIReal)(px + crossSizePt); segs[2].p.v = (AIReal)py;
                segs[3].p.h = (AIReal)(px + crossSizePt); segs[3].p.v = (AIReal)(py - stepSizePt);
            }
            for (int k = 0; k < 4; ++k) {
                segs[k].in = segs[k].out = segs[k].p;
                segs[k].corner = true;
            }

            sAIPath->SetPathSegments(pathArt, 0, 4, segs);
            sAIPath->SetPathClosed(pathArt, true);

            AIPathStyle style;
            AIBoolean hasStyle = false;
            sAIPathStyle->GetPathStyle(pathArt, &style, &hasStyle);

            style.fillPaint = true;
            style.strokePaint = false;

            const auto& patch = patchSequence[p];
            if (patch.isSpot && patch.spotHandle && sAICustomColor) {
                style.fill.color.kind = kCustomColor;
                style.fill.color.c.c.color = patch.spotHandle;
                style.fill.color.c.c.tint = 0.0f; // 100% solid spot color ink
            } else {
                style.fill.color.kind = kFourColor;
                style.fill.color.c.f.cyan = patch.c;
                style.fill.color.c.f.magenta = patch.m;
                style.fill.color.c.f.yellow = patch.y;
                style.fill.color.c.f.black = patch.k;
            }

            sAIPathStyle->SetPathStyle(pathArt, &style);
            createdCount++;

            currentOffset += stepSizePt;
        }

        if (reachedLimit) break;
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

static AILayerHandle GetOrCreateLayer(const std::string& layerName) {
    if (!sAILayer || layerName.empty() || layerName == "[ Current Layer ]") return nullptr;

    std::string actualName = layerName;
    if (actualName == "[ New: Crop Marks ]") {
        actualName = "Crop Marks";
    }

    ai::int32 layerCount = 0;
    if (sAILayer->CountLayers(&layerCount) == kNoErr && layerCount > 0) {
        for (ai::int32 i = 0; i < layerCount; ++i) {
            AILayerHandle layer = nullptr;
            if (sAILayer->GetNthLayer(i, &layer) == kNoErr && layer) {
                ai::UnicodeString title;
                if (sAILayer->GetLayerTitle(layer, title) == kNoErr) {
                    if (title.as_UTF8() == actualName) {
                        return layer;
                    }
                }
            }
        }
    }

    // Layer not found: create new top layer
    AILayerHandle newLayer = nullptr;
    if (sAILayer->InsertLayer(nullptr, kPlaceAboveAll, &newLayer) == kNoErr && newLayer) {
        ai::UnicodeString uTitle(actualName, kAIUTF8CharacterEncoding);
        sAILayer->SetLayerTitle(newLayer, uTitle);
        return newLayer;
    }
    return nullptr;
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
    if (sAIDocument) {
        sAIDocument->GetDocument(&doc);
    }
    if (!doc && sAIDocumentList) {
        ai::int32 docCount = 0;
        if (sAIDocumentList->Count(&docCount) == kNoErr && docCount > 0) {
            sAIDocumentList->GetNthDocument(&doc, 0);
            if (doc && sAIDocumentList) {
                sAIDocumentList->Activate(doc, true);
            }
        }
    }
    if (!doc) {
        res.message = "No active document found.";
        PluginLog("[CropMarkEngine] FAIL: No active document found. (sAIDoc=%p, sAIDocList=%p)", sAIDocument, sAIDocumentList);
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
            if (!a || IsGuide(a) || IsCropMarksGroup(a) || IsInsideCropMarksGroup(a)) continue;
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

    // Prepress Registration Targets and CMYK / Spot Color Bars helpers
    auto addColorBars = [&](AIArtHandle grp, const AIRealRect& box) {
        if (!params.colorBarsLong && !params.colorBarsShort) return;

        const double w = box.right - box.left;
        const double h = box.top - box.bottom;
        const double ptPerMm = 2.83464567;
        const double patchWPt = CropMarkParams::UnitToPoints(params.patchWidth, params.units);
        const double patchHPt = CropMarkParams::UnitToPoints(params.patchHeight, params.units);
        const double barsOff = CropMarkParams::UnitToPoints(params.barsOffset, params.units);

        // Prepress standard offsets:
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
                double barStartY = box.top + oVer + barsOff + patchHPt;
                totalMarksCreated += CreateColorBarStrip(
                    grp, horizStartX, barStartY, true, patchWPt, patchHPt, horizLimitX,
                    params.barMode, params.barStandard, params.barLength,
                    params.barCyan, params.barMagenta, params.barYellow, params.barBlack,
                    params.selectedSpots, params.barRepeatGap
                );
            } else {
                // Long side in portrait: Left vertical bar
                double barStartX = box.left - oHor - barsOff - patchHPt;
                totalMarksCreated += CreateColorBarStrip(
                    grp, barStartX, vertStartY, false, patchWPt, patchHPt, vertLimitY,
                    params.barMode, params.barStandard, params.barLength,
                    params.barCyan, params.barMagenta, params.barYellow, params.barBlack,
                    params.selectedSpots, params.barRepeatGap
                );
            }
        }

        if (params.colorBarsShort) {
            if (isLandscape) {
                // Short side in landscape: Left vertical bar
                double leftStartX = box.left - oHor - barsOff - patchHPt;
                totalMarksCreated += CreateColorBarStrip(
                    grp, leftStartX, vertStartY, false, patchWPt, patchHPt, vertLimitY,
                    params.barMode, params.barStandard, params.barLength,
                    params.barCyan, params.barMagenta, params.barYellow, params.barBlack,
                    params.selectedSpots, params.barRepeatGap
                );
            } else {
                // Short sides in portrait: Top and Bottom horizontal bars
                // Top side
                double topStartY = box.top + oVer + barsOff + patchHPt;
                totalMarksCreated += CreateColorBarStrip(
                    grp, horizStartX, topStartY, true, patchWPt, patchHPt, horizLimitX,
                    params.barMode, params.barStandard, params.barLength,
                    params.barCyan, params.barMagenta, params.barYellow, params.barBlack,
                    params.selectedSpots, params.barRepeatGap
                );

                // Bottom side
                double botStartY = box.bottom - oVer - barsOff;
                totalMarksCreated += CreateColorBarStrip(
                    grp, horizStartX, botStartY, true, patchWPt, patchHPt, horizLimitX,
                    params.barMode, params.barStandard, params.barLength,
                    params.barCyan, params.barMagenta, params.barYellow, params.barBlack,
                    params.selectedSpots, params.barRepeatGap
                );
            }
        }
    };

    auto addDocInfo = [&](AIArtHandle grp, const AIRealRect& box) {
        if (!params.docInfo) return;

        const double ptPerMm = 2.83464567;
        const double patchHPt = CropMarkParams::UnitToPoints(params.patchHeight, params.units);
        const double barsOff = CropMarkParams::UnitToPoints(params.barsOffset, params.units);
        const double horizStartX = (box.left + bHor) + 3.0 * ptPerMm;
        const double topBarStartY = box.top + oVer + barsOff + patchHPt;
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

        if (params.docInfoTranslit) {
            docName = TransliterateCyrillic(docName);
        }

        time_t now = time(nullptr);
        struct tm t;
        localtime_s(&t, &now);
        char timeBuf[64];
        strftime(timeBuf, sizeof(timeBuf), "%d.%m.%Y  %H:%M", &t);
        std::string infoText = docName + "  " + timeBuf;

        PluginLog("[addDocInfo] Start: textAnchorX=%.2f, textAnchorY=%.2f, infoText='%s', translit=%d, sAITextFrame=%p",
                  textAnchorX, textAnchorY, infoText.c_str(), params.docInfoTranslit ? 1 : 0, sAITextFrame);

        if (sAITextFrame) {
            AIArtHandle textArt = nullptr;
            AIRealPoint anchor = { (AIReal)textAnchorX, (AIReal)textAnchorY };
            AIErr err = sAITextFrame->NewPointText(kPlaceInsideOnTop, grp, kHorizontalTextOrientation, anchor, &textArt);
            PluginLog("[addDocInfo] NewPointText err=%d, textArt=%p", err, textArt);
            if (err == kNoErr && textArt) {
                TextRangeRef textRange = nullptr;
                if (sAITextFrame->GetATETextRange(textArt, &textRange) == kNoErr && textRange) {
                    ai::UnicodeString uText(infoText, kAIUTF8CharacterEncoding);
                    auto asUni = uText.as_ASUnicode();
                    if (sTextRange && !asUni.empty()) {
                        auto insErr = sTextRange->InsertAfter_AsUnicode(textRange, asUni.c_str(), (ATETextDOM::Int32)asUni.length());
                        PluginLog("[addDocInfo] InsertAfter_AsUnicode err=%d, len=%zu", (int)insErr, asUni.length());
                    }
                    if (sTextRange) {
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
        const double rOuterPt = (params.regCircleDiam * 0.5) * ptPerMm;
        const double armPt = (params.regCrossLength * 0.5) * ptPerMm;
        const double markRadiusPt = (std::max)(rOuterPt, armPt);
        const double regStrokePt = CropMarkParams::UnitToPoints(params.regStrokeWidth, params.regStrokeUnits);

        const double offHorPt = CropMarkParams::UnitToPoints(params.regOffsetHor, params.units);
        const double offVerPt = CropMarkParams::UnitToPoints(params.regOffsetVer, params.units);

        const double midX = (box.left + box.right) * 0.5;
        const double midY = (box.top + box.bottom) * 0.5;

        const double topY = box.top + oVer + offVerPt + markRadiusPt;
        const double botY = box.bottom - oVer - offVerPt - markRadiusPt;
        const double leftX = box.left - oHor - offHorPt - markRadiusPt;
        const double rightX = box.right + oHor + offHorPt + markRadiusPt;

        struct RegPt { double x; double y; };
        std::vector<RegPt> pts;

        if (params.regPositions == kRegPositionsCenters) {
            pts.push_back({ midX, topY });
            pts.push_back({ midX, botY });
            pts.push_back({ leftX, midY });
            pts.push_back({ rightX, midY });
        } else if (params.regPositions == kRegPositionsCorners) {
            pts.push_back({ leftX, topY });
            pts.push_back({ rightX, topY });
            pts.push_back({ leftX, botY });
            pts.push_back({ rightX, botY });
        } else if (params.regPositions == kRegPositionsCentersCorners) {
            pts.push_back({ midX, topY });
            pts.push_back({ midX, botY });
            pts.push_back({ leftX, midY });
            pts.push_back({ rightX, midY });
            pts.push_back({ leftX, topY });
            pts.push_back({ rightX, topY });
            pts.push_back({ leftX, botY });
            pts.push_back({ rightX, botY });
        } else if (params.regPositions == kRegPositionsAll16) {
            // Top 5
            pts.push_back({ leftX, topY });
            pts.push_back({ box.left + markRadiusPt, topY });
            pts.push_back({ midX, topY });
            pts.push_back({ box.right - markRadiusPt, topY });
            pts.push_back({ rightX, topY });

            // Bottom 5
            pts.push_back({ leftX, botY });
            pts.push_back({ box.left + markRadiusPt, botY });
            pts.push_back({ midX, botY });
            pts.push_back({ box.right - markRadiusPt, botY });
            pts.push_back({ rightX, botY });

            // Left 3
            pts.push_back({ leftX, box.top - markRadiusPt });
            pts.push_back({ leftX, midY });
            pts.push_back({ leftX, box.bottom + markRadiusPt });

            // Right 3
            pts.push_back({ rightX, box.top - markRadiusPt });
            pts.push_back({ rightX, midY });
            pts.push_back({ rightX, box.bottom + markRadiusPt });
        }

        std::vector<RegPt> created;
        for (const auto& p : pts) {
            bool dup = false;
            for (const auto& c : created) {
                if (std::abs(c.x - p.x) < 0.1 && std::abs(c.y - p.y) < 0.1) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                CreateRegistrationTarget(grp, p.x, p.y, params.regWhiteUnderlay, regStrokePt, rOuterPt, armPt);
                created.push_back(p);
                totalMarksCreated++;
            }
        }
    };

    AILayerHandle targetLayer = nullptr;
    if (params.layerMode == kLayerSpecific && !params.layerName.empty() && params.layerName != "[ Current Layer ]") {
        targetLayer = GetOrCreateLayer(params.layerName);
    }

    auto createContainerAndSubgroups = [&](AIArtHandle anchorArt, AIArtHandle& outMarksGroup,
                                           AIArtHandle& outCutsGroup, AIArtHandle& outRegGroup, AIArtHandle& outBarsGroup) -> bool {
        outMarksGroup = nullptr;
        outCutsGroup = nullptr;
        outRegGroup = nullptr;
        outBarsGroup = nullptr;

        if (targetLayer) {
            AIArtHandle layerRoot = nullptr;
            if (sAIArt->GetFirstArtOfLayer(targetLayer, &layerRoot) == kNoErr && layerRoot) {
                sAIArt->NewArt(kGroupArt, kPlaceInsideOnTop, layerRoot, &outMarksGroup);
            }
        }
        if (!outMarksGroup) {
            AIArtHandle safeAnchor = GetSafePlacementAnchor(anchorArt);
            if (safeAnchor) {
                sAIArt->NewArt(kGroupArt, kPlaceAbove, safeAnchor, &outMarksGroup);
            } else {
                sAIArt->NewArt(kGroupArt, kPlaceAboveAll, nullptr, &outMarksGroup);
            }
        }
        if (!outMarksGroup) return false;
        sAIArt->SetArtName(outMarksGroup, ai::UnicodeString("Crop Marks"));

        if (params.groupMode != kGroupNone) {
            sAIArt->NewArt(kGroupArt, kPlaceInsideOnBottom, outMarksGroup, &outCutsGroup);
            if (outCutsGroup) sAIArt->SetArtName(outCutsGroup, ai::UnicodeString("Cuts"));

            if (params.regMarks) {
                sAIArt->NewArt(kGroupArt, kPlaceInsideOnBottom, outMarksGroup, &outRegGroup);
                if (outRegGroup) sAIArt->SetArtName(outRegGroup, ai::UnicodeString("Registration Marks"));
            }

            if (params.colorBarsLong || params.colorBarsShort || params.docInfo) {
                sAIArt->NewArt(kGroupArt, kPlaceInsideOnBottom, outMarksGroup, &outBarsGroup);
                if (outBarsGroup) sAIArt->SetArtName(outBarsGroup, ai::UnicodeString("Color Bars"));
            }
        }
        return true;
    };

    auto finalizeSubgroups = [&](AIArtHandle marksGroup, AIArtHandle& cutsGroup, AIArtHandle& regGroup, AIArtHandle& barsGroup) {
        auto cleanupEmptySubgroup = [](AIArtHandle& grp) {
            if (!grp) return;
            AIArtHandle child = nullptr;
            if (sAIArt->GetArtFirstChild(grp, &child) == kNoErr && !child) {
                sAIArt->DisposeArt(grp);
                grp = nullptr;
            }
        };
        cleanupEmptySubgroup(cutsGroup);
        cleanupEmptySubgroup(regGroup);
        cleanupEmptySubgroup(barsGroup);

        if (barsGroup && cutsGroup) {
            if (params.cropMarksLayerOrder == kCropMarksAbovePatches) {
                sAIArt->ReorderArt(cutsGroup, kPlaceAbove, barsGroup);
            } else {
                sAIArt->ReorderArt(cutsGroup, kPlaceBelow, barsGroup);
            }
        }
        if (regGroup) {
            if (params.crossMarksLayerOrder == kCrossMarksAbovePatches) {
                if (barsGroup) sAIArt->ReorderArt(regGroup, kPlaceAbove, barsGroup);
                if (cutsGroup) sAIArt->ReorderArt(regGroup, kPlaceAbove, cutsGroup);
            } else {
                if (barsGroup) sAIArt->ReorderArt(regGroup, kPlaceBelow, barsGroup);
            }
        }
    };

    if (params.targetMode == kTargetArtboard) {
        // Artboard target
        AIArtHandle marksGroup = nullptr;
        AIArtHandle cutsGroup = nullptr;
        AIArtHandle regGroup = nullptr;
        AIArtHandle barsGroup = nullptr;

        if (!createContainerAndSubgroups(nullptr, marksGroup, cutsGroup, regGroup, barsGroup)) {
            res.message = "Failed to create crop marks container group.";
            return res;
        }

        std::vector<LineDedupeEntry> dedupeList;
        std::vector<AIArtHandle> groupMarks;

        auto addMark = [&](double x1, double y1, double x2, double y2) {
            AIArtHandle destGrp = cutsGroup ? cutsGroup : marksGroup;
            if (params.whiteContour) {
                LineDedupeEntry whiteEntry = { x1, y1, x2, y2, false };
                bool exists = false;
                for (const auto& d : dedupeList) {
                    if (AreLinesEqual(d, whiteEntry)) { exists = true; break; }
                }
                if (!exists) {
                    AIArtHandle wArt = CreateLineArt(destGrp, x1, y1, x2, y2, false, strokeWidthPt + 1.0);
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
                AIArtHandle rArt = CreateLineArt(destGrp, x1, y1, x2, y2, true, strokeWidthPt);
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

        AIArtHandle regDestGrp = regGroup ? regGroup : marksGroup;
        AIArtHandle barDestGrp = barsGroup ? barsGroup : marksGroup;

        if (params.crossMarksLayerOrder == kCrossMarksBelowPatches) {
            addRegistrationTargets(regDestGrp, targetItems[0].bounds);
        }
        if (params.cropMarksLayerOrder == kCropMarksBelowPatches) {
            generateArtboardMarks();
        }

        addColorBars(barDestGrp, targetItems[0].bounds);
        addDocInfo(barDestGrp, targetItems[0].bounds);

        if (params.cropMarksLayerOrder == kCropMarksAbovePatches) {
            generateArtboardMarks();
        }
        if (params.crossMarksLayerOrder == kCrossMarksAbovePatches) {
            addRegistrationTargets(regDestGrp, targetItems[0].bounds);
        }

        finalizeSubgroups(marksGroup, cutsGroup, regGroup, barsGroup);

        sAIArt->SetArtUserAttr(marksGroup, kArtSelected, 0);
        for (AIArtHandle m : groupMarks) {
            sAIArt->SetArtUserAttr(m, kArtSelected, 0);
        }
        totalMarksCreated = (int)groupMarks.size();

    } else if (params.targetMode == kTargetEntireSelection) {
        // ENTIRE SELECTION (Single Unified Bounding Box across all selected items)
        AIRealRect unionBounds = targetItems[0].bounds;
        for (size_t k = 1; k < targetItems.size(); ++k) {
            if (targetItems[k].bounds.left < unionBounds.left) unionBounds.left = targetItems[k].bounds.left;
            if (targetItems[k].bounds.top > unionBounds.top) unionBounds.top = targetItems[k].bounds.top;
            if (targetItems[k].bounds.right > unionBounds.right) unionBounds.right = targetItems[k].bounds.right;
            if (targetItems[k].bounds.bottom < unionBounds.bottom) unionBounds.bottom = targetItems[k].bounds.bottom;
        }

        AIArtHandle marksGroup = nullptr;
        AIArtHandle cutsGroup = nullptr;
        AIArtHandle regGroup = nullptr;
        AIArtHandle barsGroup = nullptr;

        if (!createContainerAndSubgroups(targetItems[0].art, marksGroup, cutsGroup, regGroup, barsGroup)) {
            res.message = "Failed to create crop marks container group.";
            return res;
        }

        std::vector<LineDedupeEntry> dedupeList;
        std::vector<AIArtHandle> groupMarks;

        auto addMark = [&](double x1, double y1, double x2, double y2) {
            AIArtHandle destGrp = cutsGroup ? cutsGroup : marksGroup;
            if (params.whiteContour) {
                LineDedupeEntry whiteEntry = { x1, y1, x2, y2, false };
                bool exists = false;
                for (const auto& d : dedupeList) {
                    if (AreLinesEqual(d, whiteEntry)) { exists = true; break; }
                }
                if (!exists) {
                    AIArtHandle wArt = CreateLineArt(destGrp, x1, y1, x2, y2, false, strokeWidthPt + 1.0);
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
                AIArtHandle rArt = CreateLineArt(destGrp, x1, y1, x2, y2, true, strokeWidthPt);
                if (rArt) {
                    dedupeList.push_back(regEntry);
                    groupMarks.push_back(rArt);
                }
            }
        };

        auto generateEntireCropMarks = [&]() {
            GenerateMarksForRect(unionBounds, params, true, true, true, true,
                                 lVer, lHor, oVer, oHor, bVer, bHor, addMark);
        };

        AIArtHandle regDestGrp = regGroup ? regGroup : marksGroup;
        AIArtHandle barDestGrp = barsGroup ? barsGroup : marksGroup;

        if (params.crossMarksLayerOrder == kCrossMarksBelowPatches) {
            addRegistrationTargets(regDestGrp, unionBounds);
        }
        if (params.cropMarksLayerOrder == kCropMarksBelowPatches) {
            generateEntireCropMarks();
        }

        addColorBars(barDestGrp, unionBounds);
        addDocInfo(barDestGrp, unionBounds);

        if (params.cropMarksLayerOrder == kCropMarksAbovePatches) {
            generateEntireCropMarks();
        }
        if (params.crossMarksLayerOrder == kCrossMarksAbovePatches) {
            addRegistrationTargets(regDestGrp, unionBounds);
        }

        finalizeSubgroups(marksGroup, cutsGroup, regGroup, barsGroup);

        totalMarksCreated += (int)groupMarks.size();

        sAIArt->SetArtUserAttr(marksGroup, kArtSelected, 0);
        for (AIArtHandle m : groupMarks) {
            sAIArt->SetArtUserAttr(m, kArtSelected, 0);
        }

        // Apply Grouping Mode
        if (params.groupMode == kGroupMarksAndSelection && !targetLayer) {
            AIArtHandle entireGroup = nullptr;
            sAIArt->NewArt(kGroupArt, kPlaceBelow, marksGroup, &entireGroup);
            if (entireGroup) {
                sAIArt->ReorderArt(marksGroup, kPlaceInsideOnBottom, entireGroup);
                for (const auto& ti : targetItems) {
                    if (ti.art) {
                        AIArtHandle par = nullptr;
                        sAIArt->GetArtParent(ti.art, &par);
                        if (par && IsLayerArt(par)) {
                            sAIArt->ReorderArt(ti.art, kPlaceInsideOnTop, entireGroup);
                        }
                    }
                }
                sAIArt->SetArtUserAttr(entireGroup, kArtSelected, 0);
            }
        } else if (params.groupMode == kGroupNone) {
            for (AIArtHandle m : groupMarks) {
                sAIArt->ReorderArt(m, kPlaceBelow, marksGroup);
            }
            sAIArt->DisposeArt(marksGroup);
        }

    } else {
        // EACH OBJECT IN SELECTION: split into spatial clusters
        std::vector<Cluster> clusters = BuildClusters(targetItems, 50.0);

        for (const auto& cluster : clusters) {
            if (cluster.items.empty()) continue;

            AIArtHandle anchorArt = cluster.items[0].art;
            AIArtHandle marksGroup = nullptr;
            AIArtHandle cutsGroup = nullptr;
            AIArtHandle regGroup = nullptr;
            AIArtHandle barsGroup = nullptr;

            if (!createContainerAndSubgroups(anchorArt, marksGroup, cutsGroup, regGroup, barsGroup)) continue;

            std::vector<LineDedupeEntry> dedupeList;
            std::vector<AIArtHandle> clusterMarks;

            auto addMark = [&](double x1, double y1, double x2, double y2) {
                AIArtHandle destGrp = cutsGroup ? cutsGroup : marksGroup;
                if (params.whiteContour) {
                    LineDedupeEntry whiteEntry = { x1, y1, x2, y2, false };
                    bool exists = false;
                    for (const auto& d : dedupeList) {
                        if (AreLinesEqual(d, whiteEntry)) { exists = true; break; }
                    }
                    if (!exists) {
                        AIArtHandle wArt = CreateLineArt(destGrp, x1, y1, x2, y2, false, strokeWidthPt + 1.0);
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
                    AIArtHandle rArt = CreateLineArt(destGrp, x1, y1, x2, y2, true, strokeWidthPt);
                    if (rArt) {
                        dedupeList.push_back(regEntry);
                        clusterMarks.push_back(rArt);
                    }
                }
            };

            auto generateClusterCropMarks = [&]() {
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
            };

            AIArtHandle regDestGrp = regGroup ? regGroup : marksGroup;
            AIArtHandle barDestGrp = barsGroup ? barsGroup : marksGroup;

            if (params.crossMarksLayerOrder == kCrossMarksBelowPatches) {
                addRegistrationTargets(regDestGrp, cluster.unionRc);
            }
            if (params.cropMarksLayerOrder == kCropMarksBelowPatches) {
                generateClusterCropMarks();
            }

            addColorBars(barDestGrp, cluster.unionRc);
            addDocInfo(barDestGrp, cluster.unionRc);

            if (params.cropMarksLayerOrder == kCropMarksAbovePatches) {
                generateClusterCropMarks();
            }
            if (params.crossMarksLayerOrder == kCrossMarksAbovePatches) {
                addRegistrationTargets(regDestGrp, cluster.unionRc);
            }

            finalizeSubgroups(marksGroup, cutsGroup, regGroup, barsGroup);

            if (clusterMarks.empty() && !params.regMarks && !params.colorBarsLong && !params.colorBarsShort && !params.docInfo) {
                sAIArt->DisposeArt(marksGroup);
                continue;
            }

            totalMarksCreated += (int)clusterMarks.size();

            sAIArt->SetArtUserAttr(marksGroup, kArtSelected, 0);
            for (AIArtHandle m : clusterMarks) {
                sAIArt->SetArtUserAttr(m, kArtSelected, 0);
            }

            // Apply Grouping Mode
            if (params.groupMode == kGroupMarksAndSelection && !targetLayer) {
                AIArtHandle clusterGroup = nullptr;
                sAIArt->NewArt(kGroupArt, kPlaceBelow, marksGroup, &clusterGroup);
                if (clusterGroup) {
                    sAIArt->ReorderArt(marksGroup, kPlaceInsideOnBottom, clusterGroup);
                    for (const auto& ti : cluster.items) {
                        if (ti.art) {
                            AIArtHandle par = nullptr;
                            sAIArt->GetArtParent(ti.art, &par);
                            if (par && IsLayerArt(par)) {
                                sAIArt->ReorderArt(ti.art, kPlaceInsideOnTop, clusterGroup);
                            }
                        }
                    }
                    sAIArt->SetArtUserAttr(clusterGroup, kArtSelected, 0);
                }
            } else if (params.groupMode == kGroupNone) {
                for (AIArtHandle m : clusterMarks) {
                    sAIArt->ReorderArt(m, kPlaceBelow, marksGroup);
                }
                sAIArt->DisposeArt(marksGroup);
            }
        }
    }

    if (totalMarksCreated == 0) {
        res.message = "No marks created (all directions unchecked or zero bounds).";
        return res;
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
