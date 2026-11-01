#!/bin/bash

# Make the project
make -j$(nproc)

# Get each asset from cnfs_image.h
grep -P ", ///<" main/utils/filesystem/cnfs_image.h | while IFS= read -r ASSET_LINE; do

    # Get the filename
    ASSET_FILE=$(sed 's/.*\/\/\/< //' <<< "$ASSET_LINE")
    # Remove the file extension (png/wsg messes it up)
    ASSET_FILE_NO_EXT=$(echo "${ASSET_FILE%.*}.*")

    # Get the asset name and strip whitespace
    ASSET_NAME=$(sed 's/ = .*//g' <<< "$ASSET_LINE")
    ASSET_NAME=$(echo "$ASSET_NAME" | tr -d '[:space:]')

    # Search for usage
    COUNT=$(grep -r $ASSET_NAME --exclude cnfs_image.h ./main/ ./components/ | wc -l)
    if (( ! $COUNT )); then
        # Find the path to the asset
        ASSET_PATH=$(find ./assets -name $ASSET_FILE_NO_EXT)
        # Print a command to move it

        echo "mkdir -p $(dirname ./attic/$ASSET_PATH)"
        echo "mv $ASSET_PATH ./attic/$ASSET_PATH"
    fi
done

echo "WARNING: some assets are used but not directly referenced, line adjacent cutscene portraits"
