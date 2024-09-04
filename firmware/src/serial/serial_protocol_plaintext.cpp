#include "../proto_gen/smartknob.pb.h"
#include "serial_protocol_plaintext.h"

String configValues[13] = {
    "0",
    "0",
    "1",
    "0",
    "10",
    "10 * PI / 180",
    "0",
    "1",
    "1.1",
    "Test Config",
    "0",
    "0",
    "0",
}; // Initial configValues
String configKeys[13] = {"position", "sub_position_unit", "position_nonce", "min_position", "max_position", "position_width_radians", "detent_strength_unit", "endstop_strength_unit", "snap_point", "text", "detent_positions_count", "snap_point_bias", "led_hue"};

// Function to evaluate mathematical expressions
float SerialProtocolPlaintext::evalExpression(const String &expr)
{
    // Example: "8.225806452 * PI / 720"
    float multiplier = 0.0;
    int divisor = 1;

    // Split the expression
    int piIndex = expr.indexOf("PI");
    stream_.println(piIndex);
    if (piIndex != -1)
    {
        // Extract the multiplier
        String multiplierStr = expr.substring(0, piIndex); // Get substring before "PI"
        stream_.println(multiplierStr);
        multiplierStr.trim(); // Trim whitespace

        // Check for any remaining operators or invalid characters
        if (multiplierStr.endsWith(" *") || multiplierStr.endsWith(" /"))
        {
            multiplierStr.remove(multiplierStr.length() - 2); // Remove the operator and space
        }
        stream_.println(multiplierStr);
        multiplier = multiplierStr.toFloat(); // Convert to float

        // Extract the divisor
        String divisorStr = expr.substring(piIndex + 2); // Get substring after "PI"
        stream_.println(divisorStr);
        divisorStr.trim(); // Trim whitespace

        // Check for any remaining operators or invalid characters
        if (divisorStr.startsWith("* ") || divisorStr.startsWith("/ "))
        {
            divisorStr.remove(0, 2); // Remove the operator and space
        }
        stream_.println(divisorStr);
        divisor = divisorStr.toInt(); // Convert to int

        // Check for division by zero
        if (divisor == 0)
        {
            stream_.println("Error: Division by zero");
            return 0; // Return a default value or handle the error as needed
        }
    }
    stream_.println((multiplier * PI) / divisor);
    // Calculate the result
    return (multiplier * PI) / divisor;
}

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

void SerialProtocolPlaintext::applyNewConfig()
{
    PB_SmartKnobConfig newConfig;

    newConfig.position = configValues[0].toInt();                                 // Convert String to int32_t
    newConfig.sub_position_unit = configValues[1].toFloat();                      // Convert String to float
    newConfig.position_nonce = static_cast<uint8_t>(configValues[2].toInt());     // Convert String to uint8_t
    newConfig.min_position = configValues[3].toInt();                             // Convert String to int32_t
    newConfig.max_position = configValues[4].toInt();                             // Convert String to int32_t
    newConfig.position_width_radians = evalExpression(configValues[5]);           // Convert String to float
    newConfig.detent_strength_unit = configValues[6].toFloat();                   // Convert String to float
    newConfig.endstop_strength_unit = configValues[7].toFloat();                  // Convert String to float
    newConfig.snap_point = configValues[8].toFloat();                             // Convert String to float
    strncpy(newConfig.text, configValues[9].c_str(), sizeof(newConfig.text) - 1); // Copy String to char array
    newConfig.detent_positions_count = configValues[10].toInt();                  // Convert String to pb_size_t
    newConfig.snap_point_bias = configValues[11].toFloat();                       // Convert String to float
    newConfig.led_hue = configValues[12].toInt();                                 // Convert String to int16_t
    // newConfig.detent_positions = {};

    motor_task_.setConfig(newConfig); // Use motor_task_ reference
    stream_.println("New configuration applied.");
}

bool SerialProtocolPlaintext::isNumeric(String str)
{
    bool decimalPointFound = false; // Track if a decimal point has been found
    for (int i = 0; i < str.length(); i++)
    {
        if (str[i] == '.')
        {
            if (decimalPointFound)
                return false;         // More than one decimal point
            decimalPointFound = true; // Mark decimal point as found
        }
        else if (!isDigit(str[i]))
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
    case 'w':                                          // Left arrow key (in some terminals)
        selectedIndex = (selectedIndex - 1 + 13) % 13; // Move left
        break;
    case 's':                                     // Right arrow key (in some terminals)
        selectedIndex = (selectedIndex + 1) % 13; // Move right
        break;
    case 'a': // Up arrow key for increasing numeric configValues
        if (isNumeric(configValues[selectedIndex]))
        {
            configValues[selectedIndex] = String(configValues[selectedIndex].toInt() + 0.1); // Increment number
        }
        break;
    case 'd': // Down arrow key for decreasing numeric configValues
        if (isNumeric(configValues[selectedIndex]))
        {
            configValues[selectedIndex] = String(configValues[selectedIndex].toInt() - 0.1); // Decrement number
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
    for (int i = 0; i < 13; i++)
    {
        displayValue(i); // Display each value with initial settings
    }

    if (editing)
    {
        stream_.print("Editing: " + inputBuffer + " (Press Enter to confirm)");
    }
    else
    {
        applyNewConfig();
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
            displayUI();
        }
        if (addConfigMode && !editing)
        {
            handleNavigationInput(b); // Handle navigation input (arrow keys, etc.)
            displayUI();
        }
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
