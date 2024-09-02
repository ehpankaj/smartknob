#include "../proto_gen/smartknob.pb.h"
#include "serial_protocol_plaintext.h"

String configValues[14] = {
    "0",
    "0",
    "1",
    "0",
    "10",
    "10 * PI / 180",
    "0",
    "1",
    "1.1",
    "Bounded 0-10\nNo detents",
    "0",
    {},
    "0",
    "0",
}; // Initial configValues
String configKeys[14] = {"position", "sub_position_unit", "position_nonce", "min_position", "max_position", "position_width_radians", "detent_strength_unit", "endstop_strength_unit", "snap_point", "text", "detent_positions_count", "detent_positions", "snap_point_bias", "led_hue"};

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

// void SerialProtocolPlaintext::applyNewConfig(const String &configString)
// {
//     PB_SmartKnobConfig newConfig;
//     sscanf(configString.c_str(), "%d,%f,%hhu,%d,%d,%f,%f,%f,%f,%50[^,],%hhu,%d,%f,%hhd",
//            &newConfig.position,
//            &newConfig.sub_position_unit,
//            &newConfig.position_nonce,
//            &newConfig.min_position,
//            &newConfig.max_position,
//            &newConfig.position_width_radians,
//            &newConfig.detent_strength_unit,
//            &newConfig.endstop_strength_unit,
//            &newConfig.snap_point,
//            newConfig.text,
//            &newConfig.detent_positions_count,
//            newConfig.detent_positions,
//            &newConfig.snap_point_bias,
//            &newConfig.led_hue);

//     motor_task_.setConfig(newConfig); // Use motor_task_ reference
//     stream_.println("New configuration applied.");
// }

bool SerialProtocolPlaintext::isNumeric(String str)
{
    for (int i = 0; i < str.length(); i++)
    {
        if (!isDigit(str[i]))
            return false; // Check if each character is a digit
    }
    return true;
}

void SerialProtocolPlaintext::handleEditInput(char input)
{
    if (input == '\n' || input == '\r')
    {                                              // Handle both Enter (newline) and carriage return
        configValues[selectedIndex] = inputBuffer; // Update the selected value
        editing = false;                           // Exit editing mode
    }
    else if (input == '\b' || input == 127)
    { // Handle backspace
        if (inputBuffer.length() > 0)
        {
            inputBuffer.remove(inputBuffer.length() - 1); // Remove the last character
        }
    }
    else
    {
        inputBuffer += input; // Add the new character to the input buffer
    }
}

void SerialProtocolPlaintext::handleNavigationInput(char input)
{
    switch (input)
    {
    case 'n': // Enter edit mode to type a new value
        editing = true;
        inputBuffer = configValues[selectedIndex]; // Initialize buffer with the current value
        break;
    case 'a':                                          // Left arrow key (in some terminals)
        selectedIndex = (selectedIndex - 1 + 14) % 14; // Move left
        break;
    case 'd':                                     // Right arrow key (in some terminals)
        selectedIndex = (selectedIndex + 1) % 14; // Move right
        break;
    case 'w': // Up arrow key for increasing numeric configValues
        if (isNumeric(configValues[selectedIndex]))
        {
            configValues[selectedIndex] = String(configValues[selectedIndex].toInt() + 1); // Increment number
        }
        break;
    case 's': // Down arrow key for decreasing numeric configValues
        if (isNumeric(configValues[selectedIndex]))
        {
            configValues[selectedIndex] = String(configValues[selectedIndex].toInt() - 1); // Decrement number
        }
        break;
    default:
        Serial.println("Invalid Input");
        break;
    }
}

void SerialProtocolPlaintext::displayValue(int index)
{
    if (index == selectedIndex)
    {
        stream_.print("-> "); // Highlight the selected value
    }
    else
    {
        stream_.print("   ");
    }
    stream_.println(String(configKeys[index]) + ": " + configValues[index]);
}

void SerialProtocolPlaintext::displayUI()
{
    for (int i = 0; i < 25; i++)
    {
        stream_.println(); // Print multiple new lines to simulate clearing the screen
    }

    stream_.println("=== Input Menu ===");
    for (int i = 0; i < 14; i++)
    {
        displayValue(i); // Display each value with initial settings
    }

    stream_.println("\nUse Left/Right arrow keys to select, Up/Down to change (for numbers), or 'n' to type a value.");
    if (editing)
    {
        stream_.print("Editing: " + inputBuffer + " (Press Enter to confirm)");
    }
}

void SerialProtocolPlaintext::loop()
{
    // static String currentConfig = "";

    while (stream_.available() > 0)
    {
        int b = stream_.read();
        if (editing)
        {
            handleEditInput(b); // If in editing mode, handle user input for editing
        }
        if (addConfigMode && !editing)
        {
            handleNavigationInput(b); // Handle navigation input (arrow keys, etc.)
        }
        displayUI();
        if (b == 'N') // Start editing the configuration
        {
            addConfigMode = true;
            // currentConfig = stream_.readStringUntil('\n'); // Read the new config
            // applyNewConfig(currentConfig);                 // Apply the new config
            // stream_.println("Configuration applied. You can continue editing.");
        }
        else
            // if (b == '\n') // If Enter is pressed, apply the current config
            // {
            //     if (!currentConfig.isEmpty())
            //     {
            //         applyNewConfig(currentConfig);
            //         stream_.println("Configuration applied. Continue editing or press 'N' to enter new config.");
            //     }
            // }
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
        // else // Append any other character to the current config
        // {
        //     currentConfig += (char)b; // Add the character to the current config
        // }

        // // Display the current configuration
        // stream_.print("Current configuration: ");
        // stream_.println(currentConfig);
    }
}

void SerialProtocolPlaintext::init(DemoConfigChangeCallback demo_config_change_callback, StrainCalibrationCallback strain_calibration_callback)
{
    demo_config_change_callback_ = demo_config_change_callback;
    strain_calibration_callback_ = strain_calibration_callback;
    stream_.println("SmartKnob starting!\n\nSerial mode: plaintext\nPress 'C' at any time to calibrate motor/sensor.\nPress 'S' at any time to calibrate strain sensors.\nPress <Space> to change haptic modes.");
}
