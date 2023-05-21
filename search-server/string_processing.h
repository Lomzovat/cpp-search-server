#pragma once

#include <set>
#include <vector>
#include <string>

std::vector<std::string_view> SplitIntoWords(const std::string_view text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    std::string str_temp;
    for (auto& str : strings) {
        str_temp = str;
        if (!str.empty()) {
            non_empty_strings.insert(str_temp);
        }
    }
    return non_empty_strings;
}
