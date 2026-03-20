---
description: "Execute the universal phase of the Neutral OS lifecycle"
---
# Universal Lifecycle

Use this as the default entry point for any new workspace work.

Hard Rules:
- No destructive shell commands unless explicitly approved.
- Prefer sandboxed or least-privileged execution.
- No secrets in output.
- Always report: files changed, commands run, checks/tests, next actions.

Phases (in order):
1) /analyze
2) /spec        -> MISSION.md + ACCEPTANCE.md
3) /design      -> ARCHITECTURE.md
4) /build       -> minimal working slice
5) /test        -> verification notes/results
6) /ops         -> RUNBOOK.md
7) /ship        -> packaging + handoff

Always output in this format:
1) Artifacts changed (filenames)
2) Commands run (if any)
3) Tests/checks performed + results
4) Next 3 actions (ordered)
