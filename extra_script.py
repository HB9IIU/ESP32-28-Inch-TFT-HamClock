# extra_script.py

Import("env")
import os, shutil

# ========================
# CONFIGURATION FOR YOUR TABLE
# ========================
FIRMWARE_NAME = "HamClockFirmware"   # final filename (no path)
FLASH_SIZE    = "4MB"                # your flash size
FS_OFFSET_HEX = "0x210000"           # from your CSV: spiffs at 0x210000
# ========================

build_dir   = env.subst("$BUILD_DIR")          # .pio/build/<env>
project_dir = env.subst("$PROJECT_DIR")

bootloader  = os.path.join(build_dir, "bootloader.bin")
partitions  = os.path.join(build_dir, "partitions.bin")
app         = os.path.join(build_dir, "firmware.bin")

merged      = os.path.join(build_dir, f"{FIRMWARE_NAME}.bin")
dest_dir    = os.path.join(project_dir, "firmware")
dest_file   = os.path.join(dest_dir, f"{FIRMWARE_NAME}.bin")

print(">>> extra_script.py loaded", flush=True)

def pre_buildfs(source, target, env):
    print(">>> Building filesystem image (pio run -t buildfs)…", flush=True)
    env.Execute("$PYTHONEXE -m platformio run -t buildfs")

def post_merge(source, target, env):
    print(f">>> Preparing merge for {FIRMWARE_NAME}.bin …", flush=True)

    # Clean previous merged release files only
    for f in (merged, dest_file):
        try:
            if os.path.exists(f):
                os.remove(f)
                print(f"    removed old: {f}", flush=True)
        except Exception as e:
            print(f"    could not remove {f}: {e}", flush=True)

    # Detect FS image (SPIFFS or LittleFS)
    fs_image = None
    for candidate in ("spiffs.bin", "littlefs.bin"):
        p = os.path.join(build_dir, candidate)
        if os.path.exists(p):
            fs_image = p
            print(f">>> Found FS image: {fs_image}", flush=True)
            break
    if not fs_image:
        print(">>> No FS image found (continuing without FS).", flush=True)

    # Ensure the build artifacts exist
    missing = [p for p in (bootloader, partitions, app) if not os.path.exists(p)]
    if missing:
        print("!!! Missing required artifacts:", flush=True)
        for m in missing:
            print("    -", m, flush=True)
        print("!!! Tip: run `pio run -v` once to confirm outputs, or run `pio run -t clean && pio run`.", flush=True)
        return

    # Build esptool merge command (sparse merged image)
    cmd = (
        f"esptool.py --chip esp32 merge_bin -o {merged} "
        f"--flash_mode dio --flash_freq 80m --flash_size {FLASH_SIZE} "
        f"0x1000 {bootloader} "
        f"0x8000 {partitions} "
        f"0x10000 {app}"
    )
    if fs_image:
        cmd += f" {FS_OFFSET_HEX} {fs_image}"
        print(f">>> Including filesystem at {FS_OFFSET_HEX}", flush=True)

    print(">>> Running:", cmd, flush=True)
    env.Execute(cmd)

    os.makedirs(dest_dir, exist_ok=True)
    shutil.copyfile(merged, dest_file)
    print(f">>> Copied merged firmware to {dest_file}", flush=True)

# Order: buildfs -> (normal PIO build) -> merge
env.AddPreAction("buildprog", pre_buildfs)
env.AddPostAction("buildprog", post_merge)
