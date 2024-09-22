#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "task.h"

class BluetoothTask : public Task<BluetoothTask>
{
    friend class Task<BluetoothTask>;

public:
    BluetoothTask(const uint8_t task_core);
    ~BluetoothTask();

    void sendData(const char *data);

protected:
    void run();

private:
    QueueHandle_t data_queue_;
};

extern BluetoothTask *bluetooth_task;

#endif // BLUETOOTH_H