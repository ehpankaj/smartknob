#include "bluetooth.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_TX "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_RX "beb5483e-36e1-4688-b7f5-ea07361b26a9"     // New UUID for RX characteristic
#define CHARACTERISTIC_UUID_SENSOR "beb5483e-36e1-4688-b7f5-ea07361b26aa" // New UUID for sensor data characteristic

#define MAX_BLE_DATA_SIZE 200

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
BLECharacteristic *pRxCharacteristic;     // New RX characteristic
BLECharacteristic *pSensorCharacteristic; // New sensor data characteristic
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
        Serial.println("Client connected");
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
        Serial.println("Client disconnected");
    }
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0)
        {
            Serial.println("Received Value: ");
            for (int i = 0; i < rxValue.length(); i++)
            {
                Serial.print(rxValue[i]);
            }
            Serial.println();
            // Process the received data here
        }
    }
};

void BluetoothTask::run()
{
    BLEDevice::init("ESP32SmartKnob");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // TX Characteristic (existing)
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY);
    pTxCharacteristic->addDescriptor(new BLE2902());

    // RX Characteristic (new)
    pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE);
    pRxCharacteristic->setCallbacks(new MyCharacteristicCallbacks()); // You'll need to implement this

    // Sensor Data Characteristic (new)
    pSensorCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_SENSOR,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pSensorCharacteristic->addDescriptor(new BLE2902());

    pService->start();
    pServer->getAdvertising()->start();
    Serial.println("Waiting for a client to connect...");

    char buffer[MAX_BLE_DATA_SIZE];
    while (1)
    {
        if (deviceConnected)
        {
            if (xQueueReceive(data_queue_, buffer, 0) == pdTRUE)
            {
                pTxCharacteristic->setValue((uint8_t *)buffer, strlen(buffer));
                pTxCharacteristic->notify();
                Serial.print("Sent: ");
                Serial.println(buffer);
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void BluetoothTask::sendData(const char *data)
{
    if (strlen(data) < MAX_BLE_DATA_SIZE)
    {
        xQueueSend(data_queue_, data, 0);
    }
    else
    {
        Serial.println("Data too large for BLE transmission");
    }
}

BluetoothTask::BluetoothTask(const uint8_t task_core) : Task("Bluetooth", 4000, 1, task_core)
{
    data_queue_ = xQueueCreate(10, MAX_BLE_DATA_SIZE);
    assert(data_queue_ != NULL);
}

BluetoothTask::~BluetoothTask()
{
    vQueueDelete(data_queue_);
}