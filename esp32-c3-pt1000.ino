#include <Adafruit_MAX31865.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define RTD_PIN 7
#define RREF 4300.0

Adafruit_MAX31865 pt1000 = Adafruit_MAX31865(RTD_PIN);
BLEServer *server = NULL;
BLECharacteristic *characteristic = NULL;
bool connected = false;

#define SERVICE_UUID "621ABF0E-E642-40D8-B766-8D910900B1C5"
#define CHARACTERISTIC_UUID "E33E3DC8-BCB7-476A-8D26-1B6D968F615C"

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* server) {
      connected = true;
      neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS);
      BLEDevice::getAdvertising()->stop();
    }

    void onDisconnect(BLEServer* server) {
      connected = false;
      neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0);
      BLEDevice::startAdvertising();
    }
};

void setup() {
  Serial.begin(115200);
  pt1000.begin(MAX31865_2WIRE);

  BLEDevice::init("ESP32-C3 PT1000");
  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  BLEService *service = server->createService(SERVICE_UUID);
  characteristic = service->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  characteristic->addDescriptor(new BLE2902());
  service->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0);
}

void loop() {
  uint8_t fault = pt1000.readFault();
  if (fault) {
    Serial.print("Fault 0x"); Serial.println(fault, HEX);
    neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, RGB_BRIGHTNESS / 2, 0);
    pt1000.clearFault();
  }

  unsigned long time = millis();
  float temp = pt1000.temperature(1000, RREF);

  // MTU size 23 - 3 bytes header + null byte
  char buffer[21];
  snprintf(buffer, sizeof(buffer), "%.2f %.2f", time / 1000.0, temp);
  Serial.println(buffer);

  if (connected) {
    characteristic->setValue((uint8_t *) buffer, strlen(buffer));
    characteristic->notify();
  }

  delay(50);
}
