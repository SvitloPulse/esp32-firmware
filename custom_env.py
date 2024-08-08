Import("env", "projenv")

env.Execute("$PYTHONEXE -m pip install python-dotenv qrcode")

# Install missed package
try:
    import os
    from dotenv import load_dotenv
    load_dotenv()
    smartconfig_key = os.environ['SB_SMARTCONFIG_KEY']
    os.system(f"qr --ascii sbpcfg:///?key={smartconfig_key}")
    with open('src/config.hpp', 'w') as f:
        f.write('#pragma once\n')
        f.write('\n')
        f.write(f'#define MD_SMARTCONFIG_KEY "{smartconfig_key}"\n')
        f.write('\n')
except ImportError:
    raise RuntimeError('Unable to set custom CXX flags')