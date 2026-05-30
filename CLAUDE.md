# Agent Instructions

이 리포지토리의 공통 에이전트 지침은 `AGENTS.md`를 따른다.

## Shared Skills

공유 스킬 원본은 `.agent/skills` 아래에 둔다. Claude 호환 경로는 `.claude/skills` 아래에서 같은 스킬을 심볼릭 링크로 가리킨다.

현재 공유 스킬:

- `github-workflow`: `.agent/skills/github-workflow/SKILL.md`
- `firmware-review-guidance`: `.agent/skills/firmware-review-guidance/SKILL.md`

작업 방식:

- 이슈 생성, 브랜치 생성, 커밋, 푸시, PR 작성 요청을 받으면 `github-workflow` 스킬을 먼저 읽고 따른다.
- 구현 방향 제안, 분석, 디버깅, 리뷰 요청을 받으면 `firmware-review-guidance` 스킬을 먼저 읽고 따른다.
- 작업에 GitHub 맥락과 구현 검토가 함께 있으면 두 스킬을 모두 읽고, `github-workflow`는 절차, `firmware-review-guidance`는 기술 판단 기준으로 사용한다.
- 사용자가 명시적으로 직접 수정을 요청하지 않는 한, 구현 방향 제안과 분석만 제공한다.
