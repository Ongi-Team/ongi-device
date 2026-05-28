#ifndef HTTP_CONFIG_H
#define HTTP_CONFIG_H

#define BASE_URL "your_base_url_here"

// Heartbeat endpoint
#define HEARTBEAT_ENDPOINT "/your_heartbeat_endpoint_here"
#define HEARTBEAT_URL BASE_URL HEARTBEAT_ENDPOINT

// Medication event endpoint
#define MEDICATION_EVENT_ENDPOINT "/your_medication_event_endpoint_here"
#define MEDICATION_EVENT_URL BASE_URL MEDICATION_EVENT_ENDPOINT

#endif // HTTP_CONFIG_H
