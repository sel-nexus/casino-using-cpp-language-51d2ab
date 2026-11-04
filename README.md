# Casino guessing game

A standalone C++ console game. No Node/Express migration, web server, database, or third-party runtime dependency is required.

## Build and play
Use a C++17 compiler (tested with GNU g++) from the repository root:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror 'main (1).cpp' -o casino
./casino
```

Enter your full name, a positive whole-number deposit, a bet of at least 100 that does not exceed your balance, and a guess from 1 to 10. Each answer occupies its own line. Invalid numeric input (including overflow and trailing text) is rejected and prompts again. Answer `y` or `n` to continue; uppercase is accepted. EOF at any prompt exits cleanly.

The existing game rules remain unchanged: matching guesses add ten times the bet to the current balance; losing guesses subtract the bet. Play ends when the balance is below 100 or you stop. Deposits below 100 are valid but cannot fund a round. Money is fictional and held only for the current process.

## Test
Requires Python 3 (standard library only) and g++:

```sh
python3 test_console.py --unit-only
python3 test_console.py
```

The runner builds native helper tests and the real application in an automatically cleaned temporary directory. Unit cases cover validated parsing and EOF. The full command also exercises console journeys with bounded subprocess timeouts; no browser is needed.

## Compatibility and scope
The source filename remains `main (1).cpp`. Input is now line-based: full names are supported, blank names and nonpositive deposits are rejected, and malformed values cannot leave the stream stuck. No migration or configuration variables are needed. The original random reseeding and signed-integer payout arithmetic are retained outside the input-repair scope; very large balances/bets may overflow during payouts, so this educational game is not suitable for real-money use.
