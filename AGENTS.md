# Agent Instructions

## Shared Skills

공유 스킬 원본은 `.agent/skills` 아래에 둔다.
Codex와 Claude 모두 같은 내용을 기준으로 작업해야 하며, Claude 호환 경로는
`.claude/skills` 아래에서 공유 스킬을 심볼릭 링크로 가리킨다.

현재 공유 스킬:

- `github-workflow`: `.agent/skills/github-workflow/SKILL.md`
- `firmware-review-guidance`: `.agent/skills/firmware-review-guidance/SKILL.md`

이슈 생성, 브랜치 생성, 커밋, 푸시, PR 작성 요청을 받으면
`github-workflow` 스킬을 먼저 읽고 따른다.

구현 방향 제안, 분석, 디버깅, 리뷰 요청을 받으면
`firmware-review-guidance` 스킬을 먼저 읽고 따른다.
작업에 GitHub 맥락과 구현 검토가 함께 있으면 두 스킬을 모두 읽고,
`github-workflow`는 절차, `firmware-review-guidance`는 기술 판단 기준으로 사용한다.

중요: 이 리포지토리에서는 사용자가 명시적으로 직접 수정을 요청하지 않는 한,
수정 사항 또는 구현 방향을 제안하고 분석만 한다. 코드/문서 변경은 별도 명령이
있을 때만 수행한다.

---

## Review Rules

- All responses MUST be in Korean.
- Prioritize correctness, safety, and reliability over style.
- Verify that implementation matches the intended behavior.
- Always point out concurrency, memory, and hardware risks.
- Provide file + line references when possible.

---

## Build / Validation

- Before running `idf.py`, source ESP-IDF first:
  `. ~/.espressif/v6.0/esp-idf/export.sh`
- Use the same build command as CI when validating firmware changes:
  `idf.py -DCI_BUILD=1 build`

---

## Pull Request Rules

- All PR titles and descriptions MUST be written in English.
- Keep each PR section concise: `Description`, `Review Point`, and `Etc` should each usually stay within 5 lines.
- PR review points MUST describe the implemented logic reviewers should inspect.
- Do NOT frame speculative risks as PR review points. Potential issues belong in code review findings, not in the author's PR description.
- In the PR `Etc` section, add notes for behavior that should be verified through device logs.
- Do NOT attach log screenshots yourself; the user will add log screenshots directly to the PR.

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
- Code with possible duplicate schedule trigger
- Error return ignored (no `esp_err_t` check)

---

## Review Output Format

[Severity] Short summary  
File: path  
Line: Lx-Ly  

Problem:  
Why it matters:  
Suggested fix:
