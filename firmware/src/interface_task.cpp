#include "interface_task.h"
#include "semaphore_guard.h"
#include "util.h"

static PB_SmartKnobConfig configs[] = {
    // int32_t position;
    // float sub_position_unit;
    // uint8_t position_nonce;
    // int32_t min_position;
    // int32_t max_position;
    // float position_width_radians;
    // float detent_strength_unit;
    // float endstop_strength_unit;
    // float snap_point;
    // char text[51];
    // pb_size_t detent_positions_count;
    // int32_t detent_positions[5];
    // float snap_point_bias;
    // int8_t led_hue;
    {
        0,
        0,
        100,
        0,
        -1, // max position < min position indicates no bounds
        8.225806452 * PI / 180,
        3,
        1,
        1.1,
        "Unbounded\nNo detents",
        0,
        {},
        0,
        200,
    },
    {
        127,
        0,
        5,
        0,
        10,
        0.314159,
        1,
        1,
        1.1,
        "Fine less values\nWith detents",
        0,
        {},
        0,
        25,
    },
    {
        0,
        0,
        1,
        0,
        10,
        10 * PI / 180,
        0,
        0.3,
        1.1,
        "Bounded 0-10",
        0,
        {},
        0,
        0,
    },
    {
        0,
        0,
        0,
        0,
        -1, // max position < min position indicates no bounds
        10 * PI / 180,
        0,
        1,
        1.1,
        "Unbounded\nNo detents",
        0,
        {},
        0,
        200,
    },
    {
        0,
        0,
        2,
        0,
        72,
        10 * PI / 180,
        0,
        1,
        1.1,
        "Multi-rev\nNo detents",
        0,
        {},
        0,
        73,
    },
    {
        0,
        0,
        3,
        0,
        1,
        60 * PI / 180,
        1,
        1,
        0.55, // Note the snap point is slightly past the midpoint (0.5); compare to normal detents which use a snap point *past* the next value (i.e. > 1)
        "On/off\nStrong detent",
        0,
        {},
        0,
        157,
    },
    {
        0,
        0,
        4,
        0,
        0,
        60 * PI / 180,
        0.01,
        0.6,
        1.1,
        "Return-to-center",
        0,
        {},
        0,
        45,
    },
    {
        127,
        0,
        5,
        0,
        255,
        1 * PI / 180,
        0,
        1,
        1.1,
        "Fine values\nNo detents",
        0,
        {},
        0,
        219,
    },
    {
        127,
        0,
        5,
        0,
        255,
        1 * PI / 180,
        1,
        1,
        1.1,
        "Fine values\nWith detents",
        0,
        {},
        0,
        25,
    },
    {
        0,
        0,
        6,
        0,
        31,
        8.225806452 * PI / 180,
        2,
        1,
        1.1,
        "Coarse values\nStrong detents",
        0,
        {},
        0,
        200,
    },
    {
        0,
        0,
        6,
        0,
        100,
        8.225806452 * PI / 720,
        3,
        1,
        1.1,
        "Coarse values 2\nStrong detents",
        0,
        {},
        0,
        200,
    },
    {
        0,
        0,
        6,
        0,
        31,
        8.225806452 * PI / 180,
        1.4,
        1,
        1.1,
        "Coarse values\nWeak detents",
        0,
        {},
        0,
        0,
    },
    {
        0,
        0,
        7,
        0,
        31,
        7 * PI / 180,
        2.5,
        1,
        0.7,
        "Magnetic detents",
        4,
        {2, 10, 21, 22},
        0,
        73,
    },
    {
        0,
        0,
        8,
        -6,
        6,
        60 * PI / 180,
        1,
        1,
        0.55,
        "Return-to-center\nwith detents",
        0,
        {},
        0.4,
        157,
    },
};

InterfaceTask::InterfaceTask(const uint8_t task_core, MotorTask &motor_task) : Task("Interface", 3400, 1, task_core),
                                                                               stream_(),
                                                                               motor_task_(motor_task),
                                                                               plaintext_protocol_(stream_, [this]()
                                                                                                   { motor_task_.runCalibration(); }, motor_task_),
                                                                               proto_protocol_(stream_, [this](PB_SmartKnobConfig &config)
                                                                                               { applyConfig(config, true); })
{
    log_queue_ = xQueueCreate(10, sizeof(std::string *));
    assert(log_queue_ != NULL);

    knob_state_queue_ = xQueueCreate(1, sizeof(PB_SmartKnobState));
    assert(knob_state_queue_ != NULL);

    mutex_ = xSemaphoreCreateMutex();
    assert(mutex_ != NULL);

    initTouchSensor(); // Initialize touch sensor
}

InterfaceTask::~InterfaceTask()
{
    vSemaphoreDelete(mutex_);
}

void InterfaceTask::run()
{
    stream_.begin();

    Wire.begin();
    // Wire.setClock(400000);

    applyConfig(configs[0], false);
    motor_task_.addListener(knob_state_queue_);

    plaintext_protocol_.init([this]()
                             { changeConfig(true); }, [this]()
                             {
        if (!configuration_loaded_) {
            return;
        }
        if (strain_calibration_step_ == 0) {
            log("Strain calibration step 1: Don't touch the knob, then press 'S' again");
            strain_calibration_step_ = 1;
        } else if (strain_calibration_step_ == 1) {
            configuration_value_.strain.idle_value = strain_reading_;
            snprintf(buf_, sizeof(buf_), "  idle_value=%d", configuration_value_.strain.idle_value);
            log(buf_);
            log("Strain calibration step 2: Push and hold down the knob with medium pressure, and press 'S' again");
            strain_calibration_step_ = 2;
        } else if (strain_calibration_step_ == 2) {
            configuration_value_.strain.press_delta = strain_reading_ - configuration_value_.strain.idle_value;
            configuration_value_.has_strain = true;
            snprintf(buf_, sizeof(buf_), "  press_delta=%d", configuration_value_.strain.press_delta);
            log(buf_);
            log("Strain calibration complete! Saving...");
            strain_calibration_step_ = 0;
            if (configuration_->setStrainCalibrationAndSave(configuration_value_.strain)) {
                log("  Saved!");
            } else {
                log("  FAILED to save config!!!");
            }
        } });

    // Start in legacy protocol mode
    current_protocol_ = &plaintext_protocol_;

    ProtocolChangeCallback protocol_change_callback = [this](uint8_t protocol)
    {
        switch (protocol)
        {
        case SERIAL_PROTOCOL_LEGACY:
            current_protocol_ = &plaintext_protocol_;
            break;
        case SERIAL_PROTOCOL_PROTO:
            current_protocol_ = &proto_protocol_;
            break;
        default:
            log("Unknown protocol requested");
            break;
        }
    };

    plaintext_protocol_.setProtocolChangeCallback(protocol_change_callback);
    proto_protocol_.setProtocolChangeCallback(protocol_change_callback);

    // Interface loop:
    while (1)
    {
        if (xQueueReceive(knob_state_queue_, &latest_state_, 0) == pdTRUE)
        {
            publishState();
        }

        current_protocol_->loop();

        std::string *log_string;
        while (xQueueReceive(log_queue_, &log_string, 0) == pdTRUE)
        {
            current_protocol_->log(log_string->c_str());
            delete log_string;
        }

        if (!configuration_loaded_)
        {
            SemaphoreGuard lock(mutex_);
            if (configuration_ != nullptr)
            {
                configuration_value_ = configuration_->get();
                configuration_loaded_ = true;
            }
        }

        checkTouchSensor(); // Check touch sensor state

        delay(1);
    }
}

void InterfaceTask::log(const char *msg)
{
    // Allocate a string for the duration it's in the queue; it is free'd by the queue consumer
    std::string *msg_str = new std::string(msg);

    // Put string in queue (or drop if full to avoid blocking)
    xQueueSendToBack(log_queue_, &msg_str, 0);
}

void InterfaceTask::changeConfig(bool next)
{
    if (next)
    {
        current_config_ = (current_config_ + 1) % COUNT_OF(configs);
    }
    else
    {
        if (current_config_ == 0)
        {
            current_config_ = COUNT_OF(configs) - 1;
        }
        else
        {
            current_config_--;
        }
    }

    snprintf(buf_, sizeof(buf_), "Changing config to %d -- %s", current_config_, configs[current_config_].text);
    log(buf_);
    applyConfig(configs[current_config_], false);
}

void InterfaceTask::setConfiguration(Configuration *configuration)
{
    SemaphoreGuard lock(mutex_);
    configuration_ = configuration;
}

void InterfaceTask::publishState()
{
    // Apply local state before publishing to serial
    latest_state_.press_nonce = press_count_;
    current_protocol_->handleState(latest_state_);
}

void InterfaceTask::applyConfig(PB_SmartKnobConfig &config, bool from_remote)
{
    remote_controlled_ = from_remote;
    latest_config_ = config;
    motor_task_.setConfig(config);
}

void InterfaceTask::initTouchSensor()
{
    touchAttachInterrupt(TOUCH_PIN, nullptr, TOUCH_THRESHOLD);
    touch_pad_filter_start(10); // Filter touch signal with 10ms interval
}

void InterfaceTask::checkTouchSensor()
{
    uint16_t touch_value = touchRead(TOUCH_PIN);
    uint32_t current_time = millis();

    if (touch_value < TOUCH_THRESHOLD)
    {
        if (!touch_detected_ && (current_time - last_touch_time_ > DEBOUNCE_DELAY))
        {
            touch_detected_ = true;
            last_touch_time_ = current_time;
            log("Touch detected");
            // Add your touch handling code here
        }
    }
    else
    {
        touch_detected_ = false;
    }
}
