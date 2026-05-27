---
name: github-workflow
description: Issue, branch, commit, push, and pull request workflow rules for the Ongi device repository. Use when the user asks to create or read an issue, create or inspect a branch, write or make a commit, push a branch, open or update a PR, or continue a human-created GitHub workflow in this repository. Pair with firmware-review-guidance for implementation direction, debugging, and code review.
---

# Ongi Device GitHub Workflow

이 스킬은 `Ongi-Team/ongi-device` 리포지토리에서 이슈 생성, 브랜치 생성, 커밋, PR 작성 흐름을 맞추기 위한 규칙이다.

## Operating Boundary

- 모든 응답은 한국어로 작성한다.
- 사용자가 명시적으로 "수정해줘", "구현해줘", "파일을 바꿔줘"처럼 직접 변경을 요청하지 않는 한, 코드와 문서는 직접 수정하지 않는다.
- 기본 역할은 이슈, 브랜치, 커밋, 푸시, PR의 맥락을 복원하고 같은 절차로 이어가는 것이다.
- 사용자가 "이슈 생성", "브랜치 생성", "커밋", "푸시", "PR 작성"을 명시하면 해당 Git/GitHub 작업은 수행할 수 있다.
- 사용자가 중간에 "이거 어떻게 해야 돼", "어디가 문제야", "뭐가 문제야", "에러 잡아줘"라고 물으면 먼저 원인과 방향을 제시한다. 직접 코드를 고치는 것은 별도 명령이 있을 때만 한다.
- 리뷰나 디버깅에서는 항상 FreeRTOS 동시성, 메모리 안전, 네트워크/타임아웃, RTC/스케줄 중복 실행, 하드웨어 오동작 위험을 확인한다.
- 구현 방향, 디버깅, 코드 리뷰 판단은 `firmware-review-guidance`를 함께 따른다.

## Observed Repository Patterns

### Issue

기존 이슈는 GitHub 템플릿을 따른다.

```md
## 📌 Issue
- 작업 설명

## ✅ TODO
- [ ] 작업 항목
- [ ] 작업 항목
```

- 제목은 타입을 대문자로 시작한다: `Feat: ...`, `Fix: ...`, `Refactor: ...`, `Docs: ...`, `Chore: ...`.
- 기능 구현은 보통 `Feat:`, 버그는 `Fix:` 또는 `[Bug]`, 구조 변경은 `Refactor:`, 문서는 `Docs:`, 설정/잡무는 `Chore:`를 쓴다.
- 가능한 경우 TODO는 검증 가능한 작은 단위로 쪼갠다.
- 큰 기능 이슈는 하위 이슈 번호를 TODO에 연결할 수 있다: `- [ ] #27`.
- 이 리포지토리는 메인 이슈와 서브 이슈 구조를 적극 사용한다. 메인 이슈는 큰 기능 목표와 아키텍처 방향을 담고, 서브 이슈는 PR 하나로 끝낼 수 있는 작은 작업 단위로 둔다.
- 작업 단위는 "로그로 완료 여부를 확인할 수 있는 최소 단위"를 선호한다. 예: interface 정의, dummy driver 추가, queue 생성, task에서 enqueue/dequeue 연결, timeout 처리, 중복 trigger 방지.
- 커밋 단위도 작게 가져간다. 이슈 하나에 여러 커밋이 있어도 괜찮으며, 각 커밋은 가능한 한 빌드 가능한 최소 단위여야 한다.
- 새 이슈를 만들 때 작업이 크면 바로 구현 이슈 하나로 만들지 말고, 메인 이슈와 서브 이슈로 나눌 수 있는지 먼저 판단한다.
- 서브 이슈 TODO는 각 항목이 로그 검증 또는 작은 코드 diff로 닫힐 수 있게 작성한다.
- 라벨은 기존 관례를 따른다.
  - `Feat`, `Refactor`: `enhancement`
  - `Fix`, `[Bug]`: `bug`
  - `Docs`: `documentation`
  - `Chore`: 라벨 없음 또는 필요 시 기존 라벨만 사용

### Main Issue / Sub Issue

메인 이슈는 기능 묶음, 서브 이슈는 짧은 완료 주기를 위한 실제 작업 단위다.

메인 이슈에 적는 것:

- 최종 목표와 전체 흐름
- 서브 이슈 번호 목록
- 아직 구현하지 않을 Future Extensions
- 컴포넌트 간 관계나 queue/task/message 흐름

서브 이슈에 적는 것:

- PR 하나로 완료할 수 있는 작은 변경
- 로그로 검증 가능한 완료 조건
- 필요한 interface, dummy implementation, factory, queue, task 연결 등 좁은 범위
- hardware-safe fallback, duplicate trigger, timeout 같은 해당 단위의 위험
- 여러 개의 빌드 가능한 작은 커밋으로 나눌 수 있는 구현 순서

이슈 분리 판단 기준:

- PR 설명이 길어지거나 여러 로그 흐름을 한 번에 검증해야 하면 서브 이슈로 나눈다.
- 실제 하드웨어 없이 stub/dummy 로그로 검증할 수 있는 단계는 별도 서브 이슈로 끊는다.
- 메인 이슈는 여러 서브 이슈가 닫히면서 함께 닫는다. PR이 여러 이슈를 닫을 때는 `Closes #서브이슈`와 필요한 경우 `Closes #메인이슈`를 함께 적는다.
- 서브 이슈가 메인 이슈의 TODO에 연결되어 있으면, PR 작성 전에 메인 이슈의 남은 TODO도 확인한다.

### Branch

브랜치는 깨끗한 `main`에서 만든다.

1. `git status --short --branch`로 작업 트리가 깨끗한지 확인한다.
2. `main`으로 이동하고 `origin/main`과 동기화한다.
3. 다시 작업 트리가 깨끗한지 확인한다.
4. 이슈 번호와 타입에 맞춰 브랜치를 만든다.

브랜치명 패턴:

```text
<type><issue-number>/<short-slug>
```

예시:

```text
feat03/server-connection
fix06/heartbeat-stack-overflow
docs09/documentation
refactor13/components
feat22/motor-task
refactor24/servo-driver
```

- `type`은 `feat`, `fix`, `docs`, `chore`, `refactor`, `ci` 중 기존 커밋 타입과 맞는 값을 사용한다.
- 이슈 번호가 한 자리면 두 자리처럼 `03`, `06`, `09` 형태를 쓴다.
- slug는 소문자 kebab-case로 작성한다.
- `main`이 더럽거나 사용자의 미커밋 변경이 있으면 브랜치를 만들기 전에 멈추고 상황을 설명한다.

### Commit

커밋 메시지는 Conventional Commit 형태에 이슈 번호를 붙인다.

```text
<type>: <summary> #<issue-number>
```

예시:

```text
feat: define ServoDriver interface #24
fix: move SNTP sync delay out of Wi-Fi event callback #22
refactor: decouple motor task from servo implementation #24
docs: add error handling principles #9
ci: define CI_BUILD via CMakeLists instead of EXTRA_CFLAGS #3
```

- `type`은 `feat`, `fix`, `refactor`, `docs`, `chore`, `build`, `ci`를 우선 사용한다.
- summary는 소문자로 시작하는 명령형/변경 설명형 영어 문장을 사용한다.
- 하나의 커밋은 하나의 의미 있는 변경 단위로 유지한다.
- 커밋은 가능한 한 빌드 가능한 최소 단위로 쪼갠다.
- 이슈 하나에 커밋이 여러 개 생기는 것은 정상적인 흐름이다. 예: interface 정의 → dummy implementation → factory 추가 → task 연결 → 오류 처리 보강.
- 중간 커밋이 빌드 불가능한 상태를 만들지 않도록 한다. 피하기 어려운 경우 다음 커밋에서 즉시 복구하고 PR 설명에 맥락을 남긴다.
- 작업을 안내할 때는 적절한 커밋 경계를 함께 제안한다. 예: "여기까지 구현하고 빌드가 통과하면 커밋하는 것이 좋다."
- 커밋 경계를 제안할 때는 커밋 메시지 컨벤션에 맞는 영어 메시지도 함께 적는다.
- 사용자가 "현재 변경 사항 커밋 메시지 작성해줘" 또는 "커밋해줘"라고 하면 먼저 `git diff`와 `git status`로 변경 내용을 확인한 뒤, 규칙에 맞는 메시지를 만들고 커밋한다.
- 사용자가 명시하지 않은 파일 수정, 되돌리기, 정리 작업은 커밋에 포함하지 않는다.

### Pull Request

PR은 `.github/pull_request_template.md` 형식을 따른다.

```md
## 📌 Related Issue

Closes #이슈번호

## 🚀 Description

- 변경 요약

## 📢 Review Point

- 리뷰어가 집중해서 봐야 할 위험 지점

## 📚 Etc (선택)

### Verification Criteria
- 작업 완료로 판단할 로그 조건

### Verification Evidence
- 로그 검증 스크린샷 첨부 예정
```

- PR 제목은 이슈 제목 스타일과 맞춘다: `Feat: ...`, `Fix: ...`, `Refactor: ...`, `Docs: ...`.
- `Related Issue`에는 기본적으로 `Closes #번호`를 쓴다. 여러 이슈를 닫으면 줄을 나눠 모두 적는다.
- `Description`에는 구현한 동작을 요약한다.
- `Review Point`에는 이 리포지토리에서 위험한 지점을 우선 적는다.
  - FreeRTOS task/queue/event 동시성
  - `esp_err_t` 처리와 복구 경로
  - Wi-Fi/HTTP timeout, retry, offline 동작
  - RTC sync, schedule duplicate trigger, midnight boundary
  - motor/servo/lock 등 하드웨어 안전 동작
  - 메모리 할당, buffer, stack size
- `Etc`에서는 검증 기준과 검증 증거를 구분한다.
- `Verification Criteria`에는 어떤 로그가 보이면 작업 완료로 판단할 수 있는지 bullet 형식으로 짧게 적는다.
- `Verification Evidence`에는 실제 로그 검증 스크린샷 또는 스크린샷 첨부 예정 메모를 둔다.
- PR에는 반드시 로그 검증 스크린샷이 들어간다. 이미지 파일과 실제 첨부는 사용자가 직접 반영할 수 있으므로, 에이전트는 `Verification Criteria`에 완료 조건을 명확히 남긴다.
- 스크린샷이 아직 없으면 `Verification Evidence`에 `- 로그 검증 스크린샷 첨부 예정`처럼 자리만 남긴다.
- 로그 검증 bullet은 구현 단위에 맞게 작게 쓴다. 예: schedule match, queue enqueue, motor dequeue, servo open/close, intake timeout, HTTP status, retry/backoff, duplicate trigger 없음.

## Standard Workflow

### 0. 사람이 중간에 개입한 작업을 이어받는 경우

이 리포지토리의 작업은 에이전트가 처음부터 만든 흐름이 아니어도 같은 규칙을 따른다. 사람이 이슈, 브랜치, 커밋, PR을 먼저 만들었을 수 있으므로, 항상 현재 맥락을 먼저 복원한다.

맥락 복원 우선순위:

1. 사용자가 이슈 번호를 언급하면 해당 이슈를 읽고 제목, 본문, TODO, 라벨, 연결된 PR을 확인한다.
2. 현재 브랜치가 `feat27/...`, `fix06/...`, `refactor24/...`처럼 번호를 포함하면 그 번호를 이슈 번호로 보고 이슈를 읽는다.
3. 현재 브랜치에 연결된 PR이 이미 있으면 PR 본문, 커밋, diff, 리뷰 코멘트, CI 상태를 읽는다.
4. 작업 트리에 변경 사항이 있으면 `git status`와 `git diff`로 사람이 진행한 변경을 먼저 파악한다.
5. 이슈, 브랜치, PR, diff가 서로 어긋나면 임의로 정리하지 말고 어떤 맥락이 충돌하는지 설명한다.

사람이 만든 작업을 이어받을 때의 원칙:

- 사람이 만든 이슈도 에이전트가 만든 이슈와 동일하게 취급한다.
- 사람이 만든 브랜치도 브랜치명에서 이슈 번호와 작업 타입을 추론해 맥락을 파악한다.
- 이미 push/PR 생성이 끝난 브랜치라면 리뷰 반영 중일 가능성이 높으므로 PR을 읽고 남은 리뷰 코멘트와 변경 요청을 우선 확인한다.
- 사람이 만든 커밋이나 변경 사항을 되돌리지 않는다.
- 사용자가 직접 수정하라고 하지 않으면 분석과 제안만 한다.
- 어느 에이전트가 수행하더라도 같은 결과가 나오도록 이슈 → 브랜치 → PR → diff 순서로 근거를 확인한다.

### 1. 사용자가 원하는 기능을 말하며 이슈 생성을 요청한 경우

1. 요구사항을 이슈 제목과 TODO로 정리한다.
2. 작업이 PR 하나로 로그 검증 가능한 최소 단위인지 판단한다.
3. 작업이 크면 메인 이슈와 서브 이슈로 나누고, 메인 이슈 TODO에 서브 이슈 번호가 들어가도록 구성한다.
4. 타입과 라벨을 기존 패턴에 맞춘다.
5. GitHub 이슈를 생성한다.
6. 생성된 이슈 번호를 사용해 브랜치명을 정한다. 여러 이슈가 있으면 실제로 작업할 서브 이슈 번호를 브랜치명에 사용한다.
7. 깨끗한 최신 `main`에서 브랜치를 생성한다.
8. 브랜치명, 연결된 메인/서브 이슈, 로그 검증 방향을 사용자에게 알려준다.

### 2. 사용자가 구현 중 방향을 물어보는 경우

1. 먼저 "0. 사람이 중간에 개입한 작업을 이어받는 경우"의 맥락 복원 순서를 수행한다.
2. 현재 브랜치, diff, 관련 파일을 확인한다.
3. 구현 의도와 실제 동작이 맞는지 검토한다.
4. 문제 원인과 추천 수정 방향을 제시한다.
5. 어디까지 구현한 뒤 커밋하면 좋은지 빌드 가능한 커밋 경계를 제안한다.
6. 제안한 커밋 경계에 맞는 영어 Conventional Commit 메시지를 함께 적는다.
7. 필요한 경우 파일과 라인 번호를 포함한다.
8. 직접 수정은 사용자가 명시적으로 요청할 때만 한다.

### 3. 사용자가 현재 변경 사항 커밋을 요청한 경우

1. `git status --short --branch`로 브랜치와 변경 파일을 확인한다.
2. `git diff`로 변경 내용을 확인한다.
3. 관련 이슈 번호를 브랜치명, 사용자 요청, 연결된 PR 중 하나에서 확인한다.
4. 현재 변경이 빌드 가능한 최소 커밋 단위인지 확인한다.
5. 연결된 PR이 있으면 PR 맥락과 리뷰 반영 사항에 맞는 커밋 단위인지 확인한다.
6. 적절한 Conventional Commit 메시지를 작성한다.
7. 사용자가 커밋까지 요청했다면 변경 파일을 stage하고 커밋한다.

### 4. 사용자가 푸시와 PR 생성을 요청한 경우

1. 현재 브랜치가 `main`이 아닌지 확인한다.
2. 같은 브랜치에 PR이 이미 있는지 확인한다.
3. PR이 이미 있으면 새 PR을 만들지 않고 기존 PR을 읽은 뒤 필요한 경우 본문 업데이트나 리뷰 반영 흐름으로 이어간다.
4. 필요하면 빌드/검증 결과를 확인하되, 사용자가 직접 작업한 변경을 임의로 고치지 않는다.
5. 브랜치를 push한다.
6. PR이 없을 때만 PR 템플릿에 맞춰 새 PR 본문을 작성한다.
7. `Etc`에는 `Verification Criteria`와 `Verification Evidence`를 구분해서 적는다.
8. 실제 스크린샷 파일이나 이미지 첨부는 사용자가 직접 반영할 수 있으므로, 에이전트는 `Verification Criteria`에 검증 완료 조건을 명확히 남긴다.
9. 스크린샷이 아직 없으면 `Verification Evidence`에 첨부 예정 메모를 남긴다.
10. PR URL과 요약, 검증 여부를 사용자에게 알려준다.

### 5. 사용자가 이미 열린 PR의 리뷰 반영을 요청한 경우

1. 현재 브랜치명 또는 사용자가 준 PR 번호로 PR을 찾는다.
2. PR 본문, diff, 커밋 목록, 리뷰 코멘트, unresolved thread, CI 상태를 확인한다.
3. 연결된 이슈를 읽고 PR의 목적과 리뷰 요청이 충돌하지 않는지 확인한다.
4. 현재 작업 트리에 사람이 진행한 변경이 있으면 먼저 읽고 이어서 판단한다.
5. 리뷰 코멘트별로 반영 방향, 위험, 검증 방법을 정리한다.
6. 리뷰 반영 후 PR `Etc`의 로그 검증 bullet이 여전히 맞는지 확인하고, 필요한 경우 검증 기준 업데이트를 제안한다.
7. 직접 수정은 사용자가 명시적으로 요청할 때만 한다.

## Safety Checklist

이 리포지토리는 ESP32/ESP-IDF 기반 펌웨어이므로, 제안·리뷰·PR 설명에서 다음 위험을 항상 확인한다.

- 콜백/ISR에서 blocking work, HTTP, heap allocation, heavy log를 수행하지 않는가?
- FreeRTOS task loop에 `vTaskDelay()` 또는 blocking wait가 있어 watchdog busy loop를 피하는가?
- queue/event group/mutex 없이 공유 상태를 여러 task가 읽고 쓰지 않는가?
- `esp_err_t` 반환값을 무시하지 않고 retry, fallback, cleanup을 제공하는가?
- Wi-Fi, HTTP, SNTP 동작에 timeout과 재시도 제한이 있는가?
- RTC 미동기, 재부팅, 자정 경계에서 schedule이 중복 실행되지 않는가?
- motor/servo/lock 동작이 실패했을 때 안전한 fallback이 있는가?
- stack size, malloc/calloc 실패, buffer overflow 위험을 확인했는가?
- Wi-Fi password, token, device secret, 개인 정보가 로그/커밋/이슈/PR에 노출되지 않는가?
