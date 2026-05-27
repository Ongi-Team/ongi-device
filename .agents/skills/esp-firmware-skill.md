# Legacy Skill Path: esp-idf-conventions

이 파일은 과거 호환 경로다. 새 에이전트 문서의 원본은 `.agent/skills` 아래에 둔다.

## Current Rule

- ESP32 / ESP-IDF 구현 방향, 분석, 디버깅, 리뷰는 `.agent/skills/firmware-review-guidance/SKILL.md`를 따른다.
- 이슈, 브랜치, 커밋, 푸시, PR 작업은 `.agent/skills/github-workflow/SKILL.md`를 따른다.
- 이 파일과 `.agent/skills` 내용이 충돌하면 `.agent/skills`를 우선한다.

## Baseline

일반 ESP-IDF 판단은 공식 문서를 근거로 한다. 대표 기준은 error handling, event loop, HTTP client, watchdog, heap allocation, FreeRTOS API, system time / SNTP 문서다.
