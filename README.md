# Super Magfest 2024 Swadge

This is the firmware repository for the Super Magfest 2024 Swadge. The Swadge is a wearable swag-badge powered by an ESP32-S2 and loaded with games and utilities. This project is based on the [Super Magfest 2023 Swadge](https://github.com/AEFeinstein/Super-2023-Swadge-FW).

The corresponding hardware repository for the Super Magfest 2024 Swadge [can be found here](https://github.com/AEFeinstein/Super-2024-Swadge-HW).

If you have any questions, feel free to create a Github ticket or email us at circuitboards@magfest.org.

This is living documentation, so if you notice that something is incorrect or incomplete, please fix or complete it, and submit a pull request.

## Documentation

Full documentation is [hosted online here](https://adam.feinste.in/Super-2024-Swadge-FW/). It can also be built with the command `make docs`. The documentation details all APIs and has examples for how to use them. It was written to be referenced when writing Swadge modes. It also contains sections on setup, contribution, and conduct (which are individually linked below).

The [Configuring a Development Environment guide can be found here](/docs/SETUP.md). It will help you set up a development environment.

The [Contribution Guide can be found here](/docs/CONTRIBUTING.md). It should be read before making a contribution.

Our [Code of Conduct](/docs/CODE_OF_CONDUCT.md) is simple, but worth reading and adhering to.

## Continuous Integration

This project uses Github Actions to automatically build the firmware, emulator, and documentation any time a change is pushed to `main`.

![Build Firmware and Emulator](https://github.com/AEFeinstein/Super-2024-Swadge-FW/actions/workflows/build-firmware-and-emulator.yml/badge.svg)

## Directory Structure

### Code
- [`main`](./main): The application code. This is platform-independent and is compiled into both firmware for the ESP32-S2 and an emulator for Windows or Linux.
- [`components`](./components): The hardware abstraction code for ESP32-S2. This is only compiled into firmware.
- [`emulator`](./emulator): The hardware abstraction code for the emulator. This is only compiled for Windows or Linux.

### Code Support

- [`assets`](./assets): All of the assets (images, songs, level data, etc.) used by the application
- [`tools`](./tools): Various tools which aid in development

### Configuration & Documentation
- [`.github`](./.github): Issue templates and CI workflows for GitHub
- [`.vscode`](./.vscode): Configurations for [Visual Studio Code](https://code.visualstudio.com/)
- [`docs`](./docs): Documentation 

## Troubleshooting

- Read [the documentation](https://adam.feinste.in/Super-2024-Swadge-FW/)
- Search the internet for your issue
- Ask about it either in a Github issue or the Slack channel, #circuitboards.

If your problem is solved, then the solution should be added to the documentation in the appropriate place.
