// LoRa 9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example LoRa9x_RX

#include <SPI.h>
#include <RH_RF95.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2
#define node_id "B"
 
// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

#define MAX_RETRIES 3  // Maximum number of retries
#define ACK_TIMEOUT 500 // Timeout for ACK (milliseconds)

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

void(* resetFunc) (void) = 0; //declare reset function at address 0

void setup() 
{
  Serial.begin(9600);
  delay(100);

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
      Serial.println(F("LoRa radio init failed"));
      delay(1000); // Important: Keep this delay for debugging.
  }
  Serial.println(F("LoRa radio init OK!"));

  // Defaults after init are 915.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println(F("setFrequency failed"));
    while (1);
  }

  Serial.print(F("Set Freq to: ")); Serial.println(RF95_FREQ);
  rf95.setTxPower(13, false);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println(F("SSD1306 allocation failed"));
      for (;;) { delay(1000); }
  }
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.println("LoRa TX");
  display.display();
}
 
int16_t packetnum = 0;  // packet counter, we increment per transmission
 
void loop() {
  Serial.println(F("Sending to rf95_rx"));

  uint8_t data[32];
  snprintf((char*)data, sizeof(data), "Hello World %d from %s", packetnum++, node_id);
  uint8_t len = strlen((char*)data); // Calculate message length

  Serial.print(F("Sending message: ")); Serial.println((char*)data);
  Serial.print(F("Message length: ")); Serial.println(len); // Send the length too!

  bool ackReceived = false;
  int16_t sentRSSI = 0;
    for (int retry = 0; retry < MAX_RETRIES; retry++) {
        Serial.print(F("Retry: ")); Serial.println(retry + 1);

        if (!rf95.send(data, len)) {
            Serial.println(F("Send failed"));
            display.clearDisplay();
            display.println(F("Send failed"));
            display.display();
            delay(1000);
            continue;
        }

        Serial.println(F("Wait for packet to complete..."));
        rf95.waitPacketSent();
        sentRSSI = rf95.lastRssi(); // Get RSSI of sent packet
        Serial.print(F("Sent RSSI: ")); Serial.println(sentRSSI, DEC);

        uint8_t ackBuf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t ackLen = sizeof(ackBuf);
        if (rf95.waitAvailableTimeout(ACK_TIMEOUT)) {
            if (rf95.recv(ackBuf, &ackLen)) {
                if (strncmp((char*)ackBuf, "ACK", 3) == 0) {
                    Serial.println(F("ACK received!"));
                    ackReceived = true;
                    break;
                } else {
                    Serial.print(F("Unexpected ACK: ")); Serial.println((char*)ackBuf);
                }
            } else {
                Serial.println(F("ACK receive failed"));
            }
        } else {
            Serial.println(F("No ACK received"));
        }
        delay(100);
    }

    if (ackReceived) {
        Serial.println(F("Message sent successfully with ACK"));
        display.clearDisplay();
        display.print(F("Sent: ")); display.println(packetnum-1);  //Display packet number
        display.print(F("RSSI: ")); display.println(sentRSSI); //Display RSSI
        display.display();

    } else {
        Serial.println(F("Message sending failed after multiple retries"));
        display.clearDisplay();
        display.println(F("Message failed"));
        display.display();
    }
    delay(10000);
}
