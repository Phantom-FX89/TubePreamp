---
description: "Execute the fix phase (Root Cause Analysis + Repair)"
---
# Fix (Debug Protocol)

Objective:
- Resolve a bug by identifying the root cause, not just masking symptoms.

Process:
1) **Reproduce**:
   - Create a minimal reproduction case.
   - If you can't reproduce it, you can't fix it.
2) **Isolate**:
   - Binary search the system/timeline to find *where* and *when* it broke.
   - Use logging/tracing to see the state before the crash.
3) **Root Cause Analysis (RCA)**:
   - Ask "Why?" 5 times.
   - Identify the *fundamental* flaw (logic, race condition, data corruption).
4) **Implement Fix**:
   - Write a failing test first (TDD).
   - Implement the minimal fix.
5) **Verify**:
   - Run the new test.
   - Run regression suite to ensure no side effects.
