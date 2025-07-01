#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <Update.h>
#include "cert.h"

String gsm_send_serial(String command, int delay);

#define SerialMon Serial
#define SerialAT Serial1
#define GSM_PIN ""

#define UART_BAUD 115200

#define MODEM_TX 32
#define MODEM_RX 33
#define GSM_RESET 21

// Your GPRS credentials
const char apn[] = "dialogbb";
const char gprsUser[] = "";

String current_version = "1.0.0";
String new_version;
String version_url = "https://raw.githubusercontent.com/IndustrialArduino/OTA-on-ESP/release/version.txt";

String firmware_url;

//variabls to blink without delay:
const int led1 = 2;
const int led2 = 12;
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 1000;           // interval at which to blink (milliseconds)
int ledState = LOW;             // ledState used to set the LED


void setup() {
  // Set console baud rate
  Serial.begin(115200);
  delay(10);
  SerialAT.begin(UART_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);

  delay(2000);
  pinMode(GSM_RESET, OUTPUT);
  digitalWrite(GSM_RESET, HIGH);  // RS-485
  delay(2000);

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);

  Init();
  connectToGPRS();
  connectTohttps();

}

void loop() {
  if (checkForUpdate(firmware_url)) {
    performOTA(firmware_url);
  }
  delay(1000);

  //add the code need to run and this is an example program
  //loop to blink without delay
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    ledState = not(ledState);

    // set the LED with the ledState of the variable:
    digitalWrite(led1,  ledState);
    digitalWrite(led2,  ledState);
  }

}

void Init(void) {                        // Connecting with the network and GPRS
  delay(5000);
  gsm_send_serial("AT+CFUN=1", 10000);
  gsm_send_serial("AT+CPIN?", 10000);
  gsm_send_serial("AT+CSQ", 1000);
  gsm_send_serial("AT+CREG?", 1000);
  gsm_send_serial("AT+COPS?", 1000);
  gsm_send_serial("AT+CGATT?", 1000);
  gsm_send_serial("AT+CPSI?", 500);
  String cmd = "AT+CGDCONT=1,\"IP\",\"" + String(apn) + "\"";
  gsm_send_serial(cmd, 1000);
  gsm_send_serial("AT+CGACT=1,1", 1000);
  gsm_send_serial("AT+CGATT?", 1000);
  gsm_send_serial("AT+CGPADDR=1", 500);

}

void connectToGPRS(void) {
  gsm_send_serial("AT+CGATT=1", 1000);
  String cmd = "AT+CGDCONT=1,\"IP\",\"" + String(apn) + "\"";
  gsm_send_serial(cmd, 1000);
  gsm_send_serial("AT+CGACT=1,1", 1000);
  gsm_send_serial("AT+CGPADDR=1", 500);
}

void connectTohttps(void) {
  int cert_length = root_ca.length();
  String ca_cert = "AT+QFUPL=\"RAM:github_ca.pem\"," + String(cert_length) + ",100";
  gsm_send_serial(ca_cert, 1000);
  delay(1000);
  gsm_send_serial(root_ca, 1000);
  delay(1000);
  gsm_send_serial("AT+QHTTPCFG=\"contextid\",1", 1000);
  gsm_send_serial("AT+QHTTPCFG=\"responseheader\",1", 1000);
  gsm_send_serial("AT+QHTTPCFG=\"sslctxid\",1", 1000);
  gsm_send_serial("AT+QSSLCFG=\"sslversion\",1,4", 1000);
  gsm_send_serial("AT+QSSLCFG=\"ciphersuite\",1,0xC02F", 1000);
  gsm_send_serial("AT+QSSLCFG=\"seclevel\",1,1", 1000);
  gsm_send_serial("AT+QSSLCFG=\"sni\",1,1", 1000);
  gsm_send_serial("AT+QSSLCFG=\"cacert\",1,\"RAM:github_ca.pem\"", 1000);
}


// Check the version of the latest firmware uploaded to GitHub
bool checkForUpdate(String &firmware_url) {
  Serial.println("Making GET request securely...");

  // Ensure PDP context is active (optional but recommended)
  gsm_send_serial("AT+QIACT?", 1000);
  delay(100);
  gsm_send_serial("AT+QIACT=1", 2000);  // Reactivate if needed
  delay(300);
  
  // Clear SerialAT buffer
  while (SerialAT.available()) SerialAT.read();

  // Step 1: Prepare the URL
  gsm_send_serial("AT+QHTTPURL=" + String(version_url.length()) + ",80", 1000);
  delay(200);
  gsm_send_serial(version_url, 2000);

  bool got200 = false;
  int contentLen = -1;

  // Flush again
  while (SerialAT.available()) SerialAT.read();

  // Step 2: Send HTTP GET
  Serial.println("Send ->: AT+QHTTPGET=80");
  SerialAT.println("AT+QHTTPGET=80");

  unsigned long startTime = millis();
  const unsigned long httpTimeout = 8000;  // Wait max 8s
  String qhttpgetLine = "";

  while (millis() - startTime < httpTimeout) {
    if (SerialAT.available()) {
      String line = SerialAT.readStringUntil('\n');
      line.trim();

      if (line.length() == 0) continue;
      Serial.println("[Modem Line] " + line);

      if (line.startsWith("+QHTTPGET:")) {
        qhttpgetLine = line;
        break;
      }

      if (line.indexOf("+QIURC: \"pdpdeact\"") >= 0) {
        Serial.println("[OTA] PDP deactivated! Reconnecting...");
        gsm_send_serial("AT+QIACT=1", 3000);
        return false;
      }
    }
  }

  if (qhttpgetLine.length() == 0) {
    Serial.println("[OTA] HTTP GET response not received.");
    return false;
  }

  // Step 3: Parse +QHTTPGET: 0,<status>,<len>
  int idx1 = qhttpgetLine.indexOf(',');
  int idx2 = qhttpgetLine.indexOf(',', idx1 + 1);
  if (idx1 == -1 || idx2 == -1) {
    Serial.println("[OTA] Malformed +QHTTPGET response");
    return false;
  }

  int statusCode = qhttpgetLine.substring(idx1 + 1, idx2).toInt();
  contentLen = qhttpgetLine.substring(idx2 + 1).toInt();

  if (statusCode == 200 && contentLen > 0) {
    got200 = true;
    Serial.println("[OTA] HTTP 200 OK. Content length: " + String(contentLen));
  } else {
    Serial.println("[OTA] HTTP GET failed. Status: " + String(statusCode));
    return false;
  }

  delay(300);  // Give modem time to buffer

  // Step 4: Issue QHTTPREAD
  Serial.println("Send ->: AT+QHTTPREAD=" + String(contentLen));
  SerialAT.println("AT+QHTTPREAD=" + String(contentLen));

  // Step 5: Wait for CONNECT
  bool gotConnect = false;
  unsigned long waitStart = millis();
  while (millis() - waitStart < 3000) {
    if (SerialAT.available()) {
      String line = SerialAT.readStringUntil('\n');
      line.trim();
      Serial.println("[Modem Line] " + line);
      if (line.startsWith("CONNECT")) {
        gotConnect = true;
        break;
      }
    }
  }

  if (!gotConnect) {
    Serial.println("[OTA] Failed to get CONNECT, aborting");
    return false;
  }

  // Step 6: Skip headers and extract the version
  String version = "";
  bool foundEmptyLine = false;
  unsigned long readTimeout = millis() + 5000;

  while (millis() < readTimeout) {
    if (SerialAT.available()) {
      String line = SerialAT.readStringUntil('\n');
      line.trim();

      if (line.length() == 0) {
        foundEmptyLine = true;  // Found end of headers
        continue;
      }

      if (foundEmptyLine) {
        version = line;  // First non-empty line after headers
        break;
      }

      Serial.println("[Header] " + line);  // Optional logging

      readTimeout = millis() + 1000;  // Extend timeout while reading
    }
  }

  version.trim();
  Serial.println("Extracted version: " + version);

  if (version.length() == 0) {
    Serial.println("[OTA] Failed to extract version string.");
    return false;
  }

  // Set global new_version
  new_version = version;

  // Step 7: Compare and set firmware URL
  if (new_version != current_version) {
    Serial.println("New version available. Updating...");
    firmware_url = "https://raw.githubusercontent.com/IndustrialArduino/OTA-on-ESP/release/firmware_v" + new_version + ".bin";
    Serial.println("Firmware URL: " + firmware_url);
    return true;
  } else {
    Serial.println("Already on latest version.");
    return false;
  }
}

void performOTA(String firmware_url) {

  gsm_send_serial("AT+QHTTPURL=" + String(firmware_url.length()) + ",80", 1000);
  delay(100);
  gsm_send_serial(firmware_url, 2000);

  gsm_send_serial("AT+QHTTPGET=80", 1000);
  Serial.println("[OTA] Waiting for +QHTTPGET response...");


  long contentLength = -1;
  unsigned long timeout = millis();
  while (millis() - timeout < 5000) {
    if (SerialAT.available()) {
      String line = SerialAT.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;
      Serial.println("[Modem Line] " + line);
      if (line.startsWith("+QHTTPGET:")) {
        int firstComma = line.indexOf(',');
        int secondComma = line.indexOf(',', firstComma + 1);
        if (firstComma != -1 && secondComma != -1) {
          String lenStr = line.substring(secondComma + 1);
          contentLength = lenStr.toInt();
          Serial.print("[OTA] Content-Length: ");
          Serial.println(contentLength);
        }
      }
      if (line == "OK") break;
    }
    delay(10);
  }

  Serial.println("[OTA] HTTPS GET sent");

  // Save response to RAM file
  gsm_send_serial("AT+QHTTPREADFILE=\"RAM:firmware.bin\",80", 1000);

  // Wait for final confirmation and avoid overlap
  unsigned long readfileTimeout = millis();
  while (millis() - readfileTimeout < 5000) {
    if (SerialAT.available()) {
      String line = SerialAT.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;
      Serial.println("[READFILE] " + line);
      if (line.startsWith("+QHTTPREADFILE:")) break;
    }
    delay(10);
  }

  // Clear SerialAT buffer
  while (SerialAT.available()) SerialAT.read();

  // Send QFLST directly
  SerialAT.println("AT+QFLST=\"RAM:firmware.bin\"");

  long ramFileSize = 0;
  timeout = millis();
  while (millis() - timeout < 5000) {
    if (SerialAT.available()) {
      String line = SerialAT.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;

      Serial.println("[OTA Raw] " + line);

      // Find +QFLST line
      if (line.startsWith("+QFLST:")) {
        int commaIdx = line.lastIndexOf(',');
        if (commaIdx != -1) {
          String sizeStr = line.substring(commaIdx + 1);
          sizeStr.trim();
          ramFileSize = sizeStr.toInt();
          break;
        }
      }
    }
    delay(10);
  }

  Serial.println("[OTA] File size: " + String(ramFileSize));

  if (ramFileSize <= 0) {
    Serial.println("[OTA] ERROR: Invalid file size.");
    return;
  }



  int headerSize = ramFileSize - contentLength;
  if (headerSize <= 0 || headerSize > ramFileSize) {
    Serial.println("[OTA] Invalid header size!");
    return;
  }
  Serial.println("[OTA] Header size: " + String(headerSize));

  // Clear SerialAT buffer before command
  while (SerialAT.available()) SerialAT.read();

  // Send QFOPEN directly
  SerialAT.println("AT+QFOPEN=\"RAM:firmware.bin\",0");

  int fileHandle = -1;
  unsigned long handleTimeout = millis();

  while (millis() - handleTimeout < 5000) {
    if (SerialAT.available()) {
      String line = SerialAT.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;

      Serial.println("[OTA Raw] " + line);

      if (line.startsWith("+QFOPEN:")) {
        String handleStr = line.substring(line.indexOf(":") + 1);
        handleStr.trim();
        fileHandle = handleStr.toInt();
        break;
      }
    }
    delay(10);
  }

  Serial.println("[OTA] File handle: " + String(fileHandle));

  if (fileHandle <= 0) {
    Serial.println("[OTA] ERROR: Invalid file handle.");
    return;
  }

  // Seek to payload
  gsm_send_serial("AT+QFSEEK=" + String(fileHandle) + "," + String(headerSize) + ",0", 1000);
  delay(300);
  // Step 7: Begin OTA
  if (!Update.begin(contentLength)) {
    Serial.println("[OTA] Update.begin failed");
    return;
  }

  Serial.println("[OTA] Start writing...");


  size_t chunkSize = 1024;
  size_t totalWritten = 0;
  uint8_t buffer[1024];

  while (totalWritten < contentLength) {
    size_t bytesToRead = min(chunkSize, (size_t)(contentLength - totalWritten));
    SerialAT.println("AT+QFREAD=" + String(fileHandle) + "," + String(bytesToRead));

    // Wait for CONNECT (start of binary data)
    bool gotConnect = false;
    unsigned long startWait = millis();
    while (millis() - startWait < 2000) {
      if (SerialAT.available()) {
        String line = SerialAT.readStringUntil('\n');
        line.trim();
        if (line.startsWith("CONNECT")) {
          gotConnect = true;
          break;
        }
      }
      delay(1);
    }
    if (!gotConnect) {
      Serial.println("[OTA] Failed to get CONNECT");
      Update.abort();
      return;
    }

    // Read exactly bytesToRead bytes of binary data
    size_t readCount = 0;
    unsigned long lastReadTime = millis();
    while (readCount < bytesToRead && millis() - lastReadTime < 3000) {
      if (SerialAT.available()) {
        buffer[readCount++] = (uint8_t)SerialAT.read();
        lastReadTime = millis();
      } else {
        delay(1);
      }
    }
    if (readCount != bytesToRead) {
      Serial.println("[OTA] Incomplete read from modem");
      Update.abort();
      return;
    }

    // After reading data, wait for the final OK
    bool gotOK = false;
    startWait = millis();
    while (millis() - startWait < 2000) {
      if (SerialAT.available()) {
        String line = SerialAT.readStringUntil('\n');
        line.trim();
        if (line == "OK") {
          gotOK = true;
          break;
        }
      }
      delay(1);
    }
    if (!gotOK) {
      Serial.println("[OTA] Did not receive final OK after data");
      Update.abort();
      return;
    }

    // Write to flash
    size_t written = Update.write(buffer, readCount);
    if (written != readCount) {
      Serial.println("[OTA] Flash write mismatch");
      Update.abort();
      return;
    }

    totalWritten += written;
    Serial.printf("\r[OTA] Progress: %u / %u bytes", (unsigned)totalWritten, (unsigned)contentLength);
  }

  Serial.println("\n[OTA] Firmware write complete.");

  // Close the file
  SerialAT.println("AT+QFCLOSE=" + String(fileHandle));
  delay(500);

  // Finalize OTA update
  if (Update.end()) {
    Serial.println("[OTA] Update successful!");
    if (Update.isFinished()) {
      Serial.println("[OTA] Rebooting...");
      delay(300);
      ESP.restart();
    } else {
      Serial.println("[OTA] Update not finished!");
    }
  } else {
    Serial.println("[OTA] Update failed with error: " + String(Update.getError()));
  }
}

String gsm_send_serial(String command, int timeout) {
  String buff_resp = "";
  Serial.println("Send ->: " + command);
  SerialAT.println(command);
  unsigned long startMillis = millis();

  while (millis() - startMillis < timeout) {
    while (SerialAT.available()) {
      char c = SerialAT.read();
      buff_resp += c;
    }
    delay(10); // Small delay to allow for incoming data to accumulate
  }

  Serial.println("Response ->: " + buff_resp);
  return buff_resp;
}
