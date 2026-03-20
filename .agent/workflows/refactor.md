---
description: "Execute the refactor phase (Safe Structural Improvement)"
---
# Refactor

Objective:
- Improve code structure/readability without altering external behavior.

Hard Rule:
- **Two Hats**: Never refactor and add features at the same time.

Process:
1) **Baseline**:
   - Ensure the code to be changed has passing tests.
   - If no tests, write "Characterization Tests" first to pin existing behavior.
2) **Strategy**:
   - Identify the smell (Duplication, Long Method, Complex Conditional).
   - Choose the move (Extract Method, Rename Variable, Replace Conditional with Polymorphism).
3) **Execute**:
   - Apply the refactor in small micro-steps.
   - Run tests after EVERY small change.
4) **Verify**:
   - Confirm all tests pass.
   - Commmit with "refactor: <description>".
