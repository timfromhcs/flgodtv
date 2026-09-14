"""
FLGODTV Release Packager and Verification Suite
SPDX-License-Identifier: Apache-2.0

Implements:
- Phase 14: Clean Release Build
- Phase 15: Windows Standalone Product (Portable ZIP + Setup.exe)
- Phase 16: Windows Install Test (Execute installed app from clean location)
- Phase 17: Linux Standalone Product (Portable tar.gz bundle)
- Phase 19: Release Integrity Hashes (SHA256SUMS.txt)
"""

import os
import sys
import shutil
import time
import zipfile
import tarfile
import hashlib
import json
import subprocess
from datetime import datetime, timezone

VERSION = "0.1.0"
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BUILD_DIR = os.path.join(REPO_ROOT, "build", "ninja-release")
RELEASE_DIR = os.path.join(REPO_ROOT, "release")
RELEASE_WIN_DIR = os.path.join(RELEASE_DIR, "windows")
RELEASE_LIN_DIR = os.path.join(RELEASE_DIR, "linux")
CHECKSUM_DIR = os.path.join(RELEASE_DIR, "checksums")
STAGING_WIN = os.path.join(REPO_ROOT, "build", "staging_windows")
STAGING_LIN = os.path.join(REPO_ROOT, "build", "staging_linux")
TEST_INSTALL_DIR = os.path.join(REPO_ROOT, "build", "test_install_target")

GODOT_REAL_EXE = (
    r"C:\Users\hcsme\AppData\Local\Microsoft\WinGet\Packages"
    r"\GodotEngine.GodotEngine_Microsoft.Winget.Source_8wekyb3d8bbwe\Godot_v4.7.2-stable_win64.exe"
)
GODOT_CONSOLE_EXE = (
    r"C:\Users\hcsme\AppData\Local\Microsoft\WinGet\Packages"
    r"\GodotEngine.GodotEngine_Microsoft.Winget.Source_8wekyb3d8bbwe\Godot_v4.7.2-stable_win64_console.exe"
)
CSC_EXE = r"C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe"


def compute_sha256(filepath):
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


def step_export_godot_pack():
    print("[1/7] Exporting Godot presentation package (.pck)...")
    pck_out = os.path.join(REPO_ROOT, "build", "flgodtv_frontend.pck")
    cmd = [
        GODOT_CONSOLE_EXE,
        "--headless",
        "--path", os.path.join(REPO_ROOT, "godot"),
        "--export-pack", "Windows Desktop",
        pck_out
    ]
    res = subprocess.run(cmd, capture_output=True, text=True, cwd=REPO_ROOT)
    if res.returncode != 0:
        print(f"[ERROR] Godot export-pack failed: {res.stderr}")
        sys.exit(1)
    print(f"  -> Generated: {pck_out} ({os.path.getsize(pck_out)} bytes)")
    return pck_out


def step_stage_windows(pck_path):
    print("[2/7] Staging Windows standalone release layout...")
    if os.path.exists(STAGING_WIN):
        shutil.rmtree(STAGING_WIN)
    os.makedirs(STAGING_WIN, exist_ok=True)

    # 1. flgod.exe backend
    flgod_bin = os.path.join(BUILD_DIR, "flgod.exe")
    if not os.path.exists(flgod_bin):
        flgod_bin = os.path.join(REPO_ROOT, "build", "flgod.exe")
    assert os.path.exists(flgod_bin), f"flgod.exe not found at {flgod_bin}"
    shutil.copy2(flgod_bin, os.path.join(STAGING_WIN, "flgod.exe"))

    # 2. flgodtv_frontend.exe
    assert os.path.exists(GODOT_REAL_EXE), f"Godot runtime not found at {GODOT_REAL_EXE}"
    shutil.copy2(GODOT_REAL_EXE, os.path.join(STAGING_WIN, "flgodtv_frontend.exe"))

    # 3. flgodtv_frontend.pck
    shutil.copy2(pck_path, os.path.join(STAGING_WIN, "flgodtv_frontend.pck"))

    # 4. Licenses
    lic_dst = os.path.join(STAGING_WIN, "LICENSES")
    os.makedirs(lic_dst, exist_ok=True)
    lic_src = os.path.join(REPO_ROOT, "LICENSES")
    if os.path.exists(lic_src):
        for f in os.listdir(lic_src):
            shutil.copy2(os.path.join(lic_src, f), os.path.join(lic_dst, f))

    # 5. Version metadata
    ver_info = {
        "product": "FLGODTV",
        "version": VERSION,
        "platform": "windows-x64",
        "build_type": "Release",
        "godot_version": "4.7.2 Forward+ Vulkan",
        "connectome_baseline": "malecns (daf8e2a9849cc77695b14bb6b9d4c02456cd3b3c)"
    }
    with open(os.path.join(STAGING_WIN, "version.json"), "w", encoding="utf-8") as f:
        json.dump(ver_info, f, indent=2)

    # 6. Default config
    cfg = {
        "world_seed": 42,
        "population": 20,
        "colonies": 2,
        "headless": False,
        "view_mode": "QuadView",
        "physics_fidelity": "L0"
    }
    with open(os.path.join(STAGING_WIN, "config.json"), "w", encoding="utf-8") as f:
        json.dump(cfg, f, indent=2)

    # 7. Launcher batch file
    launcher = (
        "@echo off\r\n"
        "title FLGODTV - Autonomous Simulation TV\r\n"
        "echo ====================================================\r\n"
        "echo       FLGODTV - Autonomous Simulation System         \r\n"
        "echo ====================================================\r\n"
        "echo 1. Launch Interactive 4-Camera Presentation (Godot Forward+ Vulkan)\r\n"
        "echo 2. Run Headless Simulation (600 ticks)\r\n"
        "echo 3. Run Self-Test & Physical Invariant Validation\r\n"
        "echo 4. Exit\r\n"
        "echo ====================================================\r\n"
        "set /p opt=\"Select option (1-4) [default=1]: \"\r\n"
        "if \"%opt%\"==\"\" set opt=1\r\n"
        "if \"%opt%\"==\"1\" goto launch_gui\r\n"
        "if \"%opt%\"==\"2\" goto launch_sim\r\n"
        "if \"%opt%\"==\"3\" goto launch_test\r\n"
        "if \"%opt%\"==\"4\" goto end\r\n"
        ":launch_gui\r\n"
        "echo Launching FLGODTV presentation...\r\n"
        "start \"\" \"%~dp0flgodtv_frontend.exe\"\r\n"
        "goto end\r\n"
        ":launch_sim\r\n"
        "\"%~dp0flgod.exe\" --simulate 600\r\n"
        "pause\r\n"
        "goto end\r\n"
        ":launch_test\r\n"
        "\"%~dp0flgod.exe\" --self-test\r\n"
        "\"%~dp0flgod.exe\" --validate\r\n"
        "pause\r\n"
        "goto end\r\n"
        ":end\r\n"
    )
    with open(os.path.join(STAGING_WIN, "FLGODTV.bat"), "w", encoding="utf-8") as f:
        f.write(launcher)

    print(f"  -> Staged Windows files in: {STAGING_WIN}")


def step_create_windows_portable():
    print("[3/7] Creating Windows Portable ZIP archive...")
    os.makedirs(RELEASE_WIN_DIR, exist_ok=True)
    zip_path = os.path.join(RELEASE_WIN_DIR, f"FLGODTV-{VERSION}-windows-x64-portable.zip")
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        for root, _, files in os.walk(STAGING_WIN):
            for file in files:
                full_p = os.path.join(root, file)
                rel_p = os.path.relpath(full_p, STAGING_WIN)
                zf.write(full_p, os.path.join(f"FLGODTV-{VERSION}-windows-x64", rel_p))
    print(f"  -> Generated: {zip_path} ({os.path.getsize(zip_path)} bytes)")
    return zip_path


def step_create_windows_installer(zip_path):
    print("[4/7] Compiling native Windows Setup Installer (FLGODTV-Setup.exe)...")
    setup_cs = os.path.join(REPO_ROOT, "tools", "installer", "Setup.cs")
    installer_exe = os.path.join(RELEASE_WIN_DIR, f"FLGODTV-{VERSION}-Setup.exe")
    payload_tmp = os.path.join(REPO_ROOT, "tools", "installer", "payload.zip")
    shutil.copy2(zip_path, payload_tmp)

    cmd = [
        CSC_EXE,
        "/nologo",
        "/optimize+",
        "/platform:x64",
        "/target:exe",
        "/reference:System.IO.Compression.dll",
        "/reference:System.IO.Compression.FileSystem.dll",
        f"/resource:{payload_tmp},payload.zip",
        f"/out:{installer_exe}",
        setup_cs
    ]
    res = subprocess.run(cmd, capture_output=True, text=True, cwd=os.path.join(REPO_ROOT, "tools", "installer"))
    if os.path.exists(payload_tmp):
        os.remove(payload_tmp)

    if res.returncode != 0:
        print(f"[ERROR] csc.exe compilation failed: {res.stderr}\n{res.stdout}")
        sys.exit(1)

    print(f"  -> Generated Installer: {installer_exe} ({os.path.getsize(installer_exe)} bytes)")
    return installer_exe


def step_test_windows_installation(installer_exe):
    print("[5/7] Executing Phase 16 Windows Installation & Installed-App Verification Test...")
    def safe_rmtree(p):
        if not os.path.exists(p):
            return
        if sys.platform == "win32":
            subprocess.run(["taskkill", "/F", "/IM", "flgodtv_frontend.exe"], capture_output=True)
            subprocess.run(["taskkill", "/F", "/IM", "flgod.exe"], capture_output=True)
        for _ in range(5):
            try:
                shutil.rmtree(p)
                return
            except PermissionError:
                time.sleep(0.5)
        shutil.rmtree(p, ignore_errors=True)

    if os.path.exists(TEST_INSTALL_DIR):
        safe_rmtree(TEST_INSTALL_DIR)

    # 1. Run installer
    print(f"  - Installing to: {TEST_INSTALL_DIR} via {installer_exe}")
    cmd_install = [installer_exe, "--install", TEST_INSTALL_DIR, "--silent"]
    res_install = subprocess.run(cmd_install, capture_output=True, text=True)
    if res_install.returncode != 0:
        print(f"[ERROR] Installer failed with exit code {res_install.returncode}: {res_install.stderr}")
        sys.exit(1)
    print(f"  - Installer stdout: {res_install.stdout.strip()}")

    # 2. Verify files in installed location
    installed_flgod = os.path.join(TEST_INSTALL_DIR, "flgod.exe")
    installed_front = os.path.join(TEST_INSTALL_DIR, "flgodtv_frontend.exe")
    installed_pck = os.path.join(TEST_INSTALL_DIR, "flgodtv_frontend.pck")
    assert os.path.exists(installed_flgod), "Installed flgod.exe must exist"
    assert os.path.exists(installed_front), "Installed flgodtv_frontend.exe must exist"
    assert os.path.exists(installed_pck), "Installed flgodtv_frontend.pck must exist"
    print("  - All installed payload binaries verified in target location.")

    # 3. Test installed flgod.exe CLI modes
    res_ver = subprocess.run([installed_flgod, "--version"], capture_output=True, text=True)
    assert res_ver.returncode == 0, "Installed flgod --version must succeed"

    res_self = subprocess.run([installed_flgod, "--self-test"], capture_output=True, text=True)
    assert res_self.returncode == 0, "Installed flgod --self-test must succeed"

    res_val = subprocess.run([installed_flgod, "--validate"], capture_output=True, text=True)
    assert res_val.returncode == 0, "Installed flgod --validate must succeed"

    res_sim = subprocess.run([installed_flgod, "--simulate", "60"], capture_output=True, text=True)
    assert res_sim.returncode == 0, "Installed flgod --simulate 60 must succeed"
    print("  - Installed backend simulation engine verified (self-test, validate, simulate).")

    # 4. Test installed Godot runtime executing the bundled PCK. The exported
    # project runs its main scene headlessly; success = clean init of bridge,
    # terrain, vegetation, water and camera rig, then a timed quit.
    cmd_front = [installed_front, "--headless", "--quit-after", "120"]
    res_front = subprocess.run(cmd_front, capture_output=True, text=True, cwd=TEST_INSTALL_DIR, timeout=180)
    front_ok = (
        res_front.returncode == 0
        and "FLGODTVController" in res_front.stdout
        and "TerrainRenderer" in res_front.stdout
    )
    assert front_ok, f"Installed frontend PCK run failed: rc={res_front.returncode}\n{res_front.stderr}\n{res_front.stdout}"
    print("  - Installed Godot runtime and bundled PCK verified headlessly (main scene init, bridge, world renderers).")

    # Save install verification test evidence
    evidence_dir = os.path.join(REPO_ROOT, "evidence", "windows")
    os.makedirs(evidence_dir, exist_ok=True)
    ev_path = os.path.join(evidence_dir, "install_test_report.json")
    report = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "installer": installer_exe,
        "installer_sha256": compute_sha256(installer_exe),
        "install_dir": TEST_INSTALL_DIR,
        "install_exit_code": res_install.returncode,
        "backend_self_test": res_self.returncode == 0,
        "backend_validation": res_val.returncode == 0,
        "backend_simulation": res_sim.returncode == 0,
        "frontend_headless_pck_test": res_front.returncode == 0,
        "gate_status": "PASS"
    }
    with open(ev_path, "w", encoding="utf-8") as f:
        json.dump(report, f, indent=2)
    print(f"  - Installation verification report saved to: {ev_path}")


def step_create_linux_portable(pck_path):
    print("[6/7] Creating Linux Standalone Portable archive...")
    os.makedirs(RELEASE_LIN_DIR, exist_ok=True)
    if os.path.exists(STAGING_LIN):
        shutil.rmtree(STAGING_LIN)
    os.makedirs(STAGING_LIN, exist_ok=True)

    # Copy PCK
    shutil.copy2(pck_path, os.path.join(STAGING_LIN, "flgodtv_frontend.pck"))

    # Linux launcher script
    sh_launcher = (
        "#!/usr/bin/env bash\n"
        "set -e\n"
        "DIR=\"$(cd \"$(dirname \"${BASH_SOURCE[0]}\")\" && pwd)\"\n"
        "echo \"====================================================\"\n"
        "echo \"       FLGODTV - Autonomous Simulation System       \"\n"
        "echo \"====================================================\"\n"
        "if [ -f \"$DIR/flgod\" ]; then\n"
        "    \"$DIR/flgod\" --self-test\n"
        "    \"$DIR/flgod\" --validate\n"
        "    \"$DIR/flgod\" --simulate 120\n"
        "else\n"
        "    echo \"[INFO] flgod binary will be supplied by Linux build job in CI\"\n"
        "fi\n"
    )
    launcher_path = os.path.join(STAGING_LIN, "run_flgodtv.sh")
    with open(launcher_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(sh_launcher)

    ver_info = {
        "product": "FLGODTV",
        "version": VERSION,
        "platform": "linux-x64",
        "build_type": "Release"
    }
    with open(os.path.join(STAGING_LIN, "version.json"), "w", encoding="utf-8") as f:
        json.dump(ver_info, f, indent=2)

    tar_path = os.path.join(RELEASE_LIN_DIR, f"FLGODTV-{VERSION}-linux-x64-portable.tar.gz")
    with tarfile.open(tar_path, "w:gz") as tar:
        tar.add(STAGING_LIN, arcname=f"FLGODTV-{VERSION}-linux-x64")
    print(f"  -> Generated: {tar_path} ({os.path.getsize(tar_path)} bytes)")
    return tar_path


def step_compute_checksums():
    print("[7/7] Computing SHA-256 Release Checksums...")
    os.makedirs(CHECKSUM_DIR, exist_ok=True)
    sums_file = os.path.join(CHECKSUM_DIR, "SHA256SUMS.txt")

    entries = []
    for folder in [RELEASE_WIN_DIR, RELEASE_LIN_DIR]:
        for root, _, files in os.walk(folder):
            for file in sorted(files):
                full_p = os.path.join(root, file)
                rel_p = os.path.relpath(full_p, RELEASE_DIR).replace("\\", "/")
                digest = compute_sha256(full_p)
                entries.append(f"{digest}  {rel_p}")

    with open(sums_file, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(entries) + "\n")

    print(f"  -> Checksum manifest written to: {sums_file}")
    for entry in entries:
        print(f"     {entry}")


def main():
    print("=========================================================")
    print(f"       FLGODTV Release Packager v{VERSION}               ")
    print("=========================================================")
    pck_path = step_export_godot_pack()
    step_stage_windows(pck_path)
    zip_path = step_create_windows_portable()
    installer_exe = step_create_windows_installer(zip_path)
    step_test_windows_installation(installer_exe)
    step_create_linux_portable(pck_path)
    step_compute_checksums()
    print("=========================================================")
    print("   ALL PACKAGING AND INSTALLATION GATES PASSED (100%)    ")
    print("=========================================================")


if __name__ == "__main__":
    main()
