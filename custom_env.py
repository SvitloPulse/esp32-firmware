Import("env", "projenv")

SMARTCONFIG_KEY_LEN=16

# Install missed package
try:
    import os
    from dotenv import load_dotenv
    load_dotenv()

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
except ImportError:
    raise RuntimeError('Unable to set custom CXX flags')