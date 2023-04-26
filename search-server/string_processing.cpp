
#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(const std::string_view text) {
    std::vector<std::string_view> words;
    std::string_view word;
    auto pos = text.find_first_not_of(" ");
    const auto pos_end = text.npos;
    while (pos != pos_end) {
        auto space = text.find(' ', pos);
        words.push_back(space == pos_end ? text.substr(pos) : text.substr(pos, space - pos));
        pos = text.find_first_not_of(" ", space);
    }
    return words;
}


