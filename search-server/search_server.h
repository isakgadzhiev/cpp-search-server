#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>
#include <numeric>
#include <cmath>
#include <execution>
#include <deque>
#include <execution>
#include <mutex>

#include "document.h"
#include "string_processing.h"
#include "log_duration.h"
#include "concurrent_map.h"

using namespace std::string_literals;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string_view stop_words_text)
            : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }
    explicit SearchServer(const std::string& stop_words_text)
            : SearchServer(std::string_view (stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status,
                     const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentStatus status) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query) const;



    int GetDocumentCount() const;

    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy, int document_id);
    void RemoveDocument(std::execution::parallel_policy, int document_id);

    using MatchDocuments = std::tuple<std::vector<std::string_view>, DocumentStatus>;
    MatchDocuments MatchDocument(const std::string_view raw_query, int document_id) const;
    MatchDocuments MatchDocument(const std::execution::sequenced_policy&, const std::string_view raw_query, int document_id) const;
    MatchDocuments MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query, int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::deque<std::string> string_storage;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(const std::string_view text) const;

    Query ParseQuery(const std::string_view text, const bool is_seq_pol = true) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);
    sort(matched_documents.begin(), matched_documents.end(),
         [](const Document &lhs, const Document &rhs) {
             if (std::abs(lhs.relevance - rhs.relevance) < DELTA) {
                 return lhs.rating > rhs.rating;
             } else {
                 return lhs.relevance > rhs.relevance;
             }
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template<typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const {
    if (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
        return FindTopDocuments(raw_query, document_predicate);
    } else {
        const auto query = ParseQuery(raw_query, false);
        auto matched_documents = FindAllDocuments(policy, query, document_predicate);
        sort(std::execution::par, matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs) {
                 if (std::abs(lhs.relevance - rhs.relevance) < DELTA) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    return FindAllDocuments(std::execution::seq, query, document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(100);
    std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), [&] (const auto& plus_word) {
        if (word_to_document_freqs_.count(plus_word) == 0) {
            return;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(plus_word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(plus_word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
            }
        }
    });

    std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [&] (const auto& minus_word) {
        if (word_to_document_freqs_.count(minus_word) == 0) {
            return;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(minus_word)) {
            document_to_relevance.Erase(document_id);
        }
    });
    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}
