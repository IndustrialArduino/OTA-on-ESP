# Firmware Update for ESP32 over WiFi

This project demonstrates how to perform Over-the-Air (OTA) firmware updates on an ESP32 using a WiFi connection. By uploading the latest firmware to a GitHub repository, the ESP32 can periodically check for updates and download them over WiFi, ensuring that the device always runs the most recent version of the firmware.

## Project Overview

The firmware update process works as follows:

1. **Current Version Check:** The ESP32 checks the `version.txt` file in the GitHub repository to determine the latest firmware version.
2. **New Firmware Detection:** If a new version is detected (i.e., different from the current firmware version running on the ESP32), the device downloads the new `.bin` firmware file.
3. **OTA Update:** The ESP32 initiates the OTA update and installs the new firmware. Once the update is complete, the ESP32 restarts and runs the new firmware.

## Branch Setup

- The **release** branch of your GitHub repository should contain:
  - The latest firmware file (`firmware_vX.X.X.bin`)
  - A `version.txt` file that stores the version number of the latest firmware.
  
### File Structure in Release Branch

```bash
/release/
  ├── firmware_v1.0.1.bin
  ├── version.txt

```
- `version.txt`: This file should contain only the version number of the latest firmware, e.g., `1.0.1`

## Key Components

- **WiFi Connection:** The ESP32 connects to the internet via WiFi and fetches the latest firmware from the GitHub repository.
- **HTTPClient Library:** Handles the GET request to fetch the firmware version and download the binary file.
- **ArduinoOTA Libraries:** Used to handle the OTA update on the ESP32.

## Code Overview

### 1. Setup Your APN and Firmware URLs

You need to update the following variables in the code:

```bash
const char* ssid = "your-ssid";  // Your WiFi SSID
const char* password = "your-password";  // Your WiFi password
const char* version_url = "https://raw.githubusercontent.com/YourUsername/OTA-on-ESP/release/version.txt";  // Path to the version.txt file
String firmware_url = "https://raw.githubusercontent.com/YourUsername/OTA-on-ESP/release/firmware_vX.X.X.bin";  // Path to the firmware file

```
- **SSID:** Replace `your-ssid` with your WiFi SSID.
- **Password:** Replace `your-password` with your WiFi password.
- `version_url`: Update the URL to point to your GitHub repository’s `version.txt` file.
- `firmware_url`: This is automatically generated in the code, but you can manually change it if needed to point to the correct path.

### 2. Uploading New Firmware

 1.**Build your new firmware:** Compile your code and save the `.bin` file. 
 
 2.**Upload to GitHub:**
 
     - Upload the new `.bin` file to the release branch of your GitHub repository.
     - Name the file in the format `firmware_vX.X.X.bin`, where `X.X.X` is the version number (e.g., `firmware_v1.0.1.bin`).
     
 3. **Update** `version.txt`:
    
     - Update the `version.txt` file with the new version number (e.g., `1.0.1`).

### 3. Updating the Firmware Version on ESP32

Before uploading the initial code to the ESP32, ensure that the `current_version` variable in the code matches the firmware version currently on the device.

```bash
String current_version = "1.0.0";  // Update this when you deploy new firmware to the ESP32

```
After a successful OTA update, this variable must be updated in the code to reflect the new firmware version, so that future updates will work correctly.

### 3. Uploading the CA Root Certificate

Since GitHub uses HTTPS to serve files, the CA root certificate for `raw.githubusercontent.com` needs to be uploaded to the ESP. This certificate ensures that the module can securely download the firmware from GitHub.

The CA certificate is provided in the `cert.h` file in this repository. Make sure to upload this certificate to your SIM module using the appropriate AT command.

Example command to upload the CA certificate:

```bash
 client.setCACert(root_ca);

```
Replace `root_ca` with the correct path to the certificate file on your system.

## Running the OTA Update

  1. Once the code is deployed, the ESP32 will check the `version.txt` file periodically.
  2. If a new version is available, the device will download the corresponding `.bin` file and perform the update.
  3. After the update, the ESP32 restarts with the new firmware.

## Example Output

Here's what you should see in the Serial Monitor during the OTA process: 

```bash
Connecting to WiFi... OK
Checking for updates...
Making GET request to fetch version...
Current version: 1.0.0
Available version: 1.0.1
New version available. Updating...
Downloading firmware from: https://raw.githubusercontent.com/YourUsername/OTA-on-ESP/release/firmware_v1.0.1.bin
OTA Update in progress...
Firmware downloaded successfully. Rebooting.

```