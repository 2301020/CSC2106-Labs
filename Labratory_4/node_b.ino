/*
*******************************************************************************
* Copyright (c) 2021 by M5Stack
*                  Equipped with M5StickC-Plus sample source code
*                             M5StickC-Plus
* Visit for more information: https://docs.m5stack.com/en/core/m5stickc_plus
*
* Describe: MQTT
* Date: 2021/11/5
*******************************************************************************
*/
#include "M5StickCPlus.h"
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

// Configure the name and password of the connected wifi and your MQTT Serve
// host.  
const char* ssid        = "Xperia_3789";
const char* password    = "u4bvtjuwn5ph95f";
const char* mqtt_server = "192.168.219.8";

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE]; 
int value = 0;
int value_client = 0;
bool isLedOn = false; 
bool isLedToggle = false; 

void setupWifi();
void callback(char* topic, byte* payload, unsigned int length);
void reConnect();

void setup() {
    M5.begin();
    M5.Lcd.setRotation(3);
    setupWifi();
    client.setServer(mqtt_server, 1883);  // Sets the server details.  
    client.setCallback(callback);  		  // Sets the message callback function.  
    pinMode(M5_BUTTON_HOME, INPUT);
    pinMode(M5_LED, OUTPUT);
}

void loop() {
    digitalWrite(M5_LED, !isLedOn);

    if(digitalRead(M5_BUTTON_HOME) == LOW){
      isLedToggle = !isLedToggle;
      // Home button "debouncer" logic
      while(digitalRead(M5_BUTTON_HOME) == LOW);
    }

    if (!client.connected()) {
        reConnect();
    }
    client.loop();  // This function is called periodically to allow clients to
                    // process incoming messages and maintain connections to the
                    // server.

    unsigned long now = millis();  // Obtain the host startup duration.  
    if (now - lastMsg > 2000) {
        lastMsg = now;

        if (isLedToggle) {
          client.publish("node_b/toggle_led", "1"); 
        } else {
          client.publish("node_b/toggle_led", "0"); 
        } 
    }
}

void setupWifi() {
    delay(10);
    M5.Lcd.printf("Connecting to %s", ssid);
    WiFi.mode(WIFI_STA);  		 // Set the mode to WiFi station mode.  
    WiFi.begin(ssid, password);  // Start Wifi connection.  

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5.Lcd.print(".");
    }
    M5.Lcd.printf("\nSuccess\n");
}

void callback(char* topic, byte* payload, unsigned int length) {
    ++value_client;
    M5.Lcd.print("Message arrived [");
    M5.Lcd.print(topic);
    M5.Lcd.print("] ");
    for (int i = 0; i < length; i++) {
        M5.Lcd.print((char)payload[i]);
    }

    if(strcmp(topic, "node_a/toggle_led") == 0){
      if (payload[0] == '1') {
          isLedOn = true;
      } else {
          isLedOn = false;
      }
    }


    M5.Lcd.println();

    if (value_client % 8 == 0) {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, 0);
    } 
}

void reConnect() {
    while (!client.connected()) {
        M5.Lcd.print("Attempting MQTT connection...");
        // Create a random client ID.  
        String clientId = "M5Stack-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect.  
        if (client.connect(clientId.c_str())) {
            M5.Lcd.printf("\nSuccess\n");
            // Once connected, publish an announcement to the topic.
            
            // client.publish("csc2006", "Hello CSC2006");
            // ... and resubscribe.  
            client.subscribe("M5Stack");
            client.subscribe("node_a/toggle_led");
        } else {
            M5.Lcd.print("failed, rc=");
            M5.Lcd.print(client.state());
            M5.Lcd.println("try again in 5 seconds");
            delay(5000);
        }
    }
}