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

### Code Overview

## 1. Setup Your APN and Phone Number

In `Configurations.h`:

```bash
const char apn[] = "dialogbb";                 // Your GPRS APN
const char* phoneNumber = "+94761111111";      // Your number to receive confirmation SMS

```

## 2. Set Firmware URL
In your main code:

```bash
String firmware_url = "https://raw.githubusercontent.com/IndustrialArduino/OTA-on-ESP/release/firmware.bin";

```
Make sure the URL points to your GitHub repository and the correct branch.

### Uploading New Firmware

## 1.Build your firmware
     - **Compile your code using PlatformIO or Arduino IDE and generate the .bin firmware file.

## 2.Upload to GitHub
    - ** Push the new firmware.bin to the release branch of your GitHub repository.

## 3.Trigger OTA by SMS
    - ** Send an SMS with the word update to the ESP32 SIM number.
    - ** The device will initiate the OTA update automatically.

### Root CA Certificate Upload
- **The EC25 must verify GitHub’s HTTPS certificate using the GitHub CA root certificate.
- **The certificate is included in github.h:

```bash
String root_ca  =\
-----BEGIN CERTIFICATE-----
... GitHub Root CA ...
-----END CERTIFICATE-----
;

```

In code, the certificate is uploaded using:

```bash
AT+QFUPL="RAM:github_ca.pem",<length>,100
<root_ca contents>

```

This certificate is then registered using:

```bash
AT+QSSLCFG="cacert",1,"RAM:github_ca.pem"

```

### Running the OTA Update
##  1.The ESP32 listens for incoming SMS messages.

##  2.When it receives update, it:
     - **Connects to the internet
     - **Configures SSL
     - **Downloads the firmware to RAM:firmware.bin
     - **Streams it chunk-by-chunk to ESP32 flash
    - ** Closes the file and restarts
     
##  3.Sends an SMS: Firmware Updated Successfully

### Example Output
Here's what you should see in the Serial Monitor during the OTA process:

```bash

[SMS] Received: update
[SMS Action] OTA UPDATE
[OTA] Waiting for +QHTTPGET response...
[OTA] Content-Length: 112345
[OTA] HTTPS GET sent
[OTA] File size: 113123
[OTA] Header size: 778
[OTA] Start writing...
[OTA] Progress: 112345 / 112345 bytes
[OTA] Firmware write complete.
[OTA] Update successful!
[SMS] Firmware Updated Successfully
[OTA] Rebooting...

```


