#!/bin/bash
set -e
set -x

if [ -e swadge_emulator ]; then
    BIN_SRC="swadge_emulator"
elif [ -e ../../swadge_emulator ]; then
    BIN_SRC="../../swadge_emulator"
else
    echo "Error: install file 'swadge_emulator' not found! Are you running this script from the correct directory?"
    exit 1
fi

if [ -e SwadgeEmulator.desktop ]; then
    DESKTOP_SRC="SwadgeEmulator.desktop"
elif [ -e emulator/resources/SwadgeEmulator.desktop ]; then
    DESKTOP_SRC="emulator/resources/SwadgeEmulator.desktop"
else
    echo "Error: install file 'SwadgeEmulator.desktop' not found! Are you running this script from the correct directory?"
    exit 1
fi

if [ -e icon.png ]; then
    ICON_SRC="icon.png"
elif [ -e emulator/resources/icon.png ]; then
    ICON_SRC="emulator/resources/icon.png"
else
    echo "Error: install file 'ico.png' not found! Are you running this script from the correct directory?"
fi

INSTALL_DIR="${HOME}/.local"

if [ "$#" -gt "0" ]; then
    INSTALL_DIR="${1}"
fi

echo "Installing the Swadge Emulator to: ${INSTALL_DIR}"

mkdir -p "${INSTALL_DIR}/bin" "${INSTALL_DIR}/share/icons/SwadgeEmulator" "${INSTALL_DIR}/share/applications"

cp "${BIN_SRC}" "${INSTALL_DIR}/bin/swadge_emulator"
cp "${ICON_SRC}" "${INSTALL_DIR}/share/icons/SwadgeEmulator/icon.png"
sed "s,\$ROOT,${INSTALL_DIR},g" "${DESKTOP_SRC}" > "${INSTALL_DIR}/share/applications/SwadgeEmulator.desktop"

if ! [[ ":$PATH:" == *":$INSTALL_DIR/bin:"* ]]; then
    echo "WARNING: ${INSTALL_DIR}/bin is not in your PATH!"
    echo "    If you want to be able to start the Swadge Emulator from the command-line,"
    echo "    you can run this command to update your PATH for this session,"
    echo "    or add it to a file like ~/.bash_profile:"
    echo
    echo "        export PATH=\"\$PATH:${INSTALL_DIR}/bin\""
    echo
fi

echo
echo "Installation complete!"
