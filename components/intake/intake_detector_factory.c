#include "intake_detector_factory.h"
#include "dummy_intake_detector.h"
// TODO: Add other intake detector headers here

const IntakeDetector* create_intake_detector() {
    // TODO: Implement logic to select the appropriate intake detector based on configuration or hardware detection
    return &DUMMY_INTAKE_DETECTOR;
}