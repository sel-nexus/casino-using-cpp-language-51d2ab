"""Compile native units and real console journeys using only the standard library.

Run python3 test_console.py --unit-only before console journey verification.
Run python3 test_console.py for all tests; production journeys never mock randomness.
Build products live in a temporary directory outside the workspace.
"""

import argparse
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent
FLAGS = ["-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]


class NativeBuild(unittest.TestCase):
    """Own warning-free native builds and temporary executable lifetime."""

    @classmethod
    def setUpClass(cls) -> None:
        """Compile both sources outside the workspace, surfacing compiler failures."""
        # The harness may redirect TMPDIR into the workspace. Prefer the native
        # POSIX scratch directory when present, then retain the containment check.
        temp_parent = Path("/tmp") if Path("/tmp").is_dir() else Path(tempfile.gettempdir())
        cls.temp = tempfile.TemporaryDirectory(prefix="casino-console-tests-", dir=temp_parent)
        cls.addClassCleanup(cls.temp.cleanup)
        directory = Path(cls.temp.name).resolve()
        if directory == ROOT or ROOT in directory.parents:
            raise RuntimeError("System temporary directory must be outside the workspace")
        cls.program = directory / "casino"
        cls.units = directory / "console_input_tests"
        for source, target in [("main (1).cpp", cls.program),
                               ("console_input_tests.cpp", cls.units)]:
            result = subprocess.run(
                ["g++", *FLAGS, str(ROOT / source), "-o", str(target)],
                capture_output=True, text=True, encoding="utf-8", timeout=30,
                check=False,
            )
            if result.returncode != 0 or result.stderr:
                raise AssertionError(
                    f"Warning-free compile failed for {source}:\n"
                    f"{result.stdout}\n{result.stderr}"
                )


class HelperUnitTests(NativeBuild):
    """Run helper assertions and deterministic original-round branch tests."""

    def test_native_helpers_and_original_branches(self) -> None:
        """Require all six native groups to pass with real assertions."""
        result = subprocess.run(
            [str(self.units)], capture_output=True, text=True, encoding="utf-8",
            timeout=5, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stderr, "")
        self.assertRegex(result.stdout, r"6 native test groups passed; [1-9]\d* assertions passed")
        # Include the native assertion count in the parent-readable unit run log.
        self.assertGreater(int(re.search(r"(\d+) assertions", result.stdout).group(1)), 100)
        print(result.stdout.strip(), flush=True)


class ConsoleJourneyTests(NativeBuild):
    """Exercise only the real compiled console program with bounded subprocesses."""

    def journey(self, data: str) -> str:
        """Run a complete console input script and require clean termination.

        Args:
            data: Lines sent to stdin, followed by EOF.
        Returns:
            Captured console output.
        """
        result = subprocess.run(
            [str(self.program)], input=data, capture_output=True, text=True,
            encoding="utf-8", timeout=3, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stderr, "")
        return result.stdout

    def assert_round_math(self, output: str, deposit: int, bet: int, guess: int) -> int:
        """Validate the actual random outcome against unchanged payout arithmetic.

        Args:
            output: Output containing exactly one completed round.
            deposit: Balance before the round.
            bet: Accepted wager.
            guess: Accepted guess.
        Returns:
            The actual balance after the round.
        Notes:
            Signed-int overflow for very large payouts is a pre-existing issue;
            these journeys use safe amounts and do not alter payout rules.
        """
        wins = re.findall(r"Your new balance is: (\d+)", output)
        losses = re.findall(r"Now you have (\d+) as balance", output)
        self.assertEqual(len(wins) + len(losses), 1, output)
        if wins:
            self.assertIn(f"You have won: {bet}", output)
            self.assertIn(f"You have won {bet * 10} as total.", output)
            self.assertNotIn("You have lost", output)
            self.assertEqual(int(wins[0]), deposit + bet * 10)
            return int(wins[0])
        self.assertIn(f"You have lost {bet}", output)
        correct = re.search(r"The correct number was (\d+)", output)
        self.assertIsNotNone(correct, output)
        computer = int(correct.group(1))
        self.assertGreaterEqual(computer, 1)
        self.assertLessEqual(computer, 10)
        self.assertNotEqual(computer, guess)
        self.assertNotIn("You have won:", output)
        self.assertEqual(int(losses[0]), deposit - bet)
        return int(losses[0])

    def test_full_nonblank_names_and_positive_low_deposits(self) -> None:
        """Recover blank names and retain full names without consuming the deposit."""
        output = self.journey("\n \t\n  Ada  Lovelace \r\n500\n100\n1\nn\n")
        self.assertEqual(output.count("Name cannot be blank."), 2)
        self.assertIn("Ada  Lovelace, enter the money to bet", output)
        self.assertIn("Your current balance is 500", output)
        self.assert_round_math(output, 500, 100, 1)
        for deposit in (1, 99, 2147483647):
            with self.subTest(deposit=deposit):
                output = self.journey(f"Full Name\n{deposit}\n")
                self.assertIn(f"Your current balance is {deposit}", output)
                if deposit < 100:
                    self.assertNotIn("money to bet", output)
                else:
                    self.assertIn("Goodbye.", output)

    def test_integer_recovery_at_every_numeric_prompt(self) -> None:
        """Reject blanks, malformed, overflow, trailing junk, and invalid ranges."""
        malformed = ("abc", "9999999999999999999999", "2147483648", "-2147483649",
                     "100junk", "1.5", "1 2", "", " \t", "+-100", "0x64")
        stages = (
            ("deposit", "Full Name\n", "500\n100\n1\nn\n", ("0", "-1")),
            ("bet", "Full Name\n500\n", "100\n1\nn\n", ("99", "501", "0", "-1")),
            ("guess", "Full Name\n500\n100\n", "1\nn\n", ("0", "11", "-1")),
        )
        for stage, prefix, suffix, invalid_ranges in stages:
            for invalid in malformed + invalid_ranges:
                with self.subTest(stage=stage, invalid=invalid):
                    output = self.journey(prefix + invalid + "\n" + suffix)
                    self.assert_round_math(output, 500, 100, 1)
                    if invalid in malformed:
                        self.assertEqual(output.count("Please enter a valid whole number."), 1)
                    else:
                        message = {"deposit": "Deposit must be positive.",
                                   "guess": "Number exceeds the range.",
                                   "bet": "Money exceeds your balance." if invalid == "501"
                                   else "Minimum bet is 100."}[stage]
                        self.assertIn(message, output)

    def test_eof_at_every_prompt_and_after_each_rejection(self) -> None:
        """Terminate at initial and retry prompts, including the original hang case."""
        stages = (
            ("name", "", ("", " ", "\t")),
            ("deposit", "Player\n", ("abc", "0", "2147483648", "1junk", "")),
            ("bet", "Player\n500\n", ("abc", "99", "501", "2147483648", "100junk", "")),
            ("guess", "Player\n500\n100\n", ("abc", "0", "11", "2147483648", "1junk", "")),
            ("decision", "Player\n500\n100\n1\n", ("maybe", "yes", "yn", "")),
            ("second bet", "Player\n500\n100\n1\nY\n", ("abc", "")),
        )
        for stage, prefix, rejections in stages:
            for suffix in ("", *(text + "\n" for text in rejections)):
                with self.subTest(stage=stage, suffix=suffix):
                    output = self.journey(prefix + suffix)
                    self.assertEqual(output.count("Goodbye. Have a nice day."), 1)
                    if stage in ("name", "deposit", "bet", "guess"):
                        self.assertNotIn("You have won:", output)
                        self.assertNotIn("You have lost", output)

    def test_case_insensitive_stop_and_invalid_decision_recovery(self) -> None:
        """Reject complete invalid decisions instead of accepting their first letter."""
        for decision in ("n", "N", " \tN\r "):
            with self.subTest(decision=decision):
                output = self.journey("Player\n500\n100\n10\n\nmaybe\nyes\nno\nyn\n"
                                      "1\ny junk\n" + decision + "\n")
                self.assertEqual(output.count("Please enter y or n."), 7)
                self.assertEqual(output.count("money to bet"), 1)
                self.assertNotIn("Goodbye.", output)
                self.assertIn("Have a nice day.", output)
                self.assert_round_math(output, 500, 100, 10)

    def test_case_insensitive_continue_and_balance_state(self) -> None:
        """Play two real random rounds and validate the carried-forward balance."""
        for decision in ("y", "Y", " \tY "):
            with self.subTest(decision=decision):
                output = self.journey(f"Player\n500\n100\n1\n{decision}\n100\n10\nn\n")
                sections = output.split("Do you want to continue? (y/n): ")
                self.assertEqual(len(sections), 3, output)
                balance = self.assert_round_math(sections[0], 500, 100, 1)
                self.assert_round_math(sections[1], balance, 100, 10)
                self.assertEqual(output.count("money to bet"), 2)

    def test_bet_guess_boundaries_and_random_all_in_outcome(self) -> None:
        """Accept exact bet/guess boundaries and verify either random all-in result."""
        for bet in (100, 500):
            for guess in (1, 10):
                with self.subTest(bet=bet, guess=guess):
                    output = self.journey(f"Player\n500\n +{bet}\r\n {guess} \nN\n")
                    balance = self.assert_round_math(output, 500, bet, guess)
                    if balance < 100:
                        self.assertIn("Your balance is less than 100.", output)
                        self.assertNotIn("Do you want to continue?", output)
                    else:
                        self.assertIn("Do you want to continue?", output)
                    self.assertNotIn("valid whole number", output)


def main() -> int:
    """Select units alone or the complete suite without marking journeys skipped.

    Returns:
        Zero only when every selected test passes.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unit-only", action="store_true",
                        help="Compile both binaries and run native units only; no console journeys")
    args = parser.parse_args()
    loader = unittest.TestLoader()
    suite = unittest.TestSuite([loader.loadTestsFromTestCase(HelperUnitTests)])
    if not args.unit_only:
        suite.addTests(loader.loadTestsFromTestCase(ConsoleJourneyTests))
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
