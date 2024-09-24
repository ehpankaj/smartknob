#include <Arduino.h>
#include "configuration.h"
#include "interface_task.h"
#include "motor_task.h"
#include "serial/bluetooth.h"

Configuration config;

BluetoothTask bluetooth_taskt(0);
static MotorTask motor_task(1, config);
InterfaceTask interface_task(0, motor_task, bluetooth_taskt);

void setup()
{
  interface_task.begin();
  motor_task.begin();
  bluetooth_taskt.begin();

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