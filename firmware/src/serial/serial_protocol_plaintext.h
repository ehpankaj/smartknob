#pragma once

#include "../proto_gen/smartknob.pb.h"

#include "interface_callbacks.h"
#include "motor_task.h"
#include "serial_protocol.h"
#include "uart_stream.h"

typedef std::function<void(void)> DemoConfigChangeCallback;
typedef std::function<void(void)> StrainCalibrationCallback;

class SerialProtocolPlaintext : public SerialProtocol
{
public:
    SerialProtocolPlaintext(Stream &stream, MotorCalibrationCallback motor_calibration_callback, MotorTask &motor_task)
        : SerialProtocol(), stream_(stream), motor_calibration_callback_(motor_calibration_callback), motor_task_(motor_task) {} // Added motor_task_ initialization
    ~SerialProtocolPlaintext() {}
    void log(const char *msg) override;
    void loop() override;
    void handleState(const PB_SmartKnobState &state) override;
    void displayValue(int index);
    void displayUI();
    bool isNumeric(String str);
    void handleEditInput(char input);
    void handleNavigationInput(char input);

    void init(DemoConfigChangeCallback demo_config_change_callback, StrainCalibrationCallback strain_calibration_callback);
    void applyNewConfig();

private:
    String currentConfig;
    Stream &stream_;
    int selectedIndex = 0; // Index of the currently selected value
    bool editing = false;  // Indicates if user is currently typing a new value
    bool addConfigMode = false;
    String inputBuffer = "";
    MotorCalibrationCallback motor_calibration_callback_;
    MotorTask &motor_task_;
    PB_SmartKnobState latest_state_ = {};
    DemoConfigChangeCallback demo_config_change_callback_;
    StrainCalibrationCallback strain_calibration_callback_;
};
