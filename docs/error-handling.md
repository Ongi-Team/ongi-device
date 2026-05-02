# Error Handling Guidelines

## General Principles

- Do not ignore return values from ESP-IDF, FreeRTOS, HTTP, Wi-Fi, or NVS APIs.
- Prefer returning `esp_err_t` from initialization functions instead of aborting with `ESP_ERROR_CHECK()` when the caller can decide how to recover.
- Do not cast return values just to “make the type fit.” If the type differs, check whether the API returns an error code, pointer, handle, or status.
- Validate pointer-returning APIs with `NULL` checks, not `ESP_OK` comparisons.
- Fail early during startup if required infrastructure is missing.

#### Sources

- [#8] Feat: Verify Wi-Fi connection and heartbeat transmission

## Memory And Resource Safety

- If allocation succeeds, clean it up on later initialization failure.
- If allocation fails and returns `NULL`, do not delete or dereference that handle.
- After deleting a global/static handle, set it back to `NULL`.
- Keep buffers alive until the API is done using them. For example, POST body memory must remain valid until `esp_http_client_perform()` completes.
- Check `snprintf()` result to detect formatting errors or truncated payloads.

#### Sources

- [#8] Feat: Verify Wi-Fi connection and heartbeat transmission

## FreeRTOS / Concurrency

- Check `xEventGroupCreate()` before using the EventGroup.
- Check `xTaskCreate()` result before assuming a task is running.
- Do not pass NULL or deleted handles to `xEventGroupWaitBits()`, `xEventGroupSetBits()`, or `xEventGroupClearBits()`.
- Event callbacks should stay lightweight. Log failures, set bits, clear bits, or enqueue work, but avoid heavy blocking logic.
- Loops in tasks must include blocking waits or `vTaskDelay()` to avoid busy loops and watchdog risk.

#### Sources

- [#8] Feat: Verify Wi-Fi connection and heartbeat transmission

## Wi-Fi / Network

- Wi-Fi initialization naturally has many steps; keep the order explicit.
- Check each Wi-Fi setup step separately so the failing stage is visible.
- `esp_netif_create_default_wifi_sta()` returns a pointer, not `esp_err_t`.
- `esp_wifi_connect()` can fail even inside event callbacks; log the failure.
- HTTP transport success does not mean application success. Check HTTP status codes, especially 4xx/5xx.

#### Sources

- [#8] Feat: Verify Wi-Fi connection and heartbeat transmission

## Security / Hardware Notes

- Do not log credentials, tokens, Wi-Fi passwords, or sensitive payloads.
- Avoid sending partial or unauthenticated requests if header setup fails.
- Network/heartbeat failures should not trigger unsafe hardware behavior.
- If future logic controls motors/locks, validate state before action and ensure commands cannot run twice from duplicate events.

#### Sources

- [#8] Feat: Verify Wi-Fi connection and heartbeat transmission
