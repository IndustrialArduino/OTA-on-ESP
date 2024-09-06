# OTA Firmware Update for ESP32 over SIM7500

This project demonstrates how to perform Over-the-Air (OTA) firmware updates on an ESP32 using the SIM7500 module. By uploading the latest firmware to a GitHub repository, the ESP32 can periodically check for updates and download them via GPRS, ensuring that the device always runs the most recent version of the firmware.

## Project Overview

The firmware update process works as follows:

1. **Current Version Check**: The ESP32 checks the `version.txt` file in the GitHub repository to determine the latest firmware version.
2. **New Firmware Detection**: If a new version is detected (i.e., different from the current firmware version running on the ESP32), the device downloads the new `.bin` firmware file.
3. **OTA Update**: The ESP32 initiates the OTA update and installs the new firmware. Once the update is complete, the ESP32 restarts and runs the new firmware.

### Branch Setup

- The **release** branch of your GitHub repository should contain:
  - The latest firmware file (`firmware_vX.X.X.bin`)
  - A `version.txt` file that stores the version number of the latest firmware.
  
### File Structure in Release Branch

```bash
/release/
  ├── firmware_v1.0.1.bin
  ├── version.txt

