#include "string_processing.h"


std::vector<std::string_view> SplitIntoWords(const std::string_view text) {
    std::vector<std::string_view> words;
    std::size_t start = 0;
    for (std::size_t i = 0; i <= text.size(); ++i) {
        if (i == text.size() || text[i] == ' ') {
            if (i > start) {
                words.push_back(text.substr(start, i - start));
                start = i + 1;
            }
        }
    }
    return words;
}