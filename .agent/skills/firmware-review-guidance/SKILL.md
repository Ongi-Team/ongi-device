---
name: firmware-review-guidance
description: Implementation direction, analysis, debugging, and review guidance for the Ongi ESP32 firmware. Use when the user asks what to implement, where a bug is, how to fix an error, whether current implementation is safe, or asks for review/debugging/analysis in this repository.
---

# Ongi Device Firmware Review Guidance

이 스킬은 `Ongi-Team/ongi-device` 리포지토리에서 구현 방향 제안, 분석, 디버깅, 리뷰를 할 때 사용하는 기준이다.

## Operating Boundary

- 모든 응답은 한국어로 작성한다.
- 사용자가 직접 수정, 구현, 파일 변경을 명시하지 않는 한 코드와 문서를 고치지 않는다.
- 기본 응답은 원인 분석, 구현 방향, 위험도, 검증 방법이다.
- 가능한 경우 파일과 라인 번호를 함께 제시한다.
- 판단 근거는 ESP-IDF, FreeRTOS 같은 공식 문서 또는 널리 쓰이는 임베디드 시스템 관례에 둔다.

## Current Implementation Snapshot

현재 펌웨어는 ESP32 / ESP-IDF 기반 C 프로젝트이며, `main/`과 `components/`로 분리되어 있다.

- `main/main.c`: NVS, Wi-Fi, RTC, heartbeat task, dispense queue, motor task, schedule task를 순서대로 초기화한다.
- `components/wifi`: Wi-Fi 연결, event group 기반 연결 상태 관리, HTTP heartbeat POST, 재시도, SNTP sync task 시작을 담당한다.
- `components/rtc`: internal RTC driver interface, SNTP 동기화 task, time validity check를 담당한다.
- `components/schedule`: RTC sync 이후 hardcoded test slots를 만들고 1초마다 schedule match를 확인하여 dispense event를 enqueue한다.
- `components/dispense`: `DispenseEvent` queue를 만들고 producer인 schedule task와 consumer인 motor task를 연결한다.
- `components/motor`: `dispense_queue`를 block wait하고 servo open, intake wait, servo close 순서를 실행한다.
- `components/intake`: `IntakeDetector` interface와 dummy timeout implementation을 제공한다.
- `components/motor`: `ServoDriver` interface, factory, dummy servo implementation을 제공한다.

현재 구현은 아직 테스트용 hardcoded schedule, dummy intake detector, dummy servo driver 단계다. 실제 센서, 모터, 서버 연동이 들어오면 안전 조건과 실패 복구 조건을 먼저 확정해야 한다.

## Analysis Order

사용자가 "어떻게 해야 돼", "어디가 문제야", "에러 잡아줘", "리뷰해줘"라고 물으면 다음 순서로 본다.

1. 의도 확인: 이 변경이 어떤 기능 명세나 이슈 TODO를 구현하려는 것인지 확인한다.
2. GitHub 맥락 확인: 이슈/브랜치/PR/리뷰 흐름은 `github-workflow`의 맥락 복원 순서를 따른다.
3. 현재 상태 확인: `git status`, 변경 diff, 관련 task/component 파일을 확인한다.
4. 실행 흐름 확인: producer/consumer, task 생성 순서, callback, queue, event bit, timeout 경로를 따라간다.
5. 실패 경로 확인: `esp_err_t`, `NULL`, FreeRTOS `pdPASS/pdTRUE`, HTTP status, NVS failure, queue full을 확인한다.
6. 시간/중복 확인: RTC sync 전 동작, reboot, 자정 경계, 같은 schedule의 중복 enqueue 가능성을 확인한다.
7. 하드웨어 안전 확인: servo/motor/lock 동작이 실패하거나 중복 호출될 때 안전한 상태로 돌아오는지 확인한다.
8. 검증 제안: 빌드, 로그, 단위 수준 테스트, 보드 검증 중 무엇이 필요한지 구분한다.

## Work Unit And Log Verification

이 리포지토리는 작업 완료 주기를 짧게 유지하기 위해 기능을 작은 이슈와 작은 PR로 나눈다. 기술 제안이나 리뷰를 할 때도 같은 단위를 유지한다.

- 제안하는 구현 단위는 가능한 한 "로그로 완료 여부를 확인할 수 있는 최소 단위"여야 한다.
- 한 PR이 여러 독립 흐름을 검증해야 하면 메인 이슈 아래 서브 이슈로 나누는 방향을 제안한다.
- 실제 하드웨어가 준비되지 않은 단계에서는 interface, dummy implementation, queue/task 연결, timeout path를 각각 로그 검증 가능한 단위로 끊는다.
- 커밋 단위는 가능한 한 빌드 가능한 최소 단위로 잡는다. 이슈 하나에 여러 커밋이 생기는 것은 정상적인 흐름이다.
- 구현 순서를 제안할 때는 각 단계가 빌드 가능한 커밋으로 닫힐 수 있는지도 함께 본다.
- 작업을 안내할 때는 "여기까지 구현하고 빌드가 통과하면 커밋하는 것이 좋다"처럼 커밋 경계를 함께 제안한다.
- 커밋 경계에는 영어 Conventional Commit 메시지를 함께 제안한다. 예: `feat: add medication event queue #27`.
- 로그 검증 기준은 PR `Etc`의 `Verification Criteria`에 들어갈 수 있도록 짧은 bullet로 정리한다.
- 실제 스크린샷이나 첨부 여부는 `Verification Evidence`로 분리한다.
- 검증 bullet은 "무엇이 로그에 보이면 완료인가"를 적고, 구현 설명을 길게 반복하지 않는다.

좋은 로그 검증 bullet 예시:

```md
- schedule match 이후 `dispense_enqueue()` 성공 로그가 slot별로 1회씩 출력된다.
- `motor_task`가 같은 slot event를 dequeue하고 servo open → intake wait → servo close 순서로 출력한다.
- queue enqueue 실패 시 `triggered`가 설정되지 않아 다음 tick에서 재시도 가능하다.
- heartbeat HTTP request가 2xx status code로 완료되고 retry 로그가 반복되지 않는다.
- 자정 이후 triggered flag reset 로그가 출력되고 다음 날짜 slot이 다시 실행 가능하다.
```

## Deferred Refactor Notes

핵심 기능 완료 후 검토할 구조 개선 메모는 `docs/refactor-notes.md`에 둔다.

- 이 문서는 현재 기능 PR에 섞지 않을 리팩토링 후보를 보관하는 곳이다.
- 사용자가 리팩토링 방향을 물으면 먼저 `docs/refactor-notes.md`를 확인한다.
- 문서에 있는 후보라도 사용자가 명시적으로 요청하기 전에는 직접 구현하지 않는다.
- 리팩토링 또는 fix가 완료되면 완료된 항목은 같은 PR에서 `docs/refactor-notes.md`에서 삭제한다.
- 일부만 완료되면 완료된 bullet만 삭제하고 남은 작업만 유지한다.

## Review Checklist By Subsystem

### Wi-Fi / HTTP / Heartbeat

- Wi-Fi event handler는 event bit 설정/해제, reconnect 요청, 짧은 로그만 수행해야 한다.
- HTTP 요청은 task context에서 수행하고 timeout을 명시한다.
- `esp_http_client_perform()` 성공만으로 서버 성공으로 보지 말고 status code를 확인한다.
- heartbeat 실패는 제한된 retry/backoff를 사용하고, offline 상태에서 무한 누적하지 않는다.
- Wi-Fi password, device token, full payload 등 민감 정보는 로그에 남기지 않는다.

### RTC / SNTP / Schedule

- schedule task는 RTC sync 전에는 dose trigger를 시작하지 않는다.
- `rtc_driver_get_time()` 반환값을 확인하고 invalid time을 정상 시간처럼 쓰지 않는다.
- schedule match는 같은 slot이 같은 날 여러 번 enqueue되지 않도록 상태를 둔다.
- 자정, 재부팅, SNTP time step, queue enqueue 실패 시 trigger flag 처리 순서를 확인한다.
- elapsed time은 wall clock step의 영향을 받지 않도록 필요 시 monotonic time을 쓴다.

### FreeRTOS Concurrency

- 무한 loop는 queue/event wait 또는 `vTaskDelay()`로 CPU를 양보해야 한다.
- queue는 producer와 consumer 관계가 명확해야 하며, 한 queue를 여러 consumer가 경쟁해 이벤트가 분산되지 않도록 한다.
- shared global state는 single owner, queue/event, mutex 중 하나로 보호한다.
- task handle 같은 shared state는 여러 event callback/task에서 동시에 접근할 수 있는지 확인한다.
- task 생성 실패 시 이후 컴포넌트가 잘못된 handle을 사용하지 않도록 한다.

### Motor / Intake / Hardware Safety

- servo `open`/`close` 반환값을 확인하고 실패 시 close/fallback/report 경로를 둔다.
- intake timeout은 정상 결과인지 오류인지 구분한다. MISSED와 SENSOR_FAIL을 섞지 않는다.
- 실제 motor/lock 제어가 들어오면 command idempotency, duplicate trigger, retry limit, safe state를 먼저 설계한다.
- slot id 범위 검증을 추가한다. 잘못된 slot id로 hardware command가 나가지 않아야 한다.
- callback/ISR에서 직접 hardware sequence를 실행하지 않는다. queue로 task에 위임한다.

### Memory / Stack / Resource Lifetime

- `malloc`, `calloc`, `xQueueCreate`, `xEventGroupCreate`, `esp_http_client_init` 반환값을 확인한다.
- cleanup path에서 생성한 리소스를 모두 해제하고 global/static handle은 `NULL`로 되돌린다.
- stack size는 HTTP/TLS/SNTP task와 sensor/control task를 분리해서 측정한다.
- `snprintf()` 결과를 확인해 truncation을 잡는다.
- request body buffer는 HTTP request가 끝날 때까지 살아 있어야 한다.

## Output Style

리뷰 결과는 위험도 높은 항목부터 쓴다.

```text
[Severity] Short summary
File: path
Line: Lx-Ly

Problem:
Why it matters:
Suggested fix:
```

구현 방향 제안은 다음 형식을 선호한다.

```text
추천 방향:
- ...

먼저 확인할 것:
- ...

주의할 위험:
- 동시성:
- 메모리:
- 하드웨어:
```

## Evidence Sources

이 스킬의 판단 기준은 아래 문서에 근거한다.

- ESP-IDF Error Handling: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/error-handling.html
- ESP-IDF Event Loop: https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32/api-reference/system/esp_event.html
- ESP-IDF HTTP Client: https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/protocols/esp_http_client.html
- ESP-IDF Watchdogs: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/wdts.html
- ESP-IDF Heap Memory Allocation: https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/system/mem_alloc.html
- ESP-IDF FreeRTOS API: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos_idf.html
- ESP-IDF System Time / SNTP: https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/system_time.html

## Error Handling Document Maintenance

`docs/error-handling.md`는 유지한다. 다만 모든 리뷰 체크리스트를 복사해 넣는 문서가 아니라, 실제 PR/리뷰에서 반복해서 확인된 에러 처리 원칙을 축적하는 문서로 관리한다.

- 새 원칙은 실제 PR, 이슈, 리뷰 코멘트, 재현된 장애 중 하나가 근거일 때만 추가한다.
- 각 topic은 `#### Sources`에 관련 PR 번호를 남긴다.
- 특정 컴포넌트 구현 세부사항보다 재사용 가능한 원칙을 기록한다.
- 일반 ESP-IDF 상식은 이 스킬에 두고, 프로젝트에서 반복되는 결정만 `docs/error-handling.md`에 승격한다.
- 더 이상 현재 구조와 맞지 않는 원칙은 삭제하지 말고, 해당 PR에서 왜 바뀌었는지 Sources를 갱신하며 수정한다.
