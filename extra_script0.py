Import("env")
import os, shutil

build_dir = env.subst("$BUILD_DIR")
project_dir = env.subst("$PROJECT_DIR")

bootloader = os.path.join(build_dir, "bootloader.bin")
partitions = os.path.join(build_dir, "partitions.bin")
app = os.path.join(build_dir, "firmware.bin")

# Try to detect FS image
fs_image = None
for candidate in ["littlefs.bin", "spiffs.bin"]:
    path = os.path.join(build_dir, candidate)
    if os.path.exists(path):
        fs_image = path
        break

merged = os.path.join(build_dir, "HamClockFirmware.bin")

dest_dir = os.path.join(project_dir, "firmware")
dest_file = os.path.join(dest_dir, "HamClockFirmware.bin")

def merge_bins(source, target, env):
    print(">>> Merging BIN files into single HamClockFirmware.bin...")

    cmd = (
        f"esptool.py --chip esp32 merge_bin -o {merged} "
        f"--flash_mode dio --flash_freq 80m --flash_size 4MB "
        f"0x1000 {bootloader} "
        f"0x8000 {partitions} "
        f"0x10000 {app}"
    )

    # If FS image exists, append it (⚠️ adjust offset to your no_ota.csv)
    if fs_image:
        cmd += f" 0x290000 {fs_image}"
        print(f">>> Including filesystem image: {fs_image}")
    else:
        print(">>> No filesystem image found (skipping).")

    env.Execute(cmd)

    os.makedirs(dest_dir, exist_ok=True)
    shutil.copyfile(merged, dest_file)
    print(f">>> Copied merged firmware to {dest_file}")

env.AddPostAction("buildprog", merge_bins)
