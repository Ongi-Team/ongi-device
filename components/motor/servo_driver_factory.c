#include "servo_driver_factory.h"
#include "dummy_servo_driver.h"

const ServoDriver *get_default_servo_driver(void) {
    return &DUMMY_SERVO_DRIVER;
}