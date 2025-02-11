#include <SPI.h>
#include <RH_RF95.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels

#define RF95_FREQ 915.0
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup() {
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
  display.println("LoRa RX");
  display.display();
}

void loop() {
  if (rf95.available()) {
      uint8_t len; // Variable to store received length
      if (rf95.recv(&len, 1)) { // Receive the length first (1 byte)
          if (len > RH_RF95_MAX_MESSAGE_LEN) {
              Serial.println(F("Error: Received length too large!"));
              return; // Or handle the error as appropriate
          }
          uint8_t buf[RH_RF95_MAX_MESSAGE_LEN]; // Buffer to store the message
          if (rf95.recv(buf, &len)) { // Receive the message using the received length
              Serial.print(F("Received: "));
              Serial.println((char*)buf);
              Serial.print(F("RSSI: "));
              Serial.println(rf95.lastRssi(), DEC);

            // Send an ACK - No changes needed here, it's already correct:
            uint8_t ack[] = "ACK";
            if (!rf95.send(ack, strlen((char*)ack))) { // Check if ACK send failed
                Serial.println(F("ACK send failed!"));
            } else {
                rf95.waitPacketSent();
                Serial.println(F("Sent ACK"));
            }

            // Display Received Message on OLED
            display.clearDisplay();
            display.println((char*)buf); // Display the received message
            display.display();

        } else {
            Serial.println(F("Receive failed"));
        }
    } else {
        Serial.println(F("Failed to receive message length"));
    }
}
}
