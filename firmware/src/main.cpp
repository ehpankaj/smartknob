#include <Arduino.h>
#include "configuration.h"
#include "interface_task.h"
#include "motor_task.h"
#include "serial/bluetooth.h"

Configuration config;

static MotorTask motor_task(1, config);
InterfaceTask interface_task(0, motor_task);
BluetoothTask* bluetooth_task = new BluetoothTask(0); // Changed to pointer

void setup()
{
  interface_task.begin();
  motor_task.begin();
  bluetooth_task->begin();

  config.setLogger(&interface_task);
  config.loadFromDisk();

  interface_task.setConfiguration(&config);

  motor_task.setLogger(&interface_task);

  // Free up the Arduino loop task
  vTaskDelete(NULL);
}

void loop()
{
  // This will never be called
}