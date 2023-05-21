#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> words;
    std::string_view space_delimiter = " ";


    int64_t start_pos = text.find_first_not_of(space_delimiter);
    const int64_t end_pos = text.npos;

    while (start_pos != end_pos) {
        int64_t space = text.find(' ', start_pos);
        words.push_back(space == end_pos
            ? text.substr(start_pos)
            : text.substr(start_pos, space - start_pos));
        start_pos = text.find_first_not_of(space_delimiter, space);
    }
    return words;
}