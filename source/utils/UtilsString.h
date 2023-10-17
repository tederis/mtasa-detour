#pragma once

#include <vector>
#include <string>
#include <sstream>

namespace WorldAssistant
{

// Return substrings split by a delimiter char
inline std::vector<std::string> Split(const std::string& s, char delimiter, bool trim = false)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);

    while (std::getline(tokenStream, token, delimiter)) {
        if (trim) {
            token.erase(0, token.find_first_not_of(" "));
        }

        tokens.push_back(token);
    }

    return tokens;
}

// Return string with whitespace trimmed from the beginning and the end
inline std::string Trim(const std::string& str)
{
    const size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) {
        return str;
    }

    const size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

}