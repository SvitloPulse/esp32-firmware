Import("env", "projenv")

SMARTCONFIG_KEY_LEN=16

# Install missed package
env.Execute("$PYTHONEXE -m pip install python-dotenv")

try:
    import os
    import json
    import hashlib
    from dotenv import load_dotenv
    load_dotenv()

    print("Current CLI targets", COMMAND_LINE_TARGETS)
    print("Current Build targets", BUILD_TARGETS)

    svitlobot_api = os.environ.get('SB_SVITLOBOT_API')
    svitlobot_key = os.environ.get('SB_SVITLOBOT_KEY')
    smartconfig_key = os.environ.get('SB_SMARTCONFIG_KEY')
    wifi_ssid = os.environ.get('SB_WIFI_SSID')
    wifi_password = os.environ.get('SB_WIFI_PWD')

    if smartconfig_key and len(smartconfig_key) != SMARTCONFIG_KEY_LEN:
        raise RuntimeError('Invalid SmartConfig key. Must be 16 characters long')
    with open('src/config.hpp', 'w') as f:
        f.write('#pragma once\n')
        f.write('\n')
        f.write(f'#define SB_SVITLOBOT_API "{svitlobot_api}"\n') if svitlobot_api else None
        f.write(f'#define SB_SVITLOBOT_KEY "{svitlobot_key}"\n') if svitlobot_key else None
        f.write(f'#define SB_SMARTCONFIG_KEY "{smartconfig_key}"\n') if smartconfig_key else None
        f.write(f'#define SB_WIFI_SSID "{wifi_ssid}"\n') if wifi_ssid else None
        f.write(f'#define SB_WIFI_PWD "{wifi_password}"\n') if wifi_password else None
        f.write('\n')
    platform = env.PioPlatform()
    board = env.BoardConfig()
    mcu = board.get("build.mcu", "esp32")
    UPLOADER=os.path.join(
            platform.get_package_dir("tool-esptoolpy") or "", "esptool.py")
    offset_image_pairs = []
    for image in env.get("FLASH_EXTRA_IMAGES", []):
        offset_image_pairs += [image[0], env.subst(image[1])]
    merged_bin = f"$BUILD_DIR/{mcu}-$PROGNAME-merged.bin"
    env.AddPostAction(
        "$BUILD_DIR/${PROGNAME}.bin",
        env.VerboseAction(" ".join([
            "$PYTHONEXE", UPLOADER, 
            "--chip", mcu, 
            "merge_bin", 
            "-o", merged_bin,
            "--flash_mode", "${__get_board_flash_mode(__env__)}",
            "--flash_size", board.get("upload.flash_size", "detect"),
            "$ESP32_APP_OFFSET",
            "$BUILD_DIR/${PROGNAME}.bin",
        ] + offset_image_pairs), "Merging firmware files to $BUILD_DIR/$PROGNAME-merged.bin")
    )

    def generate_manifest_json(source, target, env):
        extra_images = env.get("FLASH_EXTRA_IMAGES", [])
        bootloader_def = extra_images[0]
        with open(env.subst(merged_bin), 'rb') as f:
            data = f.read()
            sha256 = hashlib.sha256(data).hexdigest()

        manifest = dict(mcu=mcu, files=[
            dict(name=env.subst(f"{mcu}-$PROGNAME-merged.bin"), offset=bootloader_def[0], sha256=sha256),
        ])
        with open(env.subst(f"$BUILD_DIR/{mcu}-manifest.json"), 'w') as f:
            json.dump(manifest, f, indent=4)

    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", generate_manifest_json)

except ImportError:
    raise RuntimeError('Unable to set custom CXX flags')