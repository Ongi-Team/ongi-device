---
name: esp-idf-conventions
description: General best-practice coding style for ESP32 firmware written in C with ESP-IDF (v5.x and v6.x). Use this skill whenever writing or reviewing ESP-IDF C code regardless of the specific project — header layout, naming, FreeRTOS task/queue/timer patterns, esp_err_t handling, ESP_LOGx usage, NVS, esp_event, esp_http_client, esp_netif/Wi-Fi, GPIO drivers, ISR safety, stack/heap discipline, sdkconfig/CMake conventions. Trigger it on any mention of ESP-IDF, ESP32 (S2/S3/C2/C3/C5/C6/C61/H2/H4/H21/P4), idf.py, sdkconfig, menuconfig, xTaskCreate, ESP_ERROR_CHECK, IRAM_ATTR, esp_log.h, nvs_flash, or when the user shows ESP-IDF C code. This is the language/framework-level guide and is project-agnostic; pair it with any project-specific conventions skill (the project skill takes priority on conflicts).
---

# ESP-IDF Firmware Conventions (General)

Project-agnostic coding style for ESP-IDF C firmware on ESP32-family SoCs. Aligned with Espressif's official style guide and common embedded-systems practice. This is the **default** baseline; a project's own conventions override these where they differ.

Targets ESP-IDF v5.x and v6.x. Notable v6.0 changes: legacy drivers removed, compiler warnings become errors, new crypto API layer, EIM (Espressif Installation Manager) replaces the v5 installer. When upgrading, consult the official migration guide before changing code.

---

## 1. Project skeleton

ESP-IDF builds use the component model. Don't dump everything into `main/`.

```
project/
├── CMakeLists.txt
├── sdkconfig.defaults              ← committed; sdkconfig is gitignored
├── partitions.csv                  ← if customized
├── main/
│   ├── CMakeLists.txt              ← idf_component_register(...)
│   └── main.c                      ← app_main only; wires components
└── components/
    └── <component>/
        ├── CMakeLists.txt
        ├── include/<component>.h   ← public API
        ├── <component>.c
        └── <component>_internal.h  ← (optional) private headers
```

Every component:
- declares its dependencies explicitly via `REQUIRES` (public) and `PRIV_REQUIRES` (private) in `idf_component_register`. Don't add components you don't actually use,
- exposes a single public header in `include/`. Internal helpers stay outside `include/`,
- compiles cleanly with `-Wall -Wextra -Werror`. (v6.0 enforces this anyway.)

`app_main` is small: initialize NVS, the default event loop, and Wi-Fi/networking, then start tasks owned by components and return. Long-running work doesn't live in `app_main`.

---

## 2. CMake & build

- Use `idf.py` for everything: `set-target`, `menuconfig`, `build`, `flash`, `monitor`. Don't shell out to `cmake`/`ninja` directly.
- Pin the ESP-IDF version in the README and CI (e.g. "ESP-IDF v5.5.3" or "v6.0.1"). Espressif backports breaking-ish changes between minors occasionally.
- `idf.py set-target esp32` (or whichever variant) is captured in `sdkconfig.defaults`. Don't commit personal `sdkconfig` — it leaks per-developer paths.
- CMake style: 4-space indent, lowercase commands, max line 120, no content in optional parentheses after `endif()` / `endforeach()`.
- Component CMakeLists template:

```cmake
idf_component_register(
    SRCS "foo.c" "bar.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    REQUIRES esp_event esp_wifi
    PRIV_REQUIRES nvs_flash esp_http_client
)
```

- Third-party C/C++ libraries come in via the **ESP Component Manager** (`idf_component.yml`), not by vendoring source under `components/`.

---

## 3. C language & file layout

- C11. No GNU-only extensions unless necessary (and then commented).
- C++ only when a library forces it; keep public APIs in C-callable headers.
- One translation unit per concept. Don't put unrelated functions in the same file because "they were small."
- File names are `snake_case.c` / `snake_case.h`. Headers ending in `_internal.h` are not for external consumers.

### Header structure

Use `#pragma once`. After it, includes, then a C++ guard, then declarations:

```c
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t foo_init(void);

#ifdef __cplusplus
}
#endif
```

### Include order

1. C standard library (`<stdint.h>`, `<string.h>`, …)
2. Other POSIX (`<sys/queue.h>`, `<unistd.h>`, …)
3. Common IDF headers (`esp_log.h`, `esp_err.h`, `esp_system.h`, `esp_timer.h`, …)
4. Other component headers (FreeRTOS, drivers)
5. Public headers of the current component
6. Private headers

Angle brackets for standard/POSIX, double quotes for everything else. A blank line between groups.

### Naming

- **Public functions/types** are namespaced with the component prefix: `wifi_sta_start`, `nvs_kv_get_blob`. Avoid `esp_*` for non-Espressif code — that prefix is theirs.
- **Static (file-local) variables** are prefixed `s_`: `static int s_retry_count;`. Static functions don't need a prefix.
- **Macros and enum constants** are `UPPER_SNAKE`. Enum types are `snake_case_t`.
- Anything used in only one source file is `static`. Don't pollute the link namespace.

### Formatting (matches Espressif's astyle config)

- 4-space indent, never tabs (except in Makefiles).
- Max line length 120.
- Opening brace **on the same line for control flow** (`if`, `for`, `while`), **on the next line for function definitions**.
- One declaration per line. Avoid `int *a, b;` ambiguity.
- No trailing whitespace, LF line endings, file ends with a newline.

```c
// Function definition: brace on its own line.
esp_err_t foo_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }
    return do_init();
}
```

### Comments

- Doxygen-style (`/** ... */`) on public APIs in headers. `@param`, `@return`, `@note`.
- Inline comments explain **why**, not what. The code already says what.
- Don't add author/date comments (`// added 2026-04-05 by alice`) — `git blame` knows.

---

## 4. Error handling — `esp_err_t`

Almost every IDF function returns `esp_err_t`. Treat the return value as the primary signal for failure.

- Functions that can fail return `esp_err_t`. Output values go through pointer arguments.
- **`ESP_ERROR_CHECK(x)` aborts on failure.** Use it only for *bring-up code that cannot fail* — NVS init in `app_main`, GPIO config, `esp_event_loop_create_default`. Never use it on network/sensor calls during normal operation; you'll crash a deployed device on a transient error.
- For recoverable errors, propagate explicitly:

```c
esp_err_t err = nvs_get_u32(handle, "boot_count", &count);
if (err == ESP_ERR_NVS_NOT_FOUND) {
    count = 0;                               // first boot — fine
} else if (err != ESP_OK) {
    ESP_LOGE(TAG, "nvs_get_u32 failed: %s", esp_err_to_name(err));
    return err;
}
```

- `ESP_RETURN_ON_ERROR(x, TAG, "msg…")` and `ESP_GOTO_ON_ERROR` (from `esp_check.h`) reduce boilerplate when the right thing is "log and return / goto cleanup".
- `assert()` is for **invariants that indicate a bug** (NULL output pointer, slot index out of range). Not for runtime conditions like "Wi-Fi connected" or "user input valid". Side-effect-free expressions only — `CONFIG_COMPILER_OPTIMIZATION_ASSERTION_LEVEL` may compile them out.
- When checking that an `esp_err_t` is `ESP_OK`, use `ESP_ERROR_CHECK`, not `assert`.
- Don't `abort()` on hardware faults the user could otherwise weather. A reset loop in a deployed device is worse than degraded operation plus a logged error.

---

## 5. Logging — `ESP_LOGx`

- One `static const char *TAG = "<component>"` per source file. Tag is short, lowercase, matches the component name.
- Levels:
  - `ESP_LOGE` — recoverable error worth attention
  - `ESP_LOGW` — degraded but expected (reconnect, missing optional config)
  - `ESP_LOGI` — state transitions and milestones (init done, connected, request sent)
  - `ESP_LOGD` — verbose flow you'd disable in production
  - `ESP_LOGV` — sample-level / per-byte
- Default production level is `INFO`. Override per-tag at runtime with `esp_log_level_set("wifi", ESP_LOG_WARN);` to silence noisy components.
- Format with the printf specifiers — note the IDF-specific ones:
  - `%lu` / `%ld` for `uint32_t` / `int32_t` (they're `unsigned long` / `long` on Xtensa)
  - `%llu` / `%lld` for 64-bit
  - `PRIu32`, `PRIu64` from `<inttypes.h>` are the portable choice
- **Never** log secrets: Wi-Fi passwords, API tokens, full HTTP bodies that may contain PII. Log presence/length only.
- Bracketed-tag style is readable and greppable: `ESP_LOGI(TAG, "[wifi] connected ssid=%s rssi=%d", ssid, rssi);`
- `ESP_LOG_BUFFER_HEX` / `ESP_LOG_BUFFER_CHAR` exist for binary dumps — prefer them over hand-rolled hex loops.
- No `printf` in shipped code. `printf` doesn't go through the log system and can't be filtered.

---

## 6. FreeRTOS

### Tasks

- One task per long-lived concern. Don't fan out tasks for every instance of a thing — iterate inside one task.
- **Stack sizes are measured.** Start at 4096 bytes for tasks doing TLS/HTTP/JSON, 2048 for sensor/GPIO loops. Use `uxTaskGetStackHighWaterMark()` during bring-up and shrink with ~512 bytes headroom. Stack overflow on ESP32 is a hard crash with a confusing trace.
- **Priorities**: keep them in a single header so collisions are visible. Hardware/ISR-fed loops > control logic > network > telemetry. Don't go above 10 for application tasks — leave room for system services.
- **Core affinity**: default to `tskNO_AFFINITY`. Pin only after measuring contention. Common pattern: networking on PRO_CPU (core 0), tight hardware loops on APP_CPU (core 1).
- Every `xTaskCreate` gets an explicit, useful name. The name shows up in crash dumps; "Task1" wastes the slot.
- Use `xTaskCreatePinnedToCore` only when you've decided on affinity — not as a default.

### Synchronization

- **Queues** for producer→consumer data flow (sensor samples, outgoing requests). Don't share state through globals + mutexes when a queue fits.
- **Event groups** for "X happened" coordination across tasks (Wi-Fi up, provisioned, time synced). Document each bit in the public header.
- **Mutexes** (`xSemaphoreCreateMutex`) for protecting a small, well-defined struct. Hold them briefly; never across an HTTP call or any blocking I/O.
- **No `vTaskDelay` as a substitute for synchronization.** If you're delaying because "the other task should be done by now," you have a race condition — block on a queue or event bit instead.
- Avoid `taskENTER_CRITICAL` outside of tight ISR-coordination paths. It disables interrupts.

### Timers

- Software timers: prefer `esp_timer` over the FreeRTOS timer service for anything internal — it has microsecond resolution and a simple API. The FreeRTOS timer service runs callbacks on a dedicated task with a configurable depth queue (which can fill).
- Timer callbacks run in a system context and **must not block**. Push work onto a queue and return.
- For periodic work that does I/O, the timer enqueues a job; the consumer task does the work.

### ISRs

- Keep ISRs tiny. Mark them `IRAM_ATTR` so they don't fault on flash cache misses.
- ISR functions call only `*_FromISR` APIs (`xQueueSendFromISR`, `xSemaphoreGiveFromISR`).
- No `printf`, no `ESP_LOGx`, no heap allocation, no mutex acquisition inside an ISR.
- Use `portYIELD_FROM_ISR(higher_priority_task_woken)` after a `_FromISR` call that wakes a task.

---

## 7. NVS (non-volatile storage)

- One namespace per component (`"wifi"`, `"sched"`, `"queue"`). Don't dump everything into `"storage"`.
- Always handle `nvs_flash_init` returning `ESP_ERR_NVS_NO_FREE_PAGES` or `ESP_ERR_NVS_NEW_VERSION_FOUND` — erase and retry once on first boot:

```c
esp_err_t err = nvs_flash_init();
if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
}
ESP_ERROR_CHECK(err);
```

- Reads return `esp_err_t`. Missing keys come back as `ESP_ERR_NVS_NOT_FOUND` — caller decides the default explicitly. Don't paper over silently.
- Bundle related writes inside a single `nvs_open` handle and call `nvs_commit` once. Half-written state is real.
- Wi-Fi credentials should go through `wifi_prov_mgr` or a dedicated namespace. Whichever you pick, never log them.
- Use `esp_secure_cert` or `nvs_flash` encryption (`CONFIG_NVS_ENCRYPTION`) for keys/secrets in production builds.
- Bounded queues that persist across reboot (offline event queues, retry buffers) need an explicit cap. Drop oldest with a warning when full.

---

## 8. esp_event

- Use the **default event loop** (`esp_event_loop_create_default`) unless you have a specific reason to spawn a dedicated loop.
- Define a custom event base for your application's signals; don't piggyback on a system base:

```c
ESP_EVENT_DECLARE_BASE(APP_EVENTS);

typedef enum {
    APP_EVT_PROVISIONED,
    APP_EVT_NETWORK_UP,
    APP_EVT_NETWORK_DOWN,
    APP_EVT_DATA_READY,
} app_event_t;
```

- Event handlers are short and non-blocking. If the work is heavy, post onto a queue and return.
- **Payloads are plain structs by value** — `esp_event_post` copies them. Don't pass pointers to stack data.
- Register handlers from the owning component during init; deregister on shutdown (matters for tests, OTA, and re-init).
- Components communicate cross-cutting state through events, not direct calls. Direct calls are fine within a component.

---

## 9. Wi-Fi (esp_netif + esp_wifi)

- Use `esp_netif` + `esp_wifi`. Legacy `tcpip_adapter` is removed in v5+; don't use it.
- Standard init order in `app_main` (or a `wifi_sta_init`):
  1. `esp_netif_init`
  2. `esp_event_loop_create_default`
  3. `esp_netif_create_default_wifi_sta` (or AP)
  4. `esp_wifi_init` with `WIFI_INIT_CONFIG_DEFAULT`
  5. Register handlers for `WIFI_EVENT` and `IP_EVENT`
  6. `esp_wifi_set_mode`, `esp_wifi_set_config`, `esp_wifi_start`
- Reconnect uses **exponential backoff with a cap** (e.g. 1s → 2s → 4s → … → 60s). Reset to 1s on `IP_EVENT_STA_GOT_IP`.
- The application stays operational during disconnect — non-network logic must not gate on Wi-Fi state.
- On reconnect, post an app event so the rest of the system can flush queued work, re-fetch state, etc.
- Don't bring up TLS/HTTP until `IP_EVENT_STA_GOT_IP` has fired.

---

## 10. HTTP client (esp_http_client)

- **Always** set `.timeout_ms` (e.g. 5000). The default is too generous for a battery device.
- TLS: use `esp_crt_bundle_attach = esp_crt_bundle_attach` with `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE`, or pin a specific CA via `.cert_pem`. Don't ship `.skip_cert_common_name_check = true` or other "make it work" hacks.
- Build JSON with cJSON. Free with `cJSON_Delete` in **every** branch — including errors. A `goto cleanup;` pattern is clearer than nested ifs:

```c
cJSON *root = cJSON_CreateObject();
char *body = NULL;
if (!root) { err = ESP_ERR_NO_MEM; goto cleanup; }
// ... populate ...
body = cJSON_PrintUnformatted(root);
if (!body) { err = ESP_ERR_NO_MEM; goto cleanup; }
// ... do request ...

cleanup:
    cJSON_Delete(root);
    free(body);
    return err;
```

- Parse responses by status class first (`status / 100 == 2`), then by specific code.
- All outbound HTTP work goes through a request queue consumed by one networking task. UI/sensor code never blocks on `esp_http_client_perform`.
- Reuse a single client across requests when talking to the same host (saves TLS handshakes), or create per-request — pick one and stick to it in the component's API.

---

## 11. Memory & stack discipline

- **Static allocation by default.** `malloc` / `calloc` only when size is genuinely runtime-determined (cJSON output buffer, dynamic message buffers).
- No `malloc` in ISRs, in event handlers, or in tight hot loops. If a fixed-size pool fits, use it.
- Heap budget: keep ≥ 40 KB free at steady state on plain ESP32. Check with `esp_get_free_heap_size()` and log it on major state transitions during bring-up.
- Internal RAM is tight. With PSRAM, push large allocations there:
  ```c
  void *buf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  ```
  Without PSRAM, size buffers down — don't pretend.
- Strings: prefer fixed `char buf[N]` + `snprintf` over dynamic concatenation. Always check the `snprintf` return for truncation when building HTTP bodies / log messages of unknown length.
- Look at `idf.py size`, `size-components`, and `size-files` to find bloat. `CONFIG_LOG_DEFAULT_LEVEL_*` and `CONFIG_COMPILER_OPTIMIZATION_*` are the biggest knobs.

---

## 12. Time

- Sync system time via SNTP (`esp_sntp_*`) after first IP. Don't persist wall-clock time across reboots; re-sync.
- Use `clock_gettime(CLOCK_MONOTONIC, …)` for elapsed time, **never** wall-clock subtraction (NTP can step the clock).
- Reentrant variants only in multitask code: `gmtime_r`, `localtime_r`, `strtok_r`. The non-`_r` versions use shared static buffers and bite eventually.
- If logic depends on a synced clock, defer it until SNTP completes — don't act on the default 1970 timestamp.

---

## 13. Configuration & secrets

- Tunable parameters go in `Kconfig.projbuild` so they show up in `menuconfig`. Constants users shouldn't touch live in a header.
- Don't hardcode credentials. Use NVS, `esp_secure_cert`, or `Kconfig` defaults that get overridden in `sdkconfig.defaults.<env>`.
- Production builds: `CONFIG_LOG_DEFAULT_LEVEL_INFO`, `CONFIG_COMPILER_OPTIMIZATION_SIZE`, no debug-only features.
- Watchdog: `CONFIG_ESP_TASK_WDT` enabled, idle tasks added (already default in recent IDF), application tasks that may starve register themselves explicitly.
- Brownout detector: leave `CONFIG_ESP_BROWNOUT_DET` enabled. Disabling it because brownouts trigger means you're not solving the real problem.

---

## 14. Reserve OTA partitions on day one

Even if OTA isn't in the first release, set up `partitions.csv` with `factory` + `ota_0` + `ota_1` slots from the start. Re-partitioning deployed devices is painful. The default partition table works for prototypes; production should ship a custom one.

---

## 15. Debugging & observability

- `idf.py monitor` decodes panic backtraces automatically. Keep `CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT` (default) on — backtraces are gold.
- Core dumps to flash (`CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH`) for production debugging — pull with `idf.py coredump-info`.
- `heap_caps_check_integrity_all(true)` in development builds catches heap corruption near the source. Compile out for production (it's slow).
- `CONFIG_FREERTOS_USE_TRACE_FACILITY` + `vTaskList` / `vTaskGetRunTimeStats` for "where is CPU time going" investigations.

---

## 16. PR checklist

Before merging:

1. Builds cleanly with `-Wall -Wextra -Werror`.
2. No new `printf`, no `ESP_LOGx` printing secrets.
3. Every `xTaskCreate` has a stack size you've justified (measured high-water mark).
4. Every `ESP_ERROR_CHECK` is on a call that genuinely should never fail in operation.
5. Every `malloc` has a matching `free` in every branch.
6. Header includes `#pragma once` and `extern "C"` guard if it's public.
7. `static` on everything not exported.
8. Free heap and stack high-water mark logged at least once during bring-up.
9. ESP-IDF version pinned in CI matches the one used locally.