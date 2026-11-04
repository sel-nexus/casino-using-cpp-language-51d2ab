// Read complete console lines without leaving malformed tokens in the stream.
#ifndef CONSOLE_INPUT_H
#define CONSOLE_INPUT_H

#include <charconv>
#include <cctype>
#include <istream>
#include <ostream>
#include <string>
#include <system_error>

namespace console_input {

// Remove surrounding whitespace while preserving internal name spacing.
// Args:
//     line: A complete input line.
// Returns:
//     The non-padding portion, or an empty string for a blank line.
inline std::string trim(const std::string& line) {
    const auto first = line.find_first_not_of(" \t\r\n\f\v");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = line.find_last_not_of(" \t\r\n\f\v");
    return line.substr(first, last - first + 1);
}

// Parse one complete decimal integer without accepting overflow or junk.
// Args:
//     line: Text to parse; surrounding whitespace and a leading plus are allowed.
//     value: Destination updated only on success.
// Returns:
//     Whether every non-padding character forms a representable integer.
inline bool parse_integer(const std::string& line, int& value) {
    const std::string text = trim(line);
    if (text.empty()) {
        return false;
    }
    const char* begin = text.data();
    const char* end = begin + text.size();
    if (*begin == '+') {
        ++begin;
        if (begin == end || *begin < '0' || *begin > '9') {
            return false;
        }
    }
    int parsed = 0;
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc() || result.ptr != end) {
        return false;
    }
    value = parsed;
    return true;
}

// Retry blank names and retain the full accepted name.
// Args:
//     input: Source of complete lines.
//     output: Destination for prompts and validation feedback.
//     prompt: Original name prompt, repeated after a rejected line.
//     name: Destination updated only after a nonblank line.
// Returns:
//     False on EOF or stream failure; true after accepting a name.
inline bool read_name(std::istream& input, std::ostream& output,
                      const std::string& prompt, std::string& name) {
    std::string line;
    while (output << prompt, std::getline(input, line)) {
        const std::string candidate = trim(line);
        if (!candidate.empty()) {
            name = candidate;
            return true;
        }
        output << "Name cannot be blank.\n";
    }
    return false;
}

// Retry malformed and out-of-range integers without changing the destination.
// Args:
//     input: Source of complete lines.
//     output: Destination for prompts and validation feedback.
//     prompt: Prompt used on each attempt.
//     minimum: Inclusive lower bound.
//     maximum: Inclusive upper bound; must not be below minimum.
//     value: Destination updated only after validation.
//     below_message: Feedback for values below the lower bound.
//     above_message: Feedback for values above the upper bound.
// Returns:
//     False on EOF or stream failure; true after accepting a bounded integer.
inline bool read_integer(std::istream& input, std::ostream& output,
                         const std::string& prompt, int minimum, int maximum,
                         int& value, const std::string& below_message,
                         const std::string& above_message) {
    std::string line;
    while (output << prompt, std::getline(input, line)) {
        int candidate = 0;
        if (!parse_integer(line, candidate)) {
            output << "Please enter a valid whole number.\n";
        } else if (candidate < minimum) {
            output << below_message << '\n';
        } else if (candidate > maximum) {
            output << above_message << '\n';
        } else {
            value = candidate;
            return true;
        }
    }
    return false;
}

// Accept only a single y or n, ignoring case and surrounding whitespace.
// Args:
//     input: Source of complete lines.
//     output: Destination for prompts and validation feedback.
//     prompt: Continuation prompt repeated after invalid input.
//     decision: Destination normalized to lowercase only on success.
// Returns:
//     False on EOF or stream failure; true after accepting a decision.
inline bool read_decision(std::istream& input, std::ostream& output,
                          const std::string& prompt, char& decision) {
    std::string line;
    while (output << prompt, std::getline(input, line)) {
        const std::string text = trim(line);
        if (text.size() == 1) {
            const char candidate = static_cast<char>(
                std::tolower(static_cast<unsigned char>(text[0])));
            if (candidate == 'y' || candidate == 'n') {
                decision = candidate;
                return true;
            }
        }
        output << "Please enter y or n.\n";
    }
    return false;
}

}  // namespace console_input

#endif
