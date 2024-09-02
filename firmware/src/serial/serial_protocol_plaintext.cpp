#include "../proto_gen/smartknob.pb.h"
#include "serial_protocol_plaintext.h"

void SerialProtocolPlaintext::handleState(const PB_SmartKnobState &state)
{
    bool substantial_change = (latest_state_.current_position != state.current_position) || (latest_state_.config.detent_strength_unit != state.config.detent_strength_unit) || (latest_state_.config.endstop_strength_unit != state.config.endstop_strength_unit) || (latest_state_.config.min_position != state.config.min_position) || (latest_state_.config.max_position != state.config.max_position);
    latest_state_ = state;

    if (substantial_change)
    {
        stream_.printf("STATE: %d [%d, %d]  (detent strength: %0.2f, width: %0.0f deg, endstop strength: %0.2f)\n",
                       state.current_position,
                       state.config.min_position,
                       state.config.max_position,
                       state.config.detent_strength_unit,
                       degrees(state.config.position_width_radians),
                       state.config.endstop_strength_unit);
    }
}

void SerialProtocolPlaintext::log(const char *msg)
{
    stream_.print("LOG: ");
    stream_.println(msg);
}

void SerialProtocolPlaintext::applyNewConfig(const String &configString)
{
    PB_SmartKnobConfig newConfig;
    sscanf(configString.c_str(), "%d,%f,%hhu,%d,%d,%f,%f,%f,%f,%50[^,],%hhu,%d,%f,%hhd",
           &newConfig.position,
           &newConfig.sub_position_unit,
           &newConfig.position_nonce,
           &newConfig.min_position,
           &newConfig.max_position,
           &newConfig.position_width_radians,
           &newConfig.detent_strength_unit,
           &newConfig.endstop_strength_unit,
           &newConfig.snap_point,
           newConfig.text,
           &newConfig.detent_positions_count,
           newConfig.detent_positions,
           &newConfig.snap_point_bias,
           &newConfig.led_hue);

    motor_task_.setConfig(newConfig); // Use motor_task_ reference
    stream_.println("New configuration applied.");
}

void SerialProtocolPlaintext::loop()
{
    static String currentConfig = "";

    while (stream_.available() > 0)
    {
        int b = stream_.read();
        if (b == 'N') // Start editing the configuration
        {
            currentConfig = stream_.readStringUntil('\n'); // Read the new config
            applyNewConfig(currentConfig);                 // Apply the new config
            stream_.println("Configuration applied. You can continue editing.");
        }
        else if (b == '\n') // If Enter is pressed, apply the current config
        {
            if (!currentConfig.isEmpty())
            {
                applyNewConfig(currentConfig);
                stream_.println("Configuration applied. Continue editing or press 'N' to enter new config.");
            }
        }
        if (b == 0)
        {
            if (protocol_change_callback_)
            {
                protocol_change_callback_(SERIAL_PROTOCOL_PROTO);
            }
            break;
        }
        if (b == ' ')
        {
            if (demo_config_change_callback_)
            {
                demo_config_change_callback_();
            }
        }
        else if (b == 'C')
        {
            motor_calibration_callback_();
        }
        else if (b == 'S')
        {
            if (strain_calibration_callback_)
            {
                strain_calibration_callback_();
            }
        }
        else // Append any other character to the current config
        {
            currentConfig += (char)b; // Add the character to the current config
        }

        // Display the current configuration
        stream_.print("Current configuration: ");
        stream_.println(currentConfig);
    }
}

void SerialProtocolPlaintext::init(DemoConfigChangeCallback demo_config_change_callback, StrainCalibrationCallback strain_calibration_callback)
{
    demo_config_change_callback_ = demo_config_change_callback;
    strain_calibration_callback_ = strain_calibration_callback;
    stream_.println("SmartKnob starting!\n\nSerial mode: plaintext\nPress 'C' at any time to calibrate motor/sensor.\nPress 'S' at any time to calibrate strain sensors.\nPress <Space> to change haptic modes.");
}
