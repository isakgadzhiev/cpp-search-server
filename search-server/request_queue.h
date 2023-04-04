#pragma once

#include <deque>
#include <vector>

#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
            : search_server_(search_server)
            , no_results_request_(0)
    {
    }
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
        if (requests_.size() >= min_in_day_) {
            requests_.pop_front();
            --no_results_request_;
        }
        requests_.push_front({static_cast<int>(requests_.size())});
        if (result.empty()) {
            ++no_results_request_;
        }
        return result;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        int result_count;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int no_results_request_;
};