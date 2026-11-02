const fs = require("fs");
const path = require("path");
const { execSync } = require("child_process");

const ROOT_DIR = __dirname;
const BUILD_DEV_DIR = path.join(ROOT_DIR, "Build_DEV");
const STAGING_DIR = path.join(ROOT_DIR, "build", "staging");

function getVersion() {
    const pluginSrcPath = path.join(ROOT_DIR, "src", "MakeCropMarksAIPlugin.cpp");
    if (!fs.existsSync(pluginSrcPath)) return "1.00";
    const content = fs.readFileSync(pluginSrcPath, "utf8");
    const m = content.match(/PLUGIN_VERSION_STR\s+"v?([^"]+)"/);
    return m ? m[1] : "1.00";
}

console.log("================================================================================");
console.log("      CROPMARKS AI NATIVE PLUGIN - DEV BUILD & PACKAGING SCRIPT");
console.log("================================================================================");

// 1. Compile C++ plugin
console.log("[*] Building CropMarksAI.aip via build.bat...");
execSync(`cmd.exe /c "build.bat"`, { cwd: ROOT_DIR, stdio: "inherit" });

const version = getVersion();
console.log(`\n[*] Target Version: v${version}`);

// 2. Prepare staging directory
if (fs.existsSync(STAGING_DIR)) {
    fs.rmSync(STAGING_DIR, { recursive: true, force: true });
}
fs.mkdirSync(STAGING_DIR, { recursive: true });

// 3. Copy release artifacts into staging
const aipSource = path.join(ROOT_DIR, "build", "x64", "CropMarksAI.aip");
if (!fs.existsSync(aipSource)) {
    console.error("[ERROR] CropMarksAI.aip not found!");
    process.exit(1);
}

// Copy versioned and unversioned AIP
fs.copyFileSync(aipSource, path.join(STAGING_DIR, "CropMarksAI.aip"));
const versionedAip = path.join(ROOT_DIR, "build", "x64", `CropMarksAI_v${version}.aip`);
fs.copyFileSync(aipSource, versionedAip);

// Copy installer scripts and documentation
const filesToCopy = [
    "Install_Plugin.bat",
    "Uninstall_Plugin.bat",
    "README.md"
];
for (const file of filesToCopy) {
    const src = path.join(ROOT_DIR, file);
    if (fs.existsSync(src)) {
        fs.copyFileSync(src, path.join(STAGING_DIR, file));
    }
}

// Copy resources folder (SVG icons, etc.)
function copyDir(src, dst) {
    if (!fs.existsSync(src)) return;
    fs.mkdirSync(dst, { recursive: true });
    for (const ent of fs.readdirSync(src, { withFileTypes: true })) {
        const s = path.join(src, ent.name);
        const d = path.join(dst, ent.name);
        if (ent.isDirectory()) {
            copyDir(s, d);
        } else {
            fs.copyFileSync(s, d);
        }
    }
}
copyDir(path.join(ROOT_DIR, "resources"), path.join(STAGING_DIR, "resources"));
copyDir(path.join(ROOT_DIR, "images"), path.join(STAGING_DIR, "images"));

// 4. Create ZIP archive in Build_DEV
fs.mkdirSync(BUILD_DEV_DIR, { recursive: true });
const zipName = `CropMarksAI_Plugin_v${version}.zip`;
const zipPath = path.join(BUILD_DEV_DIR, zipName);

if (fs.existsSync(zipPath)) {
    try { fs.unlinkSync(zipPath); } catch (e) {}
}

console.log(`[*] Packaging into ${zipPath}...`);
const psCommand = `powershell -NoProfile -Command "Compress-Archive -Path '${STAGING_DIR}\\*' -DestinationPath '${zipPath}' -Force"`;
execSync(psCommand, { stdio: "inherit" });

// Clean up staging
try { fs.rmSync(STAGING_DIR, { recursive: true, force: true }); } catch (e) {}

console.log("\n================================================================================");
console.log(` [SUCCESS] Archive created: Build_DEV/${zipName}`);
console.log(` [SUCCESS] Versioned binary: build/x64/CropMarksAI_v${version}.aip`);
console.log("================================================================================\n");
