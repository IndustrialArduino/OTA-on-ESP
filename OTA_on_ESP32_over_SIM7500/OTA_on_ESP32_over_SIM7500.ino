#include <Arduino.h>
#include <Wire.h>
#include <Update.h>

#define TINY_GSM_MODEM_SIM7500
// Increase RX buffer
#define TINY_GSM_RX_BUFFER 1030
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include "SSLClient.h"
#include "cert.h"

#define TINY_GSM_TEST_GPRS          true
#define TINY_GSM_TEST_TCP           true

#define SerialMon Serial
#define SerialAT Serial1
#define GSM_PIN ""

// Your GPRS credentials
const char apn[] = "dialogbb";
const char user[] = "";
const char pass[] = "";

const char server[] = "raw.githubusercontent.com";
const int port = 443;

#define CURRENT_VERSION_ADDR 2
#define UART_BAUD 115200

#define MODEM_TX 32
#define MODEM_RX 33
#define GSM_RESET 21

String current_version = "1.0.0";
String new_version;
const char version_url[] = "/IndustrialArduino/OTA-on-ESP/release/version.txt";

String firmware_url;

//variabls to blink without delay:
const int led1 = 2;
const int led2 = 12;
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 1000;           // interval at which to blink (milliseconds)
int ledState = LOW;             // ledState used to set the LED

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

// HTTPS Transport
TinyGsmClient base_client(modem, 0);
SSLClient secure_layer(&base_client);
HttpClient client = HttpClient(secure_layer, server, port);

void setup() {
  // Set console baud rate
  Serial.begin(115200);
  delay(10);

  SerialAT.begin(UART_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);

  delay(2000);
  pinMode(GSM_RESET, OUTPUT);
  digitalWrite(GSM_RESET, HIGH);  // RS-485
  delay(10);
  SerialMon.println("Wait...");

  //Add CA Certificate
  secure_layer.setCACert(root_ca);

  Serial.println("Initializing modem...");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  Serial.print("Modem: ");
  Serial.println(modemInfo);

  Serial.print("Waiting for network...");
  if (!modem.waitForNetwork())
  {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" OK");

  Serial.print("Connecting to ");
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass))
  {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" OK");

}

void loop() {

  if (checkForUpdate(firmware_url)) {
    performOTA(firmware_url.c_str());
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

//Check the version of the latest firmware which has uploaded to Github
bool checkForUpdate(String &firmware_url) {
  Serial.println("Making GET request securely...");
  client.get(version_url);
  int status_code = client.responseStatusCode();
  delay(1000);
  String response_body = client.responseBody();
  delay(1000);

  Serial.print("Status code: ");
  Serial.println(status_code);
  Serial.print("Response: ");
  Serial.println(response_body);

  response_body.trim();
  response_body.replace("\r", "");  // Remove carriage returns
  response_body.replace("\n", "");  // Remove newlines

  // Extract the version number from the response
  new_version = response_body;

  Serial.println("Current version: " + current_version);
  Serial.println("Available version: " + new_version);
  client.stop();

  if (new_version != current_version) {
    Serial.println("New version available. Updating...");
    firmware_url = String("/IndustrialArduino/OTA-on-ESP/release/firmware_v") + new_version + ".bin";
    Serial.println("Firmware URL: " + firmware_url);
    return true;

  } else {
    Serial.println("Already on the latest version");
  }

  return false;
}

//Update the latest firmware which has uploaded to Github
void performOTA(const char* firmware_url) {
  // Initialize HTTP

  Serial.println("Making GET request securely...");
  client.get(firmware_url);
  int status_code = client.responseStatusCode();
  delay(1000);
  long contentlength = client.contentLength();
  delay(1000);

  Serial.print("Contentlength: ");
  Serial.println(contentlength);

  if (status_code == 200) {

    if (contentlength <= 0) {
      SerialMon.println("Failed to get content length");
      client.stop();
      return;
    }

    // Begin OTA update
    bool canBegin = Update.begin(contentlength);
    size_t written ;
    long totalBytesWritten = 0;
    uint8_t buffer[1024];
    int bytesRead ;
    long contentlength_real = contentlength;

    if (canBegin) {
      while (contentlength  > 0) {
        bytesRead = client.readBytes(buffer, sizeof(buffer));
        if (bytesRead > 0) {
          written = Update.write(buffer, bytesRead);
          if (written != bytesRead) {
            Serial.println("Error: written bytes do not match read bytes");
            Update.abort();
            return;
          }
          totalBytesWritten += written;  // Track total bytes written
          contentlength -= bytesRead;  // Reduce remaining content length
        } else {
          Serial.println("Error: Timeout or no data received");
          break;
        }
      }

      if (totalBytesWritten == contentlength_real) {
        Serial.println("Written : " + String(totalBytesWritten) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentlength_real) + ". Retry?");
      }

      if (Update.end()) {
        SerialMon.println("OTA done!");
        if (Update.isFinished()) {
          SerialMon.println("Update successfully completed. Rebooting.");
          delay(300);
          ESP.restart();
        } else {
          SerialMon.println("Update not finished? Something went wrong!");
        }
      } else {
        SerialMon.println("Error Occurred. Error #: " + String(Update.getError()));
      }
    } else {
      Serial.println("Not enough space to begin OTA");
    }
  } else {
    Serial.println("Cannot download firmware. HTTP code: " + String(status_code));
  }

  client.stop();
}
