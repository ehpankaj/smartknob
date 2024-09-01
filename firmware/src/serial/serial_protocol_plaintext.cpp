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

void SerialProtocolPlaintext::loop()
{
    while (stream_.available() > 0)
    {
        int b = stream_.read();
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
        else if (b == 'n')
        { // New command for entering config
            handleNewConfigInput();
        }
    }
}

// New method to handle new configuration input
void SerialProtocolPlaintext::handleNewConfigInput()
{
    // Stop all ongoing processes
    stream_.println("Entering new configuration mode. Please provide the new config values in the following format:");
    stream_.println("position, sub_position_unit, position_nonce, min_position, max_position, position_width_radians, detent_strength_unit, endstop_strength_unit, snap_point, text, detent_positions_count, detent_positions (comma-separated)");
    stream_.println("Example: 0, 0, 0, 0, -1, 0.1745329, 0, 1, 1.1, \"Unbounded\\nNo detents\", 0, {}");

    // Read user input
    std::string userInput;
    while (true)
    {
        if (stream_.available())
        {
            char c = stream_.read();
            if (c == '\n')
            { // End of input
                break;
            }
            userInput += c; // Append character to input string
        }
    }

    // Print the received input
    stream_.println("Received configuration:");
    stream_.println(userInput.c_str());

    // Here you can add logic to parse the user input and update the configs array
    // For example, you can split the userInput string by commas and assign values to a new PB_SmartKnobConfig object
    // Note: Implement the actual parsing and updating logic as needed
}

void SerialProtocolPlaintext::init(DemoConfigChangeCallback demo_config_change_callback, StrainCalibrationCallback strain_calibration_callback)
{
    demo_config_change_callback_ = demo_config_change_callback;
    strain_calibration_callback_ = strain_calibration_callback;
    stream_.println("SmartKnob starting!\n\nSerial mode: plaintext\nPress 'C' at any time to calibrate motor/sensor.\nPress 'S' at any time to calibrate strain sensors.\nPress <Space> to change haptic modes.");
}
