# Swadge Emulator

The Swadge Emulator is a Windows or Linux program which is really more of a simulator, but the name 'emulator' stuck.

The Emulator does not require the ESP-IDF, and instead the files required for compilation were copied from the ESP-IDF to [`./idf-inc/`](./idf-inc/) and stripped down. If function prototypes, struct definitions, or any other headers in ESP-IDF change, the changes must be manually copied here.

The [`./src/idf`](./src/idf/) folder reimplements a few parts of the IDF. For the most part the Emulator strives to reimplement components rather than parts of the IDF.

The [`./src/components`](./src/components) folder reimplements this project's [`../components`](../components/) folder, which has the hardware interfaces for ESP32-S2. The Emulator is compiled with the header files from the main [`../components`](../components/) folder for consistency.
