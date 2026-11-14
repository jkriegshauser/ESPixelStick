import os
Import("env")

framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
# print ("FRAMEWORK_DIR:" + framework_dir)

needed_target = framework_dir + "/tools/sdk/esp32/include/hal/esp32/include/hal/i2s_ll.h"
# print ("NEEDED_TARGET:" + needed_target)

target_file = os.path.join(framework_dir, needed_target)
# print ("TARGET_FILE:" + target_file)

old_pattern = "*dsr = hw->pdm_conf.rx_sinc_dsr_16_en;"
# print ("OLD_PATTERN:" + old_pattern)

new_pattern = "*dsr = ((hw->pdm_conf.rx_sinc_dsr_16_en) ? i2s_pdm_dsr_t::I2S_PDM_DSR_16S : i2s_pdm_dsr_t::I2S_PDM_DSR_8S);"
# print ("NEW_PATTERN:" + new_pattern)

if os.path.exists(target_file):
    with open(target_file, "r", encoding="utf-8") as f:
        content = f.read()

    # Only write to disk if the old pattern is still there
    # (Prevents unnecessary recompilation on subsequent builds)
    if old_pattern in content:
        content = content.replace(old_pattern, new_pattern)
        with open(target_file, "w", encoding="utf-8") as f:
            f.write(content)
        print(f"[Pre-Build] Successfully patched {target_file}")
else:
    print(f"[Pre-Build] Warning: Could not find {target_file}")

