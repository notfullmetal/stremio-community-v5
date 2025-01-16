/****************************************************
 * build_checksums.js
 *
 * Usage:
 *   node build_checksums.js "<OpenSSLBinPath>" "<GitTag>" "<ShellVersion>" "<ServerVersion>"
 *
 * Example:
 *   node build_checksums.js "C:\\Program Files\\OpenSSL-Win64\\bin" "5.0.0-beta.7" "5.0.7" "4.20.11"
 *
 * This script:
 *   1) Validates CLI args: OPENSSL_BIN, GIT_TAG, SHELL_VERSION, SERVER_VERSION
 *   2) Locates and verifies openssl.exe
 *   3) Computes sha256 checksums for Stremio.<ShellVersion>-x64.exe, -x86.exe, and server.js
 *   4) Updates version-details.json and version.json for the built-in auto-updater.
 *   5) Signs version-details.json, base64-encodes the signature, injects signature into version.json.
 *   6) Cleans up ephemeral signature files.
 *   7) Also updates:
 *        - utils/chocolatey/stremio.nuspec  (the <version> tag)
 *        - utils/chocolatey/tools/chocolateyinstall.ps1 (the download URL(s))
 *        - utils/scoop/stremio-desktop-v5.json (the "version", "url", and "hash" fields for x86/x64)
 *   8) Generates .sha256 files for the x86/x64 executables (so Scoop autoupdate can consume them).
 *
 ****************************************************/

const fs = require("fs");
const path = require("path");
const { execFileSync } = require("child_process");

// Parse CLI arguments
const [,, OPENSSL_BIN, GIT_TAG, SHELL_VERSION, SERVER_VERSION] = process.argv;

(async function main() {
    // 1) Validate args
    if (!OPENSSL_BIN || !GIT_TAG || !SHELL_VERSION || !SERVER_VERSION) {
        console.error("Usage: node build_checksums.js <OpenSSLBinPath> <GitTag> <ShellVersion> <ServerVersion>");
        console.error('Example: node build_checksums.js "C:\\Program Files\\OpenSSL-Win64\\bin" "5.0.0-beta.7" "5.0.7" 4.20.11');
        process.exit(1);
    }

    // 2) Verify openssl.exe
    const opensslExe = path.join(OPENSSL_BIN, "openssl.exe");
    if (!fs.existsSync(opensslExe)) {
        console.error("ERROR: Cannot find openssl.exe in:", opensslExe);
        process.exit(1);
    }

    console.log("Using OpenSSL at:", OPENSSL_BIN);
    console.log("Git Tag:", GIT_TAG);
    console.log("Shell Version:", SHELL_VERSION);
    console.log("server.js Version:", SERVER_VERSION);
    console.log();

    // 3) Build paths
    const scriptDir = path.dirname(__filename);
    const projectRoot = path.resolve(scriptDir, "..");

    // The local EXE file names; adapt if your naming convention differs
    const exeNameX64 = `Stremio ${SHELL_VERSION}-x64.exe`;
    const exeNameX86 = `Stremio ${SHELL_VERSION}-x86.exe`;

    const EXE_PATH_x64 = path.join(projectRoot, "utils", exeNameX64);
    const EXE_PATH_x86 = path.join(projectRoot, "utils", exeNameX86);

    // Where is server.js? Adjust if needed
    const SERVERJS_PATH = path.join(projectRoot, "utils", "windows", "server.js");

    // version details
    const VERSION_DETAILS_PATH = path.join(projectRoot, "version", "version-details.json");
    const VERSION_JSON_PATH = path.join(projectRoot, "version", "version.json");
    const PRIVATE_KEY = path.join(projectRoot, "private_key.pem");

    // Paths to your choco and scoop files:
    const CHOCO_NUSPEC_PATH = path.join(projectRoot, "utils", "chocolatey", "stremio.nuspec");
    const CHOCO_INSTALL_PS1_PATH = path.join(projectRoot, "utils", "chocolatey", "tools", "chocolateyinstall.ps1");
    const SCOOP_MANIFEST_PATH = path.join(projectRoot, "utils", "scoop", "stremio-desktop-v5.json");

    // 4) Generate SHA-256 for the .exe and server.js
    checkFileExists(EXE_PATH_x64, "Stremio x64 .exe");
    checkFileExists(EXE_PATH_x86, "Stremio x86 .exe");
    const exeHash_x64 = computeSha256(opensslExe, EXE_PATH_x64);
    const exeHash_x86 = computeSha256(opensslExe, EXE_PATH_x86);

    checkFileExists(SERVERJS_PATH, "server.js");
    const serverHash = computeSha256(opensslExe, SERVERJS_PATH);

    console.log("EXE sha256 x64 =", exeHash_x64);
    console.log("EXE sha256 x86 =", exeHash_x86);
    console.log("server.js sha256 =", serverHash);
    console.log();

    // 5) Update version-details.json
    checkFileExists(VERSION_DETAILS_PATH, "version-details.json");
    let versionDetails;
    try {
        versionDetails = JSON.parse(fs.readFileSync(VERSION_DETAILS_PATH, "utf8"));
    } catch (err) {
        console.error("ERROR: Unable to parse version-details.json:", err.message);
        process.exit(1);
    }

    // Ensure structure
    if (!versionDetails.files) {
        console.error("ERROR: version-details.json missing property 'files'");
        process.exit(1);
    }

    // Update version-details.json
    versionDetails.shellVersion = SHELL_VERSION;

    // windows-x64
    if (!versionDetails.files["windows-x64"]) versionDetails.files["windows-x64"] = {};
    versionDetails.files["windows-x64"].url = `https://github.com/Zaarrg/stremio-desktop-v5/releases/download/${GIT_TAG}/Stremio.${SHELL_VERSION}-x64.exe`;
    versionDetails.files["windows-x64"].checksum = exeHash_x64;

    // windows-x86
    if (!versionDetails.files["windows-x86"]) versionDetails.files["windows-x86"] = {};
    versionDetails.files["windows-x86"].url = `https://github.com/Zaarrg/stremio-desktop-v5/releases/download/${GIT_TAG}/Stremio.${SHELL_VERSION}-x86.exe`;
    versionDetails.files["windows-x86"].checksum = exeHash_x86;

    // server.js
    if (!versionDetails.files["server.js"]) versionDetails.files["server.js"] = {};
    versionDetails.files["server.js"].url = `https://dl.strem.io/server/${SERVER_VERSION}/desktop/server.js`;
    versionDetails.files["server.js"].checksum = serverHash;

    // Save updated version-details.json
    try {
        fs.writeFileSync(VERSION_DETAILS_PATH, JSON.stringify(versionDetails, null, 2), "utf8");
    } catch (e) {
        console.error("ERROR: Failed writing version-details.json:", e.message);
        process.exit(1);
    }

    // 6) Sign version-details.json & base64-encode
    checkFileExists(PRIVATE_KEY, "private_key.pem");
    process.chdir(path.join(projectRoot, "version"));

    const sigFile = path.join(process.cwd(), "version-details.json.sig");
    const sigB64  = path.join(process.cwd(), "version-details.json.sig.b64");
    if (fs.existsSync(sigFile)) fs.unlinkSync(sigFile);
    if (fs.existsSync(sigB64))  fs.unlinkSync(sigB64);

    console.log(`Signing version-details.json with ${PRIVATE_KEY}...`);
    try {
        execFileSync(opensslExe, [
            "dgst",
            "-sha256",
            "-sign",
            PRIVATE_KEY,
            "-out",
            "version-details.json.sig",
            "version-details.json"
        ], { stdio: "inherit" });
    } catch (err) {
        console.error("ERROR: Signing failed:", err.message);
        process.exit(1);
    }

    try {
        execFileSync(opensslExe, [
            "base64",
            "-in",
            "version-details.json.sig",
            "-out",
            "version-details.json.sig.b64"
        ], { stdio: "inherit" });
    } catch (err) {
        console.error("ERROR: Base64 encoding failed:", err.message);
        process.exit(1);
    }

    if (!fs.existsSync(sigB64)) {
        console.error("ERROR: Could not create signature file:", sigB64);
        process.exit(1);
    }
    process.chdir(projectRoot);

    // 7) Insert signature into version.json
    checkFileExists(VERSION_JSON_PATH, "version.json");
    console.log(`Updating signature in "${VERSION_JSON_PATH}"...`);
    let signatureB64;
    try {
        signatureB64 = fs.readFileSync(sigB64, "utf8").replace(/\r?\n/g, "");
    } catch (err) {
        console.error("ERROR: Unable to read version-details.json.sig.b64:", err.message);
        process.exit(1);
    }

    let versionJson;
    try {
        versionJson = JSON.parse(fs.readFileSync(VERSION_JSON_PATH, "utf8"));
    } catch (err) {
        console.error("ERROR: Unable to parse version.json:", err.message);
        process.exit(1);
    }

    versionJson.signature = signatureB64;
    try {
        fs.writeFileSync(VERSION_JSON_PATH, JSON.stringify(versionJson, null, 2), "utf8");
    } catch (err) {
        console.error("ERROR: Unable to write version.json:", err.message);
        process.exit(1);
    }

    // Cleanup ephemeral signature files
    try {
        if (fs.existsSync(sigFile)) fs.unlinkSync(sigFile);
        if (fs.existsSync(sigB64)) fs.unlinkSync(sigB64);
    } catch (cleanupErr) {
        console.error("WARNING: Could not remove signature files:", cleanupErr.message);
    }

    console.log("\nSuccess! Checksums and signature updated. Now updating Scoop & Chocolatey files...\n");

    // 8) Update stremio.nuspec <version>
    updateStremioNuspec(CHOCO_NUSPEC_PATH, SHELL_VERSION);

    // 9) Update chocolateyinstall.ps1 URLs
    updateChocolateyInstall(CHOCO_INSTALL_PS1_PATH, GIT_TAG, SHELL_VERSION, exeHash_x64, exeHash_x86);

    // 10) Update the Scoop manifest (stremio-desktop-v5.json)
    updateScoopManifest(SCOOP_MANIFEST_PATH, GIT_TAG, SHELL_VERSION, exeHash_x64, exeHash_x86);

    // 11) Generate .sha256 files for each EXE in /utils.
    //     This is required if you keep "hash.url" in your Scoop "autoupdate" section.
    generateSha256FilesForScoop(
        projectRoot,
        GIT_TAG,
        SHELL_VERSION,
        { x64: exeHash_x64, x86: exeHash_x86 }
    );

    console.log("\nAll updates complete. You may now commit/push these changes and attach the .exe and .sha256 files to your release.\n");
    process.exit(0);

})().catch(err => {
    console.error("Unexpected error:", err);
    process.exit(1);
});


/************************************************************
 * Helper Functions
 ************************************************************/

function checkFileExists(filePath, label) {
    if (!fs.existsSync(filePath)) {
        console.error(`ERROR: ${label} file not found at: ${filePath}`);
        process.exit(1);
    }
}

// runs "openssl dgst -sha256 <file>" and returns the hex string
function computeSha256(opensslExe, filePath) {
    try {
        const output = execFileSync(opensslExe, ["dgst", "-sha256", filePath], { encoding: "utf8" });
        // Typically "SHA256(file)= <hexhash>"
        const match = output.match(/=.\s*([0-9a-fA-F]+)/);
        if (!match) {
            console.error("ERROR: Unexpected openssl dgst output for", filePath, "-", output);
            process.exit(1);
        }
        return match[1].toLowerCase();
    } catch (err) {
        console.error(`ERROR: openssl dgst failed for ${filePath}:`, err.message);
        process.exit(1);
    }
}

// 8) Update the stremio.nuspec with the new <version>
function updateStremioNuspec(nuspecPath, newVersion) {
    checkFileExists(nuspecPath, "stremio.nuspec");
    let content = fs.readFileSync(nuspecPath, "utf8");

    // Replace the <version>...</version> with newVersion
    content = content.replace(
        /<version>[^<]+<\/version>/,
        `<version>${newVersion}</version>`
    );

    fs.writeFileSync(nuspecPath, content, "utf8");
    console.log(`Updated stremio.nuspec <version> to ${newVersion}`);
}

// 9) Update chocolateyinstall.ps1 with new GIT_TAG + SHELL_VERSION in the URLs
function updateChocolateyInstall(ps1Path, gitTag, newVersion, hash64, hash86) {
    checkFileExists(ps1Path, "chocolateyinstall.ps1");
    let content = fs.readFileSync(ps1Path, "utf8");

    // We'll build a single block that covers both if/else in one go.
    const newBlock = `
if ([Environment]::Is64BitOperatingSystem) {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/${gitTag}/Stremio.${newVersion}-x64.exe'
    $packageArgs['checksum']     = '${hash64}'
    $packageArgs['checksumType'] = 'sha256'
} else {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/${gitTag}/Stremio.${newVersion}-x86.exe'
    $packageArgs['checksum']     = '${hash86}'
    $packageArgs['checksumType'] = 'sha256'
}
`;

    // Regex to capture the entire if...else block (non-greedy):
    // This should match from "if ([Environment]::Is64BitOperatingSystem) {"
    // until the closing "}" of the else block.
    const pattern = /if\s*\(\[Environment\]::Is64BitOperatingSystem\)\s*\{[\s\S]+?\}\s*else\s*\{[\s\S]+?\}/m;

    // Replace the entire old block with newBlock
    content = content.replace(pattern, newBlock.trim());

    fs.writeFileSync(ps1Path, content, "utf8");
    console.log(`Updated chocolateyinstall.ps1 with new version=${newVersion}, hash64=${hash64}, hash86=${hash86}`);
}

// 10) Update the Scoop manifest stremio-desktop-v5.json
function updateScoopManifest(scoopPath, gitTag, newVersion, hash64, hash86) {
    checkFileExists(scoopPath, "stremio-desktop-v5.json");
    let scoopJson;

    try {
        const raw = fs.readFileSync(scoopPath, "utf8");
        scoopJson = JSON.parse(raw);
    } catch (err) {
        console.error("ERROR: Unable to parse scoop manifest JSON:", err.message);
        process.exit(1);
    }

    // "version": "5.0.7"
    scoopJson.version = newVersion;

    if (!scoopJson.architecture || !scoopJson.architecture["64bit"] || !scoopJson.architecture["32bit"]) {
        console.error("ERROR: scoop manifest missing architecture stanzas");
        process.exit(1);
    }

    // Update 64bit url + hash
    scoopJson.architecture["64bit"].url = `https://github.com/Zaarrg/stremio-desktop-v5/releases/download/${gitTag}/Stremio.${newVersion}-x64.exe`;
    scoopJson.architecture["64bit"].hash = hash64;

    // Update 32bit url + hash
    scoopJson.architecture["32bit"].url = `https://github.com/Zaarrg/stremio-desktop-v5/releases/download/${gitTag}/Stremio.${newVersion}-x86.exe`;
    scoopJson.architecture["32bit"].hash = hash86;

    // If you want to rely on .sha256 files for autoupdate, keep the `hash.url` lines in "autoupdate".
    // If you prefer not to upload .sha256 files, remove or modify that.
    // Just note that removing them will break the default Scoop auto-updater checks.

    // Save updates
    try {
        fs.writeFileSync(scoopPath, JSON.stringify(scoopJson, null, 2), "utf8");
    } catch (err) {
        console.error("ERROR: Failed writing scoop manifest:", err.message);
        process.exit(1);
    }

    console.log(`Updated Scoop manifest with version=${newVersion}, x64Hash=${hash64}, x86Hash=${hash86}`);
}

// 11) Create .sha256 files for each EXE in /utils so Scoop "autoupdate" can fetch them
function generateSha256FilesForScoop(projectRoot, gitTag, shellVersion, hashes) {
    // We'll create: Stremio.<shellVersion>-x64.exe.sha256 and Stremio.<shellVersion>-x86.exe.sha256
    // in /utils, each containing the hex digest plus a newline.

    const outDir = path.join(projectRoot, "utils");
    if (!fs.existsSync(outDir)) {
        fs.mkdirSync(outDir, { recursive: true });
    }

    // x64
    const x64filename = `Stremio.${shellVersion}-x64.exe.sha256`;
    const x64path = path.join(outDir, x64filename);
    fs.writeFileSync(x64path, hashes.x64 + "\n", "utf8");

    // x86
    const x86filename = `Stremio.${shellVersion}-x86.exe.sha256`;
    const x86path = path.join(outDir, x86filename);
    fs.writeFileSync(x86path, hashes.x86 + "\n", "utf8");

    console.log(
        `\nGenerated .sha256 files:\n  ${x64filename} -> ${hashes.x64}\n  ${x86filename} -> ${hashes.x86}\n\n` +
        "Remember to upload these *.sha256 files alongside your EXEs in the GitHub release if you want Scoop autoupdate to work."
    );
}
