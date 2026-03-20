---
description: "Execute the ops phase of the Neutral OS lifecycle"
---
# Ops

Objective:
- Make the system runnable, supportable, and safe to change.

Artifact:
- RUNBOOK.md

Process:
1) Document how to run:
   - prerequisites
   - install/setup steps
   - configuration (env vars, files)
   - start/stop commands
2) Document how it fails:
   - common failure modes
   - troubleshooting steps
   - where logs live and what to look for
3) Define rollback/undo:
   - how to revert changes
   - how to disable automation safely
4) Minimal observability:
   - logging approach
   - health checks (if applicable)
5) Define operational cadence:
   - what should be checked weekly/monthly
