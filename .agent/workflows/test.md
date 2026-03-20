---
description: "Execute the test phase of the Neutral OS lifecycle"
---
# Test

Objective:
- Verify behavior against ACCEPTANCE.md and document gaps.

Outputs:
- Update ACCEPTANCE.md with results (Pass/Fail/Blocked).
- Add TEST_NOTES.md if needed (optional).

Process:
1) Choose the acceptance checks relevant to the current slice.
2) Create or run tests/validation steps:
   - unit tests when feasible
   - lightweight integration checks
   - sanity checks with sample inputs
3) Probe edge cases (2–5) and failure paths.
4) Record:
   - what was tested
   - what passed/failed
   - what’s unknown
   - next fixes needed
