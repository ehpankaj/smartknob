#include "bluetooth.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_TX "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define MAX_BLE_DATA_SIZE 200

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
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

void BluetoothTask::run()
{
    BLEDevice::init("ESP32SmartKnob");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY);
    pTxCharacteristic->addDescriptor(new BLE2902());
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