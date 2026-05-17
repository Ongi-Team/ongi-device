#include "intake_detector_factory.h"
#include "dummy_intake_detector.h"
// TODO: Add other intake detector headers here

const IntakeDetector *get_default_intake_detector(void) {
    // TODO: Implement logic to select the appropriate intake detector based on configuration or hardware detection
    return &DUMMY_INTAKE_DETECTOR;
}