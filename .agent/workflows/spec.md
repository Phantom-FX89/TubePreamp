---
description: "Execute the spec phase of the Neutral OS lifecycle"
---
# Spec

Objective:
- Produce a mission packet and acceptance checks before design/build.

Artifacts:
- MISSION.md
- ACCEPTANCE.md

Process:
1) Fill/update MISSION.md:
   - Objective (measurable)
   - In-scope / Out-of-scope
   - Constraints (security, offline/online, performance, budget, deadlines)
   - Inputs/Outputs (exact files/endpoints/reports)
   - Ownership & operational expectations (who runs it, where it runs)
2) Fill/update ACCEPTANCE.md:
   - 8–15 testable checks written as “Given/When/Then” or “If X, expect Y”
   - Include 2–3 sample cases
3) Confirm readiness to proceed:
   - No major unknowns blocking architecture
   - Acceptance checks are clear and testable
4) Stop. Do not start architecture or code in this mode.
