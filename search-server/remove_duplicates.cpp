#include <algorithm>
#include "remove_duplicates.h"

std::set<std::string>CreateSet(const std::map<std::string, double> words_freqs) {
    std::set<std::string> result;
    std::transform(words_freqs.begin(), words_freqs.end(), std::inserter(result, result.begin()),
                   [] (const auto& elem) {return elem.first;});
    return result;
}

void RemoveDuplicates(SearchServer& search_server) {
    std::vector<int> ids_to_delete;
    std::set<std::set<std::string>> uniq_docs;
    for (const int document_id : search_server) {
        const auto set_words = CreateSet(search_server.GetWordFrequencies(document_id));
        const auto find_iterator = uniq_docs.find(set_words);
        if (find_iterator == uniq_docs.end()) { // дубликат не найден
            uniq_docs.insert(set_words);
        } else { // дубликат есть
            ids_to_delete.push_back(document_id);
        }
    }
    for (const int id : ids_to_delete) {
        std::cout << "Found duplicate document id "s << id << std::endl;
        search_server.RemoveDocument(id);
    }
}