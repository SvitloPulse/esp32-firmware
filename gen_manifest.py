import json
from pathlib import Path

if __name__ == "__main__":
    with open('manifest.json', 'w') as manifest_fd:

        manifest = {}
        path = Path('.')
        for board_manifest in path.glob('.pio/**/*-manifest.json'):
            with open(board_manifest, 'r') as board_manifest_fd:
                board = json.load(board_manifest_fd)
                chip_id = board['chipId']
                board_id = board['boardId']
                boards = {}
                boards[board_id] = board
                chip_entry = dict(chipId=chip_id, boards=boards)
                if chip_id in manifest:
                    manifest[chip_id]['boards'][board_id] = board
                else:
                    manifest[chip_id] = chip_entry

        json.dump(manifest, manifest_fd, indent=2)
