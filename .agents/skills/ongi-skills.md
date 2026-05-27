# Legacy Skill Path: ongi-device-conventions

이 파일은 과거 호환 경로다. 새 에이전트 문서의 원본은 `.agent/skills` 아래에 둔다.

## Current Rule

- 구현 방향, 분석, 디버깅, 리뷰는 `.agent/skills/firmware-review-guidance/SKILL.md`를 따른다.
- 이슈, 브랜치, 커밋, 푸시, PR 작업은 `.agent/skills/github-workflow/SKILL.md`를 따른다.
- 이 파일과 `.agent/skills` 내용이 충돌하면 `.agent/skills`를 우선한다.

## Compatibility Note

과거 이 파일에는 아직 구현되지 않은 BLE provisioning, event bus, lockbox, offline queue 같은 미래 도메인 규칙이 포함되어 있었다. 현재 작업 판단에서는 실제 구현 상태와 연결된 공유 스킬을 우선한다.
