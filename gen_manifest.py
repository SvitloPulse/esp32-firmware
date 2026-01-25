import json
import os
from pathlib import Path

if __name__ == "__main__":
    with open('manifest.json', 'w') as manifest_fd:

        manifest = dict(version=os.environ.get('APPVEYOR_BUILD_VERSION', '0.0.0'), supportedChips=dict())
        path = Path('.')
        for firmware_manifest in path.glob('.pio/**/*-manifest.json'):
            with open(firmware_manifest, 'r') as firmware_manifest_fd:
                board_variants = json.load(firmware_manifest_fd)
                boards_entry = {}
                chip_id = board_variants[0]['chipId']

                if chip_id not in manifest['supportedChips']:
                    manifest['supportedChips'][chip_id] = dict(chipId=chip_id, boards={})

                for board_variant in board_variants:
                    manifest['supportedChips'][chip_id]['boards'][board_variant['boardId']] = board_variant
                    
        json.dump(manifest, manifest_fd, indent=2)
