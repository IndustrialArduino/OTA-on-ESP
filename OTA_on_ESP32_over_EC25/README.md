# OTA Firmware Update for ESP32 over EC25 (HTTPS + AT Commands)

This project demonstrates how to perform Over-the-Air (OTA) firmware updates on an ESP32 using the **Quectel EC25** LTE modem. The firmware is hosted on GitHub, and the ESP32 downloads and installs updates using HTTPS via AT commands. The process ensures secure firmware delivery and automatic version checking.

## 📅 Project Overview

The OTA update process works as follows:

1. **Current Version Check**: ESP32 uses the EC25 modem to make an HTTPS GET request to a `version.txt` file on GitHub.
2. **New Firmware Detection**: If the version in the file differs from the ESP32's current firmware, an OTA update is initiated.
3. **Firmware Download**: ESP32 downloads the `.bin` file to the EC25's RAM file system.
4. **OTA Flash**: The firmware is read chunk-by-chunk from EC25 RAM and flashed to the ESP32.
5. **Restart & Confirmation**: After successful flashing, the ESP32 reboots and runs the new firmware.

---

## 📚 Branch Setup

* The `release` branch of your GitHub repository should contain:

  * The latest firmware binary file: `firmware_vX.X.X.bin`
  * A `version.txt` file containing the latest firmware version number.

### Example File Structure:

```
/release/
├── firmware_v1.0.1.bin
├── version.txt
```

* `version.txt`: Contains a string like `1.0.1`

---

## 🔍 Key Components

* **Quectel EC25 Modem**: Used for GPRS/LTE and HTTPS requests via AT commands.
* **ESP32 Arduino**: Host microcontroller that controls the EC25 and handles OTA using the `Update` class.
* **Root CA Certificate**: Required for HTTPS communication with GitHub. Must be uploaded to EC25 RAM using AT commands.
* **GitHub Raw URLs**: Used to fetch both `version.txt` and the binary firmware file.

---

## 📝 Code Configuration

### 1. Set APN and Version URL

```cpp
const char apn[] = "dialogbb";  // Your SIM provider APN
String current_version = "1.0.0";  // Current version of firmware on device
String version_url = "https://raw.githubusercontent.com/IndustrialArduino/OTA-on-ESP/release/version.txt";
```

### 2. Firmware File URL

The firmware URL is dynamically built:

```cpp
firmware_url = "https://raw.githubusercontent.com/IndustrialArduino/OTA-on-ESP/release/firmware_v" + new_version + ".bin";
```

### 3. CA Root Certificate Upload

Upload the GitHub CA certificate using AT commands:

```cpp
AT+QFUPL="RAM:github_ca.pem",<length>,100
<root_ca contents>

AT+QSSLCFG="cacert",1,"RAM:github_ca.pem"
```

---

## 🎓 Uploading New Firmware to GitHub

### 1. Build New Firmware

* Compile your updated code and generate the `.bin` file.

### 2. Push to GitHub

* Upload the new file as `firmware_vX.X.X.bin` to the `release` branch.
* Update `version.txt` to the new version (e.g., `1.0.1`).

---

## 🔄 Running the OTA Update

The ESP32 will:

* Connect to the mobile network
* Download `version.txt` via HTTPS
* Compare with its current firmware version
* If a new version is found:

  * Download the new `.bin` firmware to EC25's RAM as `firmware.bin`
  * Use `AT+QFOPEN`, `AT+QFREAD`, and `Update.write()` to stream firmware to ESP32
  * Close the file, complete the update, and reboot

### OTA Steps (in bullet form):

* Connects to the internet
* Configures SSL
* Downloads the firmware to `RAM:firmware.bin`
* Streams it chunk-by-chunk to ESP32 flash
* Closes the file and restarts
* Sends confirmation SMS (optional)

---

## ✅ Example Serial Output

```bash
Making GET request securely...
[OTA] HTTP 200 OK. Content length: 112345
[OTA] Waiting for +QHTTPGET response...
[OTA] File size: 113123
[OTA] Header size: 778
[OTA] Start writing...
[OTA] Progress: 112345 / 112345 bytes
[OTA] Firmware write complete.
[OTA] Update successful!
[OTA] Rebooting...
```

---

## ⚠️ Notes

* You must ensure that the EC25 module has active PDP context and the correct APN.
* If using SSL, always upload and register the root CA before initiating HTTPS requests.
* `current_version` should match the firmware flashed to the ESP32.

---

## 🔍 Useful AT Commands

```bash
AT+QFUPL="RAM:github_ca.pem",<length>,100
AT+QSSLCFG="cacert",1,"RAM:github_ca.pem"
AT+QHTTPURL=<length>,80
AT+QHTTPGET=80
AT+QHTTPREADFILE="RAM:firmware.bin",80
AT+QFLST="RAM:firmware.bin"
AT+QFOPEN="RAM:firmware.bin",0
AT+QFREAD=<handle>,<size>
AT+QFCLOSE=<handle>
```

---


