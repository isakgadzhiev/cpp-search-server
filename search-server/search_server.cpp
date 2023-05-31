#include "search_server.h"

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (document_ids_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    string_storage_.emplace_back(document);
    const auto words = SplitIntoWordsNoStop(string_storage_.back());

    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);

}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.cbegin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.cend();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view, double> empty_map = {};
    if (!word_freqs_.count(document_id)) {
        return empty_map;
    } else {
        return word_freqs_.at(document_id);
    }
}

void SearchServer::RemoveDocument(int document_id){
    if (document_ids_.count(document_id)) {
        documents_.erase(document_id);
        document_ids_.erase(document_id);

        for (auto& [word, freqs] : word_freqs_.at(document_id)) {
            word_to_document_freqs_.at(word).erase(document_id);
        }
        word_freqs_.erase(document_id);
    }
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy, int document_id){
    return RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy, int document_id){
    if (document_ids_.count(document_id)) {
        documents_.erase(document_id);
        document_ids_.erase(document_id);
        auto& words_rel_to_remove = word_freqs_.at(document_id);
        std::vector<const std::string_view*> only_words_to_remove(word_freqs_.at(document_id).size());
        std::transform(std::execution::par, words_rel_to_remove.begin(), words_rel_to_remove.end(), only_words_to_remove.begin(),
                       [] (auto& words) {return &words.first;});
        std::for_each(std::execution::par, only_words_to_remove.begin(), only_words_to_remove.end(),
                      [&] (const auto& word) {word_to_document_freqs_.at(*word).erase(document_id);});
        word_freqs_.erase(document_id);
    }
}

SearchServer::TupleType SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    if (document_ids_.count(document_id) == 0) {
        throw std::out_of_range("Invalid document_id"s);
    }
    const auto query = ParseQuery(raw_query);
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            std::vector<std::string_view> matched_words = {};
            return {matched_words, documents_.at(document_id).status};
        }
    }
    std::vector<std::string_view> matched_words;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return {matched_words, documents_.at(document_id).status};
}

SearchServer::TupleType SearchServer::MatchDocument(const std::execution::sequenced_policy&, const std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

SearchServer::TupleType SearchServer::MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query, int document_id) const {
    if (document_ids_.count(document_id) == 0) {
        throw std::out_of_range("Invalid document_id"s);
    }
    const auto query = ParseQuery(raw_query, true);

    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
                    [&](const auto& minus_word) {
                        if (word_to_document_freqs_.count(minus_word)) {
                            return word_to_document_freqs_.at(minus_word).count(document_id) != 0;
                        } else return false;
                    })) {
        std::vector<std::string_view> matched_words = {};
        return {matched_words, documents_.at(document_id).status};
    }
    std::vector<std::string_view> matched_words(query.plus_words.size());
    std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
                 [&] (const auto& plus_word) {
                     if (word_to_document_freqs_.count(plus_word)) {
                         return word_to_document_freqs_.at(plus_word).count(document_id) != 0;
                     } else return false;
                 });
    matched_words.erase(std::remove(matched_words.begin(), matched_words.end(), ""), matched_words.end());
    std::sort(matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique(matched_words.begin(), matched_words.end()), matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(),ratings.end(),0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view result = text;
    bool is_minus = false;
    if (result[0] == '-') {
        is_minus = true;
        result = result.substr(1);
    }
    if (result.empty() || result[0] == '-' || !IsValidWord(result)) {
        throw std::invalid_argument("Query word "s + std::string(result) + " is invalid");
    }

    return {result, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text, bool Is_par_policy) const {
    Query result;
    for (const std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    if (!Is_par_policy) {
        std::sort(result.plus_words.begin(), result.plus_words.end());
        std::sort(result.minus_words.begin(), result.minus_words.end());
        result.plus_words.erase(std::unique(result.plus_words.begin(), result.plus_words.end()), result.plus_words.end());
        result.minus_words.erase(std::unique(result.minus_words.begin(), result.minus_words.end()), result.minus_words.end());
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
