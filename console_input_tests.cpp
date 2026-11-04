// Exercise real readers and original round branches in a deterministic unit harness.
#include "console_input.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
int checks = 0;

// Fail immediately when an observed value violates the test contract.
// Args:
//     condition: Whether the assertion holds.
//     message: Failure context for the native test runner.
void check(bool condition, const std::string& message) {
    ++checks;
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// Fix only the unit harness's random source so both real payout branches run.
// Returns:
//     A value producing computer number five in the unchanged round logic.
int fixed_rand() { return 4; }

// Ignore reseeding in the unit harness without changing production randomness.
// Args:
//     seed: The original round's time-derived seed.
void fixed_srand(unsigned int seed) { static_cast<void>(seed); }
}  // namespace

#define main casino_entry
#define rand fixed_rand
#define srand fixed_srand
#include "main (1).cpp"
#undef srand
#undef rand
#undef main

namespace {
// Capture a real round in-process and restore global console streams on exit.
// Args:
//     data: Complete input lines for the round.
// Returns:
//     All output produced by the original round implementation.
std::string run_round(const std::string& data) {
    std::istringstream input(data);
    std::ostringstream output;
    const auto old_state = std::cin.rdstate();
    auto* old_input = std::cin.rdbuf(input.rdbuf());
    auto* old_output = std::cout.rdbuf(output.rdbuf());
    std::cin.clear();
    round1();
    std::cin.rdbuf(old_input);
    std::cin.clear(old_state);
    std::cout.rdbuf(old_output);
    return output.str();
}

// Verify whitespace handling and strict signed integer parsing at int limits.
void test_parser() {
    check(console_input::trim(" \tAda  Lovelace\r ") == "Ada  Lovelace", "trim full name");
    check(console_input::trim("\t\r\n\f\v ").empty(), "trim blank");
    check(console_input::trim("").empty(), "trim empty");
    for (const int expected : {0, 1, -1, std::numeric_limits<int>::min(),
                               std::numeric_limits<int>::max()}) {
        int value = 73;
        check(console_input::parse_integer(std::to_string(expected), value), "valid integer");
        check(value == expected, "parsed exact integer value");
    }
    int value = 73;
    check(console_input::parse_integer(" \t+100\r ", value) && value == 100, "padding and plus");
    for (const std::string text : {"", " ", "abc", "100xyz", "1.5", "1 2", "0x10",
                                   "+", "-", "+-1", "++1", "--1", "999999999999999999999",
                                   "-999999999999999999999"}) {
        value = 73;
        check(!console_input::parse_integer(text, value), "reject malformed: " + text);
        check(value == 73, "parse failure preserves value");
    }
    value = 73;
    check(!console_input::parse_integer(
              std::to_string(static_cast<long long>(std::numeric_limits<int>::max()) + 1), value),
          "reject int max plus one");
    check(!console_input::parse_integer(
              std::to_string(static_cast<long long>(std::numeric_limits<int>::min()) - 1), value),
          "reject int min minus one");
    check(value == 73, "overflow preserves destination");
}

// Verify full names, blank recovery, final unterminated lines, and EOF state.
void test_names() {
    std::istringstream input("\n \t\r\n  Ada  Lovelace \r\nNext Player");
    std::ostringstream output;
    std::string name = "unchanged";
    check(console_input::read_name(input, output, "Name: ", name), "name recovery");
    check(name == "Ada  Lovelace", "full name retained");
    check(output.str().find("Name cannot be blank.") != std::string::npos, "blank name feedback");
    check(console_input::read_name(input, output, "Name: ", name) && name == "Next Player",
          "next line remains available and final line accepted");
    for (const std::string data : {"", "\n \t\n"}) {
        std::istringstream eof(data);
        name = "unchanged";
        check(!console_input::read_name(eof, output, "Name: ", name), "name EOF");
        check(name == "unchanged", "name EOF preserves destination");
    }
}

// Verify rejected integer lines cannot leak tokens or overwrite validated state.
void test_integers() {
    std::ostringstream output;
    int value = 73;
    std::istringstream input("abc\n\n9999999999999999999999\n100xyz\n99\n501\n100\n500");
    check(console_input::read_integer(input, output, "Bet: ", 100, 500, value,
                                     "too small", "too large") && value == 100, "bet recovery lower bound");
    check(output.str().find("too small") != std::string::npos, "lower feedback");
    check(output.str().find("too large") != std::string::npos, "upper feedback");
    check(output.str().find("valid whole number") != std::string::npos, "malformed feedback");
    check(console_input::read_integer(input, output, "Bet: ", 100, 500, value,
                                     "too small", "too large") && value == 500, "balance upper bound");
    for (const int expected : {1, 10, std::numeric_limits<int>::max()}) {
        std::istringstream boundary(std::to_string(expected));
        check(console_input::read_integer(boundary, output, "Deposit: ", 1,
                                         std::numeric_limits<int>::max(), value, "positive", "large"),
              "positive deposit boundary");
        check(value == expected, "exact boundary deposit");
    }
    std::istringstream guess("0\n11\n-1\n1\n10\n");
    check(console_input::read_integer(guess, output, "Guess: ", 1, 10, value, "range", "range")
          && value == 1, "guess lower bound recovery");
    check(console_input::read_integer(guess, output, "Guess: ", 1, 10, value, "range", "range")
          && value == 10, "guess upper bound");
    std::istringstream deposit("0\n-1\n1\n");
    check(console_input::read_integer(deposit, output, "Deposit: ", 1,
                                     std::numeric_limits<int>::max(), value, "positive", "large")
          && value == 1, "nonpositive deposit recovery");
    for (const std::string data : {"", "abc\n\n0\n999999999999999999999\n"}) {
        std::istringstream eof(data);
        value = 73;
        check(!console_input::read_integer(eof, output, "Integer: ", 1, 10, value,
                                          "low", "high"), "integer EOF after rejection");
        check(value == 73, "integer EOF preserves destination");
    }
}

// Verify both cases of y/n, rejected whole words, and clean EOF.
void test_decisions() {
    std::ostringstream output;
    for (const std::string text : {"y", "Y", "n", "N", " \tY\r "}) {
        std::istringstream input(text);
        char value = '?';
        check(console_input::read_decision(input, output, "Continue: ", value), "valid decision");
        check(value == (text.find('n') != std::string::npos || text.find('N') != std::string::npos
                        ? 'n' : 'y'), "decision normalized");
    }
    std::istringstream input("\n \t\nmaybe\nyes\nno\nyn\n1\ny junk\nN\n");
    char value = '?';
    check(console_input::read_decision(input, output, "Continue: ", value) && value == 'n',
          "decision recovery consumes whole lines");
    check(output.str().find("Please enter y or n.") != std::string::npos, "decision feedback");
    for (const std::string data : {"", "maybe\n\n"}) {
        std::istringstream eof(data);
        value = '?';
        check(!console_input::read_decision(eof, output, "Continue: ", value), "decision EOF");
        check(value == '?', "decision EOF preserves destination");
    }
}

// Verify unchanged payouts, balance exhaustion, continuation, and initial low deposits.
// Notes:
//     Large payout signed-int overflow is pre-existing and outside this input repair.
void test_round_branches() {
    const auto win = run_round("Ada Lovelace\n500\n100\n5\nN\n");
    check(win.find("Ada Lovelace, enter") != std::string::npos, "round retains full name");
    check(win.find("You have won 1000 as total.") != std::string::npos, "original tenfold winnings");
    check(win.find("Your new balance is: 1500") != std::string::npos, "original additive win math");
    check(win.find("Have a nice day.") != std::string::npos, "uppercase stop");
    const auto loss = run_round("Player\n500\n100\n1\nn\n");
    check(loss.find("You have lost 100") != std::string::npos, "original loss branch");
    check(loss.find("The correct number was 5") != std::string::npos, "deterministic native number");
    check(loss.find("Now you have 400 as balance.") != std::string::npos, "original subtractive loss math");
    const auto exhausted = run_round("Player\n100\n100\n1\n");
    check(exhausted.find("Now you have 0 as balance.") != std::string::npos, "all-in loss");
    check(exhausted.find("Your balance is less than 100.") != std::string::npos, "exhaustion exit");
    check(exhausted.find("Do you want to continue?") == std::string::npos, "no continuation when exhausted");
    const auto continued = run_round("Player\n500\n100\n5\nY\n1500\n1\n");
    check(continued.find("Now you have 0 as balance.") != std::string::npos, "updated balance permits next bet");
    const auto low = run_round("Player\n99\n");
    check(low.find("Your current balance is 99") != std::string::npos, "positive low deposit accepted");
    check(low.find("money to bet") == std::string::npos, "original low-deposit no-round branch");
}

// Verify every original prompt returns on immediate EOF and EOF after invalid input.
void test_round_eof() {
    for (const std::string data : {"", "\n", "Player\n", "Player\nabc\n", "Player\n500\n",
                                   "Player\n500\nabc\n", "Player\n500\n100\n",
                                   "Player\n500\n100\n11\n", "Player\n500\n100\n5\n",
                                   "Player\n500\n100\n5\nmaybe\n"}) {
        const auto output = run_round(data);
        check(output.find("Goodbye. Have a nice day.") != std::string::npos, "round clean EOF exit");
    }
}
}  // namespace

// Run native assertions and report a nonzero failure status to Python unittest.
// Returns:
//     Zero after all six groups pass, or one with the failing assertion.
int main() {
    try {
        test_parser();
        test_names();
        test_integers();
        test_decisions();
        test_round_branches();
        test_round_eof();
        std::cout << "6 native test groups passed; " << checks << " assertions passed.\n";
        return 0;
    } catch (const std::runtime_error& error) {
        std::cerr << "Native test failed: " << error.what() << '\n';
        return 1;
    }
}
