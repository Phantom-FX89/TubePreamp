---
description: "Execute the design phase of the Neutral OS lifecycle"
---
# Design

Objective:
- Convert a signed-off spec into a reliable system design.

Artifact:
- ARCHITECTURE.md

Process:
1) Define the intended outcome and success criteria (from MISSION/ACCEPTANCE).
2) List constraints that materially shape design (security, cost, time, tooling).
3) Decompose into components:
   - Interfaces/contracts between components
   - Data flow and control flow
   - **SOLID Audit**: Verify the decomposition doesn't violate SRP or high coupling.
4) Choose a minimal architecture that supports the next vertical slice.
5) Identify failure modes + mitigations:
   - retries, idempotency, timeouts
   - error handling and logging
   - rollback / kill switch (especially for automation)
6) Provide 2 options:
   - Conservative (simple, low-risk)
   - Flexible (more capability, more complexity)
7) Recommend one with rationale.
8) Output an ASCII diagram and key contracts to ARCHITECTURE.md.
