# Changelog

## Console input repair
- Updated `main (1).cpp` in place to accept full names, require positive integer deposits, validate bets and guesses, accept only case-insensitive y/n, and exit cleanly at EOF.
- Added `console_input.h` for line-based stream readers. Blank input, trailing junk, nonnumeric values, integer overflow, and out-of-range values now produce feedback and retry instead of leaving the game stuck.
- Added `console_input_tests.cpp` with six native groups and 110 assertions, including unchanged win/loss arithmetic and EOF branches.
- Added `test_console.py`, a Python standard-library runner with native unit and real console journey tests. All seven top-level tests passed; strict C++17 warning-as-error build passed. Independent test critic: PASS, score 1.00, zero high failures.
- Added `README.md` for native build/run/testing instructions, `.gitignore` for runtime and pipeline files, and refreshed the code graph.

### Compatibility and migrations
No language migration, database, dependencies, or environment configuration was introduced. Game rules, payout operations, and production random-number behavior are unchanged. Each prompt now consumes one complete line; blank names, nonpositive deposits, numeric trailing text, and continuation words such as `yes`/`no` are rejected. Use `y` or `n` instead. The requested five-line limit applies to the final summary, as clarified by the requestor.

### Known pre-existing limitations
This educational game retains signed-int payout arithmetic that can overflow for very large balances/bets and reseeds randomness each round. These were outside the approved input-only repair scope and remain unsuitable for real-money use. The original repository had no test runner; malformed bet input followed by EOF was confirmed to hang before the repair.
