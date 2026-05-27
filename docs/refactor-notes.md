# Refactor Notes

This document keeps deferred refactor candidates that should not interrupt the current feature work.
Treat these notes as backlog context, not active implementation tasks.

## Maintenance Rule

- When a refactor or fix note is fully completed, remove that completed note from this document in the same PR.
- If only part of a note is completed, delete the completed bullets and keep only the remaining work.
- If the completed work creates a reusable reliability principle, move that principle to `docs/error-handling.md` with the PR source instead of keeping it here.
- Do not keep completed items as a changelog; GitHub issues, PRs, and git history are the source of completed work.

## After Core Feature Completion

These refactors should be considered after the core medication event / reporting flow is complete.

### Move task creation behind module-level start functions

- Current direction: keep `app_main()` focused on boot orchestration only.
- Candidate shape:
  - `wifi_heartbeat_start()`
  - `rtc_sync_start()`
  - `dispense_init()`
  - `motor_task_start()`
  - `schedule_task_start()`
- Motivation:
  - Keep task ownership inside the component that owns the task.
  - Reduce `app_main()` growth as more modules are added.
  - Make task creation failure handling easier to keep near component setup.
- Validation criteria when implemented:
  - Boot logs still show NVS, Wi-Fi, RTC, dispense, motor, and schedule startup in the expected order.
  - Task creation failures are logged by the owning module and propagated to `app_main()`.
  - No task starts before its required queue/event/driver dependency is initialized.

### Define task priority policy

- Current situation: several application tasks use priority `5`.
- Candidate direction:
  - Define named priority constants by responsibility instead of copying numeric values.
  - Separate network/RTC/schedule/motor priorities only after the core behavior is stable enough to validate.
- Motivation:
  - Make scheduling intent explicit.
  - Avoid accidental priority ties as MQTT, event reporting, real sensor, or hardware control tasks are added.
  - Keep hardware-control and queue-consumer timing decisions visible.
- Validation criteria when implemented:
  - Logs show schedule events are still dispatched at the expected interval.
  - `motor_task` consumes dispense events without unexpected delay under Wi-Fi/HTTP activity.
  - Wi-Fi heartbeat and RTC sync retries do not starve schedule or motor work.
  - No busy loop or watchdog risk is introduced.
