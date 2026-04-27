## Review Rules

- All responses MUST be in Korean.
- Prioritize correctness, safety, and reliability over style.
- Verify that implementation matches the intended behavior.
- Always point out concurrency, memory, and hardware risks.
- Provide file + line references when possible.

---

## Review Priorities

### Critical
- Credentials (WiFi, token, key) in code or logs
- Buffer overflow / invalid memory access
- Blocking or unsafe logic in ISR / callbacks
- Missing delay → busy loop / watchdog risk
- Hardware unsafe behavior (motor/lock misfire)

### High
- Race condition between tasks (shared state without protection)
- Missing retry / timeout for WiFi or server
- `esp_err_t` not checked
- Schedule executed multiple times (duplicate trigger)
- NVS read/write failure not handled

### Medium
- Missing edge case handling (network loss, reboot, RTC not synced)
- Excessive logging
- Hardcoded values (URL, time, pin, UUID)
- Weak error handling (log only, no recovery)

### Low
- Naming / readability
- Minor duplication

---

## Key Checks

### 1. Intent vs Behavior
- Does the code actually implement the feature spec?
- Any mismatch in schedule logic, locking logic, or retry behavior?

### 2. Concurrency (FreeRTOS)
- Shared state protected (mutex / queue / event)?
- Multiple tasks modifying same variable?
- Missing `vTaskDelay()` in loops?

### 3. Memory Safety
- `malloc` / `calloc` result checked?
- Buffer overflow risk (`sprintf`, `strcpy`)?
- Memory freed on all paths?

### 4. Error Handling
- All `esp_err_t` checked?
- Failure paths handled or ignored?
- Retry / fallback exists?

### 5. Network Robustness
- Timeout set?
- Retry with limit/backoff?
- Offline case handled?

### 6. RTC / Scheduler
- Duplicate execution possible?
- RTC invalid or unsynced case handled?
- Time boundary edge cases considered?

### 7. Hardware Control
- Motor/solenoid failure handled?
- Safe fallback (e.g., auto-close)?
- Command executed only once?

### 8. Logging
- No sensitive data
- No excessive logs in loops

---

## Red Flags (Must Call Out)

- Infinite loop without delay
- BLE/WiFi callback doing heavy work
- Direct hardware control without state validation
- Duplicate schedule trigger 가능 코드
- Error ignored (`ESP_OK` 체크 없음)

---

## Review Output Format

[Severity] Short summary  
File: path  
Line: Lx-Ly  

Problem:  
Why it matters:  
Suggested fix: