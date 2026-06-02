# Schedule Fetch Contract

## Server Contract

- Endpoint: `GET /api/device/schedules`
- Auth: `Device-Token` header
- Response data: array of schedule entries sorted by slot number
- Entry fields:
  - `slotNumber`: 1-based device slot number
  - `scheduledTime`: local time string from the server, for example `08:00:00`

Example response:

```json
{
  "isSuccess": true,
  "code": "DEVICE_200",
  "message": "디바이스 스케줄 조회에 성공했습니다.",
  "data": [
    {
      "slotNumber": 1,
      "scheduledTime": "08:00:00"
    }
  ]
}
```

## Device Behavior For Issue #35

- MQTT `SCHEDULE_UPDATED` is only a refresh notification.
- Receiving `SCHEDULE_UPDATED` must not enqueue a dispense event.
- Until the device-side HTTP fetch and parsing flow is implemented, the schedule task only logs that a refresh notification was consumed.
- Unknown payloads on the schedule-updated topic are ignored.

## Pending Device Work

- Fetch schedules from `GET /api/device/schedules` after a validated refresh notification.
- Validate `slotNumber` against `SCHEDULE_SLOT_COUNT`.
- Parse `scheduledTime` into hour/minute before calling `schedule_store_apply()`.
- Handle HTTP timeout, non-2xx status, malformed JSON, duplicate slots, and empty schedule responses without changing hardware state.

#### Sources

- Server PR: `Ongi-Team/ongi-server#63`
- Device issue: `Ongi-Team/ongi-device#35`
