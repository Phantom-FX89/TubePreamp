---
description: "Execute the build phase of the Neutral OS lifecycle"
---
# Build

Objective:
- Implement the smallest working slice that satisfies the next acceptance check.

Rules:
- Prefer clarity over cleverness.
- Avoid premature abstraction.
- Only build what is needed to satisfy the next acceptance check(s).
- Mark unknowns with TODO rather than guessing.

Process:
1) Select the next unmet acceptance check(s).
2) Implement a vertical slice end-to-end (even if minimal).
3) Add basic instrumentation:
   - meaningful logs/errors
   - clear configuration points
4) Update NOTES.md with decisions and TODOs.
5) Stop once the slice runs and the chosen acceptance check(s) can be validated.
