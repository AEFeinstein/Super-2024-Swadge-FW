#!/usr/bin/env python3

import sys
from rme_view import view
from rme_model import model
from rme_controller import controller
import argparse
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(
                    prog=Path(__file__).name,
                    description='Tile map editor for Swadge games')

    defaultAssetsPath = '../../assets/magtroid/'
    defaultImagesPath = './imgs/'
    parser.add_argument('-a', '--swadge-assets-path', help=f'Path to the Swadge assets directory. The default value is "{defaultAssetsPath}".', default=defaultAssetsPath)
    parser.add_argument('-i', '--editor-images-path', help=f'Path to the Editor images directory. The default value is "{defaultImagesPath}".', default=defaultImagesPath)
    parser.add_argument('-f', '--file', help='The map file to load (optional)', default=None)
    args = parser.parse_args()

    v: view = view(args.swadge_assets_path, args.editor_images_path)
    m: model = model(32, 16)
    c: controller = controller()

    c.setModel(m)
    v.setController(c)
    v.setModel(m)
    m.setView(v)

    if len(sys.argv) >= 2:
        v.loadFile(open(args.file, 'rb'))

    v.redraw()

    v.mainloop()


if __name__ == '__main__':
    main()
