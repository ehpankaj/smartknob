#pragma once

#include <AceButton.h>
#include <Arduino.h>
#include <driver/touch_pad.h> // Include touch pad driver

#include "configuration.h"
#include "logger.h"
#include "motor_task.h"
#include "serial/serial_protocol_plaintext.h"
#include "serial/serial_protocol_protobuf.h"
#include "serial/uart_stream.h"
#include "task.h"
#include "serial/bluetooth.h" // Include BluetoothTask header

#ifndef SK_FORCE_UART_STREAM
#define SK_FORCE_UART_STREAM 0
#endif

class InterfaceTask : public Task<InterfaceTask>, public Logger
{
    friend class Task<InterfaceTask>; // Allow base Task to invoke protected run()

public:
    InterfaceTask(const uint8_t task_core, MotorTask &motor_task, BluetoothTask &bluetooth_task); // Add BluetoothTask parameter
    virtual ~InterfaceTask();

    void log(const char *msg) override;
    void setConfiguration(Configuration *configuration);

    void sendConfigType(int configType, int currentPosition); // Ensure this line is present

protected:
    void run();

private:
    UartStream stream_;
    MotorTask &motor_task_;
    BluetoothTask &bluetooth_task_; // Declare BluetoothTask member variable
    char buf_[128];

    SemaphoreHandle_t mutex_;
    Configuration *configuration_ = nullptr; // protected by mutex_

    PB_PersistentConfiguration configuration_value_;
    bool configuration_loaded_ = false;

    uint8_t strain_calibration_step_ = 0;
    int32_t strain_reading_ = 0;

    SerialProtocol *current_protocol_ = nullptr;
    bool remote_controlled_ = false;
    int current_config_ = 0;
    uint8_t press_count_ = 1;

    PB_SmartKnobState latest_state_ = {};
    PB_SmartKnobConfig latest_config_ = {};

    QueueHandle_t log_queue_;
    QueueHandle_t knob_state_queue_;
    SerialProtocolPlaintext plaintext_protocol_;
    SerialProtocolProtobuf proto_protocol_;

    void changeConfig(bool next);
    void publishState();
    void applyConfig(PB_SmartKnobConfig &config, bool from_remote);

    // Touch sensor variables
    void initTouchSensor();
    void checkTouchSensor();
    void switchConfig();                            // New method to switch configurations
    static constexpr uint8_t TOUCH_PIN = 15;        // GPIO15
    static constexpr uint16_t TOUCH_THRESHOLD = 20; // Adjust as needed
    static constexpr uint32_t DEBOUNCE_DELAY = 50;  // 50 ms debounce delay
    uint32_t last_touch_time_ = 0;
    int config_index = 0;
    bool touch_detected_ = false;
};
