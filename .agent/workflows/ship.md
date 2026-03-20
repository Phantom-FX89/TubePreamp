---
description: "Execute the ship phase of the Neutral OS lifecycle"
---
# Ship

Objective:
- Package and hand off a stable deliverable.

Outputs:
- Updated RUNBOOK.md
- A concise HANDOFF section in NOTES.md
- Updated CHANGELOG.md

Process:
1) Ensure acceptance checks for the slice are green or clearly documented.
2) Package deliverable:
   - app: run instructions + build artifacts if applicable
   - analytics: reproducible run command + outputs location
   - automation: export format + versioning + rollback plan
3) **Semantic Versioning Protocol**:
   - **Determine Scope**: Major (Breaking), Minor (Feature), or Patch (Fix).
   - **Update Source**: Bump version in `package.json` (or equivalent).
   - **Document**: Add entry to `CHANGELOG.md` with date and version.
   - **Tag**: Run `git tag vX.Y.Z`.
4) Provide Handoff:
   - what to deploy/run
   - what to monitor
   - what to do if it breaks
5) End with next 3 highest-leverage improvements (backlog seeds).
