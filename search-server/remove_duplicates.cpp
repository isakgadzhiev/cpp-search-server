#include <algorithm>
#include "remove_duplicates.h"

std::set<std::string>CreateSet(const std::map<std::string, double> words_freqs) {
    std::set<std::string> result;
    for (const auto [word, freq] : words_freqs) {
        result.insert(word);
    }
    return result;
}

void RemoveDuplicates(SearchServer& search_server) {
    std::vector<int> ids_to_delete;
    std::map<std::set<std::string>, int> uniq_docs;
    for (const int document_id : search_server) {
        const auto set_words = CreateSet(search_server.GetWordFrequencies(document_id));
        const auto find_iterator = uniq_docs.find(set_words);
        if (find_iterator == uniq_docs.end()) { // дубликат не найден
            uniq_docs[set_words] = document_id;
        } else {
            ids_to_delete.push_back(find_iterator->second > document_id ? find_iterator->second : document_id);
        }
    }
    for (const int id : ids_to_delete) {
        std::cout << "Found duplicate document id "s << id << std::endl;
        search_server.RemoveDocument(id);
    }
}