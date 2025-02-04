#include <BLEDevice.h>
#include <BLEServer.h>
//#include <BLEUtils.h>
//#include <BLE2902.h>
#include <M5StickCPlus.h>

//#include <Wire.h>

//Default Temperature in Celsius
#define temperatureCelsius

//change to unique BLE server name
#define bleServerName "CSC2106-BLE#01"

float tempC;
float tempF;
float vBatt = 5.0;  // initial value
bool isLedOn = false; 


// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 15000;   // update refresh every 15sec

bool deviceConnected = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "01234567-0123-4567-89ab-0123456789ab"

// Temperature Characteristic and Descriptor
#ifdef temperatureCelsius
  BLECharacteristic imuTemperatureCelsiusCharacteristics("01234567-0123-4567-89ab-0123456789cd", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor imuTemperatureCelsiusDescriptor(BLEUUID((uint16_t)0x2902));
#else
  BLECharacteristic imuTemperatureFahrenheitCharacteristics("01234567-0123-4567-89ab-0123456789de", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor imuTemperatureFahrenheitDescriptor(BLEUUID((uint16_t)0x2902));
#endif

// Battery Voltage Characteristic and Descriptor
BLECharacteristic axpVoltageCharacteristics("01234567-0123-4567-89ab-0123456789ef", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor axpVoltageDescriptor(BLEUUID((uint16_t)0x2903));

// Battery Led Characteristic and Descriptor
BLECharacteristic axpLedCharacteristics("01234567-0123-4567-89ab-0123456789fg", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor axpLedDescriptor(BLEUUID((uint16_t)0x2904));

//Setup callbacks onConnect and onDisconnect
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("MyServerCallbacks::Connected...");
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("MyServerCallbacks::Disconnected...");
    BLEDevice::getAdvertising()->start();  // Restart advertising after disconnect
  }
};

// Led callback
class MyLedCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    // Get the value written to the characteristic
    std::string value = pCharacteristic->getValue();
    if (value == "abc") {
      isLedOn = !isLedOn;
      digitalWrite(M5_LED, isLedOn);
    } 
  }
};

void setup() {
  // Start serial communication 
  Serial.begin(115200);

  // put your setup code here, to run once:
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("BLE Server", 0);

  pinMode(M5_BUTTON_HOME, INPUT);
  pinMode(M5_LED, OUTPUT);

  // Create the BLE Device
  BLEDevice::init(bleServerName);

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *bleService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristics and Create a BLE Descriptor
  // Temperature
  #ifdef temperatureCelsius
    bleService->addCharacteristic(&imuTemperatureCelsiusCharacteristics);
    imuTemperatureCelsiusDescriptor.setValue("IMU Temperature(C)");
    imuTemperatureCelsiusCharacteristics.addDescriptor(&imuTemperatureCelsiusDescriptor);
  #else
    bleService->addCharacteristic(&imuTemperatureFahrenheitCharacteristics);
    imuTemperatureFahrenheitDescriptor.setValue("IMU Temperature(F)");
    imuTemperatureFahrenheitCharacteristics.addDescriptor(&imuTemperatureFahrenheitDescriptor);
  #endif  

  // Battery
  bleService->addCharacteristic(&axpVoltageCharacteristics);
  axpVoltageDescriptor.setValue("AXP Battery(V)");
  axpVoltageCharacteristics.addDescriptor(&axpVoltageDescriptor); 

  // Led
  bleService->addCharacteristic(&axpLedCharacteristics);
  axpLedDescriptor.setValue("Led State");
  axpLedCharacteristics.addDescriptor(&axpLedDescriptor); 
  axpLedCharacteristics.setCallbacks(new MyLedCharacteristicCallbacks());
    
  // Start the service
  bleService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

// Read temp celcius function
float readTemperature(){
  float t;
  M5.IMU.getTempData(&t);
  t = (t-32.0)*5.0/9.0;
  return t;
}

void loop() {
  digitalWrite(M5_LED, isLedOn);
  if (deviceConnected) {
    if(digitalRead(M5_BUTTON_HOME) == LOW){
      isLedOn = !isLedOn;
      
      // Read temperature as Celsius (the default)
      tempC = readTemperature();

      // Fahrenheit
      tempF = 1.8*tempC + 32;

      vBatt = M5.Axp.GetBatVoltage();
  
      //Notify temperature reading from IMU
      #ifdef temperatureCelsius
        static char temperatureCTemp[6];
        dtostrf(tempC, 6, 2, temperatureCTemp);
        //Set temperature Characteristic value and notify connected client
        imuTemperatureCelsiusCharacteristics.setValue(temperatureCTemp);
        imuTemperatureCelsiusCharacteristics.notify();
        Serial.print("Temperature = ");
        Serial.print(tempC);
        Serial.print(" C");

        M5.Lcd.setCursor(0, 20, 2);
        M5.Lcd.print("Temperature = ");
        M5.Lcd.print(tempC);
        M5.Lcd.println(" C");
      #else
        static char temperatureFTemp[6];
        dtostrf(tempF, 6, 2, temperatureFTemp);
        //Set temperature Characteristic value and notify connected client
        imuTemperatureFahrenheitCharacteristics.setValue(temperatureFTemp);
        imuTemperatureFahrenheitCharacteristics.notify();
        Serial.print("Temperature = ");
        Serial.print(tempF);
        Serial.print(" F");

        M5.Lcd.setCursor(0, 20, 2);
        M5.Lcd.print("Temperature = ");
        M5.Lcd.print(tempF);
        M5.Lcd.println(" F");
      #endif
      
      //Notify battery status reading from AXP192
      static char voltageBatt[6];
      dtostrf(vBatt, 6, 2, voltageBatt);
      //Set voltage Characteristic value and notify connected client
      axpVoltageCharacteristics.setValue(voltageBatt);
      axpVoltageCharacteristics.notify();   
      Serial.print(" - Battery Voltage = ");
      Serial.print(vBatt);
      Serial.println(" V");

      M5.Lcd.setCursor(0, 40, 2);
      M5.Lcd.print("Battery Votage = ");
      M5.Lcd.print(vBatt);
      M5.Lcd.println(" V");

      // Set Led state
      static char ledBoolStr[2];
      if (isLedOn) {
        strcpy(ledBoolStr, "0");
      } else {
        strcpy(ledBoolStr, "1");
      } 
      axpLedCharacteristics.setValue(ledBoolStr);
      axpLedCharacteristics.notify();

      M5.Lcd.setCursor(0, 60, 2);
      M5.Lcd.print("Led State: ");
      if (isLedOn) {
        M5.Lcd.println("0");
      } else {
        M5.Lcd.println("1");
      } 

      // Home button "debouncer" logic
      while(digitalRead(M5_BUTTON_HOME) == LOW);
    }

    if ((millis() - lastTime) > timerDelay) {
      // Read temperature as Celsius (the default)
      // if (random(2)>0)
      //   tempC += random(10)/100.0;
      // else
      //   tempC -= random(10)/100.0;
      tempC = readTemperature();

      // Fahrenheit
      tempF = 1.8*tempC + 32;

      // Battery voltage
      // if (vBatt<1.0)
      //   vBatt = 5.0;
      // else
      //   vBatt -= 0.01;

      vBatt = M5.Axp.GetBatVoltage();
  
      //Notify temperature reading from IMU
      #ifdef temperatureCelsius
        static char temperatureCTemp[6];
        dtostrf(tempC, 6, 2, temperatureCTemp);
        //Set temperature Characteristic value and notify connected client
        imuTemperatureCelsiusCharacteristics.setValue(temperatureCTemp);
        imuTemperatureCelsiusCharacteristics.notify();
        Serial.print("Temperature = ");
        Serial.print(tempC);
        Serial.print(" C");

        M5.Lcd.setCursor(0, 20, 2);
        M5.Lcd.print("Temperature = ");
        M5.Lcd.print(tempC);
        M5.Lcd.println(" C");
      #else
        static char temperatureFTemp[6];
        dtostrf(tempF, 6, 2, temperatureFTemp);
        //Set temperature Characteristic value and notify connected client
        imuTemperatureFahrenheitCharacteristics.setValue(temperatureFTemp);
        imuTemperatureFahrenheitCharacteristics.notify();
        Serial.print("Temperature = ");
        Serial.print(tempF);
        Serial.print(" F");

        M5.Lcd.setCursor(0, 20, 2);
        M5.Lcd.print("Temperature = ");
        M5.Lcd.print(tempF);
        M5.Lcd.println(" F");
      #endif
      
      //Notify battery status reading from AXP192
      static char voltageBatt[6];
      dtostrf(vBatt, 6, 2, voltageBatt);
      //Set voltage Characteristic value and notify connected client
      axpVoltageCharacteristics.setValue(voltageBatt);
      axpVoltageCharacteristics.notify();   
      Serial.print(" - Battery Voltage = ");
      Serial.print(vBatt);
      Serial.println(" V");

      M5.Lcd.setCursor(0, 40, 2);
      M5.Lcd.print("Battery Votage = ");
      M5.Lcd.print(vBatt);
      M5.Lcd.println(" V");

      // // Set Led state
      // if (isLedOn) {
      //   axpLedCharacteristics.setValue("0");
      //   axpLedCharacteristics.notify();
      // } else {
      //   axpLedCharacteristics.setValue("1");
      //   axpLedCharacteristics.notify();
      // }

      // if (isLedOn) {
      //   M5.Lcd.println("0");
      // } else {
      //   M5.Lcd.println("1");
      // } 
      // Set Led state
      static char ledBoolStr[2];
      if (isLedOn) {
        strcpy(ledBoolStr, "0");
      } else {
        strcpy(ledBoolStr, "1");
      } 
      axpLedCharacteristics.setValue(ledBoolStr);
      axpLedCharacteristics.notify();

      M5.Lcd.setCursor(0, 60, 2);
      M5.Lcd.print("Led State: ");
      if (isLedOn) {
        M5.Lcd.println("0");
      } else {
        M5.Lcd.println("1");
      } 
      
      lastTime = millis();
    }
  }
}
