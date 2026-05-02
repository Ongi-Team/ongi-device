# Skill: docs/ Conventions

Conventions for maintaining files under `docs/`.
This file is the source of truth for how documents under `docs/` are structured and updated.

## When to use this skill
When asked to create or update any file under `docs/`.

## Section structure
- A file may optionally start with a single level-1 document title.
- Each major topic is a level-1 heading when there is no document title, or a level-2 heading when a document title is present.
- A file may contain one or more topics, 
    and topics may later be split into separate files —
    each split file must remain self-contained with its own Sources section at the bottom.
- Each sub section within a topic uses the next heading level below the topic heading as needed.
- Every topic ends with a `#### Sources` section listing the PRs that
  contributed to that topic's content.

## Sources section format
- Heading: `#### Sources` (level 4), placed at the bottom of each topic.
- Each entry: `- [#<PR number>] <PR title or short description>`
- Entries are listed in ascending PR number order.
- When a PR contributes to multiple topics within the same file, it
  appears in each affected topic's Sources section.

## Update rules
- Add new principles to the most relevant topic, 
    or create a new topic if none fits.
- Prefer adding to an existing topic when the new principle changes behavior in the same subsystem, workflow, or review domain.
- Create a new topic when the principle introduces a new subsystem, recurring workflow, safety domain, or review category that does not fit existing topics.
- When a PR contributes to an existing topic, 
    append a new entry to that topic's `#### Sources` section.
- When changing or removing an existing principle,
    update the relevant `#### Sources` section with the PR that justified the change.
- When creating a new topic, add its `#### Sources` section immediately, 
    even if only one PR contributes initially.
- If a file is split into multiple files, 
    each resulting file keeps its own per-topic Sources sections 
    — Sources do not move to a parent index.

## Required inputs
When invoking this skill, the human provides:
- **PR number** (e.g., `#6`) — used in the topic's Sources section.
- **PR title or short description** — used as the Sources entry text.
- **The principles to add** — explicit list, in the user's words or paraphrased from review feedback.
- **Target topic** (optional) — if the user already knows where it
  belongs; otherwise the agent proposes one.

The agent does NOT:
- Infer PR numbers, titles, or content from git history or open PRs.
- Decide on its own which principles are worth recording.
- Add content not explicitly provided by the user.

If any required input is missing, ask before proceeding.

## Working procedure (for agents)
1. Read the target file in full.
2. Identify the most relevant topic, or propose a new one.
3. Append the new principle in the existing bullet style.
4. Update the topic's `#### Sources` section with the new PR entry, maintaining ascending order by PR number.
5. If creating a new topic, add a `#### Sources` section at its bottom.
6. Keep the diff minimal — do not reformat unrelated topics or sections.
