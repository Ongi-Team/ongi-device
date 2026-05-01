---
name: ongi-device-conventions
description: Project-specific firmware conventions for the Ongi Smartcare smart pillbox (ESP32 / ESP-IDF, C). Use this skill whenever working on Ongi device firmware — BLE provisioning, Wi-Fi onboarding, RTC-driven dose scheduling, IR-based TAKEN/MISSED judgment, solenoid lock state machine, OPEN_ALL/CLOSE_ALL command handling, offline queue + sync, heartbeat, SOLENOID_FAIL/SENSOR_FAIL reporting. Trigger on "Ongi", "smart pillbox", "스마트 약통", "dispenser", "복약 알림", or anything referencing the project's domain terms. General ESP-IDF style (headers, FreeRTOS, esp_err_t, logging, NVS) lives in `esp-idf-conventions`. When the two conflict, this project skill wins. For wire-level API details (endpoint paths, payload schemas), refer to the server contract; this skill does not duplicate them.
---

# Ongi Device — Domain Conventions

Project-specific layer on top of `esp-idf-conventions`. This skill captures the *patterns* unique to the Ongi pillbox firmware. Endpoint paths and payload schemas are deliberately not listed here — those live in the server contract / `ongi-server-conventions`, and the device must match whatever the server agrees on.

## System shape

ESP32 firmware in C. The device drives 1–8 dispenser slots (each = one solenoid + one IR sensor). BLE is used **only for initial provisioning**; once Wi-Fi credentials are received, all server traffic goes over Wi-Fi + HTTP. Schedules and lock windows are cached locally so the device keeps working offline — RTC and the local cache drive dose triggering, not network state.

## Component layout

Split by domain responsibility, not by hardware peripheral. Recommended set:

```
components/
  ble_prov/    wifi_sta/    http_api/    nvs_store/
  scheduler/   dispenser/   lockbox/     event_bus/
```

`main.c` only orchestrates boot order: NVS → event loop → BLE-or-Wi-Fi → HTTP → scheduler. Don't spawn one task per slot — `dispenser_task` iterates over slots internally.

## Shared domain types

Wire values must match the server exactly. Define them once and convert through a single function — never `sprintf` enum names inline.

```c
typedef enum { DOSE_RESULT_TAKEN, DOSE_RESULT_MISSED } dose_result_t;
const char *dose_result_to_str(dose_result_t r);

typedef enum { HW_ERR_SOLENOID_FAIL, HW_ERR_SENSOR_FAIL } hw_error_code_t;
const char *hw_error_to_str(hw_error_code_t e);
```

When the server changes a string value, both sides ship in the same PR pair.

## Boot sequence

1. NVS init.
2. Load `device_id` and any cached Wi-Fi creds, schedules, lock window from NVS.
3. If Wi-Fi creds are present, try Wi-Fi. Otherwise enter BLE provisioning mode.
4. **BLE advertising** includes the device's unique ID. The app scans the printed QR for that same ID and only connects to a matching advertisement; reject GATT writes from peers that don't match.
5. After receiving Wi-Fi creds: persist to NVS, connect, then deinit BLE to reclaim ~30 KB RAM. Re-enable BLE only on an explicit re-provisioning trigger (e.g. long-press).
6. Register with the server, then fetch schedules + lock window. Treat both 200 and 409 on register as "already known".
7. On schedule fetch failure, **keep the existing cache** — never overwrite with empty.

Only NVS init is `ESP_ERROR_CHECK` material. Everything after that propagates `esp_err_t`.

## Event bus

One application event base for cross-component signals. Components communicate through events, not direct calls.

```c
ESP_EVENT_DECLARE_BASE(ONGI_EVENTS);

typedef enum {
    ONGI_EVT_PROVISIONED,
    ONGI_EVT_WIFI_UP,
    ONGI_EVT_WIFI_DOWN,
    ONGI_EVT_TIME_SYNCED,
    ONGI_EVT_SCHEDULE_UPDATED,
    ONGI_EVT_DOSE_DUE,           // { slot }
    ONGI_EVT_DOSE_RESULT,        // { slot, result, ts }
    ONGI_EVT_CMD_OPEN_ALL,
    ONGI_EVT_CMD_CLOSE_ALL,
    ONGI_EVT_HW_ERROR,           // { code, slot, ts }
} ongi_event_t;
```

The `WIFI_UP` handler enqueues three jobs and returns: flush pending records, flush pending errors, refresh schedules. **No HTTP work inside event handlers** — they post to the network task's queue.

## NVS layout

One namespace per component. Don't share a "storage" bucket.

| namespace | what lives here |
|---|---|
| `creds` | Wi-Fi SSID + password (encrypted) |
| `device` | `device_id` (written once at factory) |
| `sched` | schedules blob, lock window |
| `queue` | `pending_records`, `pending_errors` (offline queues) |

Offline queues are **bounded** (records ≈ 256, errors ≈ 64). When full, drop the oldest entry and `ESP_LOGW` — matches the spec's "discard oldest" rule.

## Server commands (long-poll, not push)

The original spec describes server-to-device commands as a "push", but plain HTTP can't push. Implement as **device-side polling** (every 2–5 s while idle) with the server treating commands as at-most-once: once the device fetches a command, the server removes it from the queue. Execute received commands immediately and report results through the normal event/error channels.

The server skill (`ongi-server-conventions`) is expected to implement the matching command-queue endpoint.

## Dose judgment ★

The most error-prone part. Encode the spec's rules carefully and pull all timing values into `app_config.h` rather than scattering magic numbers.

| constant | starting value | meaning |
|---|---|---|
| `STABILIZATION_MS` | 1000–2000 | wait after opening before reading the IR sensor |
| `IR_SAMPLE_HZ` | 10 | sampling rate during the dose window |
| `IR_DEBOUNCE_N` | 5 | consecutive "no pill" samples needed for TAKEN |
| `DOSE_WINDOW_MIN` | 20–30 | total window before MISSED is finalized |
| `REMINDER_1_MIN` | 5 | first re-alarm |
| `REMINDER_2_MIN` | 15 | second re-alarm |

**TAKEN** requires `IR_DEBOUNCE_N` consecutive "no pill" samples after stabilization. A single absent frame is noise — never TAKEN on one sample. The spec's "treat micro-fluctuations as not-taken" maps directly to this rule: only debounced absence counts.

**MISSED** is the dose window expiring without TAKEN ever debouncing. MISSED is a *normal event*, not an error — it goes through the regular dose-result reporting path so the server can fan out the guardian notification.

**SENSOR_FAIL** is when the IR sensor sits at a rail (saturated / disconnected / consistently above noise threshold) through the whole stabilization window. Plain absence of a pill is **not** a sensor failure — that's MISSED. On SENSOR_FAIL, abandon the dose and post `ONGI_EVT_HW_ERROR`.

**SOLENOID_FAIL**: command issued, expected state change doesn't happen → retry once → still failing → post `ONGI_EVT_HW_ERROR` with `SOLENOID_FAIL` and abandon the dose. Never `abort()`. A reset loop in a user's home is worse than a single reported error with degraded operation.

## Time-window lock

| schedules | lock window |
|---|---|
| ≥ 2 | first dose time → last dose time, hard-locked (resists physical force) |
| 1 | dose time − 30 min → dose time, auto-released after intake |
| 0 (`lockTimeRange = null`) | always locked unless an explicit OPEN_ALL is in flight |

Outside any lock window the lid still must not open without an OPEN_ALL command — physical tampering is ignored. The `lockbox` component owns this as a single state machine driven by RTC ticks plus `SCHEDULE_UPDATED` and `CMD_OPEN_ALL/CLOSE_ALL` events.

## Offline behavior

Network loss does **not** stop the device. The scheduler keeps firing off RTC, dispenser keeps judging, results and errors append to the persistent queues. Heartbeat is **suppressed** while offline — don't queue meaningless beats.

On reconnect (`WIFI_UP`):
1. Flush `pending_records` as a single batch upload.
2. Flush `pending_errors` one by one.
3. Re-fetch schedules; only replace the cache on success.

Only remove queue entries the server actually accepted. Failed sends stay in the queue for the next reconnect.

## Time

The device only reports wall-clock timestamps after SNTP has completed at least once. If SNTP hasn't synced and there's no prior sync persisted, **defer dose triggering** rather than firing on a 1970 timestamp — log a warning and retry sync. Use `clock_gettime(CLOCK_MONOTONIC, ...)` for elapsed-time logic so NTP steps don't break duration math.

## Logging style

Match the server's bracketed-event format so logs grep cleanly across the stack:

```c
ESP_LOGI(TAG, "[heartbeat] rssi=%d uptime=%llds", rssi, uptime_s);
ESP_LOGI(TAG, "[dose] slot=%d result=%s", slot, dose_result_to_str(result));
ESP_LOGW(TAG, "[wifi] disconnect reason=%d backoff=%dms", reason, backoff_ms);
ESP_LOGE(TAG, "[hw_error] code=%s slot=%d", hw_error_to_str(code), slot);
```

Never log Wi-Fi passwords, full HTTP response bodies, or raw IR ADC streams (use `ESP_LOGV` if you really must).

## Power / battery (out of scope)

Battery reporting was struck from the original spec. If it gets added later: agree on the payload schema with the server first, reuse the `pending_errors` queue mechanism so threshold events survive offline, and average several `esp_adc` samples rather than acting on a single read.

## Adding a new feature — checklist

1. Decide which component owns it; create a new component if the responsibility doesn't fit.
2. Cross-component signals go through `ongi_event_t`, not direct calls between components.
3. Persistence goes into that component's NVS namespace with an explicit default-on-missing path.
4. I/O runs in a task, never in `app_main` or an event handler.
5. If it talks to the server, the payload matches the server contract — ship the device PR and server PR together.
6. Tunable values go in `app_config.h`. No magic numbers in code.
7. Log state transitions at INFO, never log secrets.
8. For everything not in this skill — header layout, FreeRTOS sizing, `esp_err_t` handling, etc. — follow `esp-idf-conventions`.