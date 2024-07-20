# Super Swadge Land level editor

## Prerequisites:
- Tiled map editor application (www.mapeditor.org, open source, multiplatform, free to download)
- Swadge Land tileset file (swadge-land-tileset.tsx), provided in this directory

## Setup:
- Place swadge-land-editor.js into the Extensions directory of the Tiled application
-- (Windows) C:/Users/<USER>/AppData/Local/Tiled/extensions/
-- (macOS) ~/Library/Preferences/Tiled/extensions/
-- (Linux) ~/.config/tiled/extensions/

## To open an existing level file:
- Make sure swadge-land-tileset.tsx is in the same directory as the level binary (.bin) file
    - Recommended, but optional: copy the level binary (.bin) file into this directory
- In Tiled, click File->Open File or Project
- Select "Super Swadge Land map format" as the file type
- Locate and select the level binary (.bin) file you want to open in your filesystem, then open it

## To save a level file:
- In Tiled, click File->Export As...
- Select "Super Swadge Land map format" as the file type
- Save the file as normal

## Testing your level:
This method allows you to test your level without modifying the code.
You can even use this with a precompiled Swadge emulator executable to test without setting up a full development environment!

1. Make a copy of your level file 
2. Name the copy with the same name as an existing level in the assets_image directory.
3. Place your copy of the level file into the assets_image directory. Overwrite any existing file with the same name.
4. (Skip this step if you want to test using a precompiled Swadge emulator executable): Reflash the firmware to your device (no need to compile!)
5. Use Level Select to navigate to the corresponsing level
