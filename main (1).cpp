#include <iostream>
#include <cstdlib>
#include <ctime>
#include <limits>
#include "console_input.h"
using namespace std;

void rules();
void round1();

int main() {
    rules();
    cout << "\n\n";
    round1();
    return 0;
}

void round1() {
    string name;
    if (!console_input::read_name(cin, cout, "Enter your name: ", name)) {
        cout << "\nGoodbye. Have a nice day.\n";
        return;
    }
    cout << "\n\n";

    int deposit1;
    if (!console_input::read_integer(cin, cout,
            "Enter the money to deposit for the game: ",
            1, numeric_limits<int>::max(), deposit1,
            "Deposit must be positive.", "Deposit exceeds the integer range.")) {
        cout << "\nGoodbye. Have a nice day.\n";
        return;
    }
    cout << "\n\n";

    cout << "Your current balance is " << deposit1 << "\n\n";

    while (deposit1 >= 100) {
        int bet1;
        if (!console_input::read_integer(cin, cout,
                name + ", enter the money to bet (minimum 100): ",
                100, deposit1, bet1, "Minimum bet is 100.",
                "Money exceeds your balance.")) {
            cout << "\nGoodbye. Have a nice day.\n";
            return;
        }
        cout << "\n\n";

        srand(time(0)); // Seed for random number generation
        int computer1 = rand() % 10 + 1; // Generate a number between 1 and 10
        int guess1;

        if (!console_input::read_integer(cin, cout,
                "Guess a number between 1 to 10: ", 1, 10, guess1,
                "Number exceeds the range.", "Number exceeds the range.")) {
            cout << "\nGoodbye. Have a nice day.\n";
            return;
        }
        cout << "\n\n";

        if (guess1 == computer1) {
            cout << "You have won: " << bet1 << "\n\n";
            int winnings = bet1 * 10;
            deposit1 += winnings;
            cout << "You have won " << winnings << " as total.\n";
            cout << "Your new balance is: " << deposit1 << "\n\n";
        } else {
            cout << "You have lost " << bet1 << "\n\n";
            cout << "The correct number was " << computer1 << "\n\n";
            deposit1 -= bet1;
            cout << "Now you have " << deposit1 << " as balance.\n\n";
        }

        if (deposit1 < 100) {
            cout << "Your balance is less than 100. You can't continue playing. Have a nice day.\n";
            break;
        }

        char decision;
        if (!console_input::read_decision(cin, cout,
                "Do you want to continue? (y/n): ", decision)) {
            cout << "\nGoodbye. Have a nice day.\n";
            return;
        }
        cout << "\n\n";

        if (decision == 'n' || decision == 'N') {
            cout << "Have a nice day.\n";
            break;
        }
    }
}

void rules() {
    cout << "-------------------------------------------------------\n";
    cout << "Rules\n";
    cout << "1) Enter a number between 1 to 10.\n";
    cout << "2) If you win, you will get 10 times your bet.\n";
    cout << "3) If you lose, you will lose your bet.\n";
    cout << "4) Minimum bet is 100.\n";
    cout << "5) The game continues until your balance drops below 100 or you choose to stop.\n";
    cout << "-------------------------------------------------------\n";
}
