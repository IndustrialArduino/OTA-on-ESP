# OTA Firmware Update for ESP32 over EC25 via SMS

This project demonstrates how to perform Over-the-Air (OTA) firmware updates on an ESP32 using the Quectel EC25 4G module, triggered via **SMS**. By uploading the latest firmware to a GitHub repository, the ESP32 can receive an SMS with the word `update`, connect to the internet via GPRS, download the new firmware, and update itself securely over HTTPS.

## Project Overview

The firmware update process works as follows:

1. **SMS Trigger**: The ESP32 monitors incoming SMS messages. When it receives a message containing the word `update`, it initiates the OTA process.
2. **Firmware Download**: The ESP32 uses the EC25 module to connect to GitHub and securely download the latest `.bin` firmware file using HTTPS.
3. **OTA Update**: The firmware binary is saved in the EC25’s RAM and streamed to the ESP32's flash memory using the Arduino `Update` class.
4. **Reboot and SMS Confirmation**: After a successful update, the ESP32 restarts and sends an SMS saying **"Firmware Updated Successfully"**.

### Branch Setup

- The **release** branch of your GitHub repository should contain:
  - The latest firmware file (`firmware.bin`)

### File Structure in Release Branch

/release/
├── firmware.bin

- `firmware.bin`: This is the compiled firmware that will be downloaded and flashed.

### Key Components

- **Quectel EC25 Module**: Used to connect to the internet via GPRS/4G and make HTTPS requests.
- **Arduino `Update` Library**: Used to write the firmware to ESP32 flash memory.
- **AT Commands**: The EC25 is controlled using serial AT commands to configure SSL, send HTTP GET, and read binary data from RAM.
- **Root CA Certificate**: The GitHub CA root certificate must be uploaded to EC25 RAM (`RAM:github_ca.pem`) to allow secure HTTPS downloads. This is stored in `github.h`.

---

## Code Overview

### 1. Setup Your APN and Phone Number

In `Configurations.h`:

```cpp
const char apn[] = "dialogbb";                 // Your GPRS APN
const char* phoneNumber = "+94769164662";      // Your number to receive confirmation SMS
