# tools

This folder has tools used for development. Each has it's own `README.md` if you want to read more

## [`spiffs_file_preprocessor`](./spiffs_file_preprocessor)

`spiffs_file_preprocessor` is a C program which takes assets like text, PNG images, or font files and processes them into compressed, embedded friendly formats, like WSG. It is used by the build system to process files in the `assets` folder into the `spiffs_image` folder, which is built into the firmware as a SPIFFS partition.