#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h> 
#include "Secrets.h"  // SSID and password of the Wifi

#define CURRENT_VERSION_ADDR 2

// GitHub version and firmware URLs
String firmware_url;
const char* version_url = "https://raw.githubusercontent.com/IndustrialArduino/OTA-on-ESP/release/version.txt";
String current_version = "1.0.0"; // Current firmware version
String new_version;

// SSL/TLS certificate for the GitHub server
const char* root_ca = \

"-----BEGIN CERTIFICATE-----\n" \
"MIID0zCCArugAwIBAgIQVmcdBOpPmUxvEIFHWdJ1lDANBgkqhkiG9w0BAQwFADB7\n" \
"MQswCQYDVQQGEwJHQjEbMBkGA1UECAwSR3JlYXRlciBNYW5jaGVzdGVyMRAwDgYD\n" \
"VQQHDAdTYWxmb3JkMRowGAYDVQQKDBFDb21vZG8gQ0EgTGltaXRlZDEhMB8GA1UE\n" \
"AwwYQUFBIENlcnRpZmljYXRlIFNlcnZpY2VzMB4XDTE5MDMxMjAwMDAwMFoXDTI4\n" \
"MTIzMTIzNTk1OVowgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpOZXcgSmVyc2V5\n" \
"MRQwEgYDVQQHEwtKZXJzZXkgQ2l0eTEeMBwGA1UEChMVVGhlIFVTRVJUUlVTVCBO\n" \
"ZXR3b3JrMS4wLAYDVQQDEyVVU0VSVHJ1c3QgRUNDIENlcnRpZmljYXRpb24gQXV0\n" \
"aG9yaXR5MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAEGqxUWqn5aCPnetUkb1PGWthL\n" \
"q8bVttHmc3Gu3ZzWDGH926CJA7gFFOxXzu5dP+Ihs8731Ip54KODfi2X0GHE8Znc\n" \
"JZFjq38wo7Rw4sehM5zzvy5cU7Ffs30yf4o043l5o4HyMIHvMB8GA1UdIwQYMBaA\n" \
"FKARCiM+lvEH7OKvKe+CpX/QMKS0MB0GA1UdDgQWBBQ64QmG1M8ZwpZ2dEl23OA1\n" \
"xmNjmjAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zARBgNVHSAECjAI\n" \
"MAYGBFUdIAAwQwYDVR0fBDwwOjA4oDagNIYyaHR0cDovL2NybC5jb21vZG9jYS5j\n" \
"b20vQUFBQ2VydGlmaWNhdGVTZXJ2aWNlcy5jcmwwNAYIKwYBBQUHAQEEKDAmMCQG\n" \
"CCsGAQUFBzABhhhodHRwOi8vb2NzcC5jb21vZG9jYS5jb20wDQYJKoZIhvcNAQEM\n" \
"BQADggEBABns652JLCALBIAdGN5CmXKZFjK9Dpx1WywV4ilAbe7/ctvbq5AfjJXy\n" \
"ij0IckKJUAfiORVsAYfZFhr1wHUrxeZWEQff2Ji8fJ8ZOd+LygBkc7xGEJuTI42+\n" \
"FsMuCIKchjN0djsoTI0DQoWz4rIjQtUfenVqGtF8qmchxDM6OW1TyaLtYiKou+JV\n" \
"bJlsQ2uRl9EMC5MCHdK8aXdJ5htN978UeAOwproLtOGFfy/cQjutdAFI3tZs4RmY\n" \
"CV4Ks2dH/hzg1cEo70qLRDEmBDeNiXQ2Lu+lIg+DdEmSx/cQwgwp+7e9un/jX9Wf\n" \
"8qn0dNW44bOwgeThpWOjzOoEeJBuv/c=\n" \
"-----END CERTIFICATE-----\n";

//variabls to blink without delay:
const int led1 = 2;
const int led2 = 13;
const int led3 = 12;
const int led4 = 14;
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 1000;           // interval at which to blink (milliseconds)
int ledState = LOW;             // ledState used to set the LED

WebServer server(80); // Web server for OTA Web Updater
WiFiClientSecure client;

void setup() {
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  
  Serial.begin(115200);
  EEPROM.begin(4096);
  EEPROM.get(CURRENT_VERSION_ADDR, current_version);
  delay(100);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

    client.setCACert(root_ca); // Set the CA certificate

  // Web Updater endpoints
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  });

  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.begin();
}

void loop() {
  server.handleClient(); // Handle web updater requests
  delay(1000);

 // Check for OTA updates on GitHub once at startup
 
  if (checkForUpdate(firmware_url)) {
    performOTA(firmware_url.c_str());
  }
  delay(1000);
  
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
    digitalWrite(led3,  ledState);
    digitalWrite(led4,  ledState);
  } 
}

// Function to check for updates
bool checkForUpdate(String &firmware_url) {
  HTTPClient http;
  http.begin(version_url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    new_version = http.getString();
    new_version.trim();

    Serial.println("Current version: " + current_version);
    Serial.println("Available version: " + new_version);

    if (new_version != current_version) {
      Serial.println("New version available. Updating...");
      firmware_url = String("https://raw.githubusercontent.com/IndustrialArduino/OTA-on-ESP/release/firmware_v")+ new_version + String(".bin");
      Serial.println("Firmware URL: " + firmware_url);
      return true;
    } else {
      Serial.println("Already on the latest version");
    }
  } else {
    Serial.println("Failed to check for update, HTTP code: " + String(httpCode));
  }

  http.end();
  return false;
}

void performOTA(const char* firmware_url) {

    HTTPClient http;

    http.begin(firmware_url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      int contentLength = http.getSize();
      bool canBegin = Update.begin(contentLength);

      if (canBegin) {
        size_t written = Update.writeStream(http.getStream());

        if (written == contentLength) {
          Serial.println("Written : " + String(written) + " successfully");
        } else {
          Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
        }

        if (Update.end()) {
          Serial.println("OTA done!");
          if (Update.isFinished()) {
             Serial.println("Update successfully completed. Rebooting.");
             current_version = new_version;
             EEPROM.put(CURRENT_VERSION_ADDR, current_version);
             EEPROM.commit();
             delay(300);
             ESP.restart();
          } else {
            Serial.println("Update not finished? Something went wrong!");
          }
        } else {
          Serial.println("Error Occurred. Error #: " + String(Update.getError()));
        }
      } else {
        Serial.println("Not enough space to begin OTA");
      } 
    } else {
      Serial.println("Cannot download firmware. HTTP code: " + String(httpCode));
    }

    http.end();

}
